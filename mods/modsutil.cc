#include "modsutil.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <unordered_set>

ModsUtil::ModsUtil(Task &task)
    : task(task) {

    std::vector<int> varorder(1,0);
    std::unordered_set<std::vector<bool>> entries;
    emptyformula = SSFMods(std::move(varorder), std::move(entries));
    varorder = std::vector<int> {0};
    entries = std::unordered_set<std::vector<bool>> {{true},{false}};
    trueformula = SSFMods(std::move(varorder), std::move(entries));
    varorder = std::vector<int>();
    int var = 0;
    for ( int val : task.get_goal()) {
        if (val == 1) {
            varorder.push_back(var);
        }
        var++;
    }
    entries = std::unordered_set<std::vector<bool>> {std::vector<bool>(varorder.size(), true)};
    goalformula = SSFMods(std::move(varorder),std::move(entries));
    varorder = std::vector<int>(task.get_number_of_facts(),-1);
    for (size_t i = 0; i < varorder.size(); ++i) {
        varorder[i] = i;
    }
    entries = std::unordered_set<std::vector<bool>>();
    std::vector<bool> entry(varorder.size());
    var = 0;
    for ( int val : task.get_initial_state()) {
        if (val == 1) {
            entry[var] = true;
        }
    }
    entries.insert(std::move(entry));
    initformula = SSFMods(std::move(varorder),std::move(entries));


    hex.reserve(16);
    for(int i = 0; i < 16; ++i) {
        std::vector<bool> tmp = { (bool)((i >> 3)%2), (bool)((i >> 2)%2),
                                 (bool)((i >> 1)%2), (bool)(i%2)};
        hex.push_back(tmp);
    }

}

bool ModsUtil::check_same_vars(std::vector<SSFMods *> &formulas) {
    for(size_t i = 1; i < formulas.size(); ++i) {
        if(!std::is_permutation(formulas[i-1]->vars.begin(), formulas[i-1]->vars.end(),
                                formulas[i]->vars.begin())) {
            return false;
        }
    }
    return true;
}

inline std::vector<bool> get_transformed_model(GlobalModel &global_model,
                                               std::vector<GlobalModelVarOcc> var_occurences) {
    std::vector<bool> f_model(var_occurences.size(), false);
    for (size_t i = 0; i < var_occurences.size(); ++i) {
        GlobalModelVarOcc &var_occ = var_occurences[i];
        f_model[i] = global_model.at(var_occ.first)->at(var_occ.second);
    }
    return f_model;
}

ModsUtil::SubsetCheckHelper ModsUtil::get_subset_checker_helper(
        std::vector<int> &varorder,
        std::vector<SSFMods *> &left_formulas,
        std::vector<SSFMods *> &right_formulas) {

    SubsetCheckHelper helper;
    helper.varorder = varorder;

    for (SSFMods *formula : left_formulas) {
        if (formula->vars == helper.varorder) {
            helper.same_varorder_left.push_back(formula);
        } else {
            helper.other_varorder_left.push_back(OtherVarorderFormula(formula));
        }
    }
    for (SSFMods *formula : right_formulas) {
        if (formula->vars == helper.varorder) {
            helper.same_varorder_right.push_back(formula);
        } else {
            helper.other_varorder_right.push_back(OtherVarorderFormula(formula));
        }
    }

    helper.global_model.resize(helper.other_varorder_left.size()+2, nullptr);
    helper.model_extensions.resize(helper.other_varorder_left.size()+1);

    // Stores for each variable where in the "global model" it occurs.
    std::unordered_map<int,GlobalModelVarOcc> var_occurence_map;
    for (size_t i = 0; i < helper.varorder.size(); ++i)  {
        var_occurence_map.insert(std::make_pair(helper.varorder[i],
                                                std::make_pair(0,i)));
    }

    // Find for each other_formula where the variables occur.
    int index = 0;
    for (OtherVarorderFormula &of : helper.other_varorder_left) {
        int newvar_amount = 0;
        for (size_t i = 0; i < of.formula->vars.size(); ++i) {
            int var = of.formula->vars[i];
            auto pos = var_occurence_map.find(var);
            if (pos == var_occurence_map.end()) {
                GlobalModelVarOcc occ(index+1,newvar_amount++);
                pos = var_occurence_map.insert({var,occ}).first;
                of.newvars_pos.push_back(i);
            }
            of.var_occurences.push_back(pos->second);
        }
        // fill model_extensions with dummy vector
        helper.model_extensions[index] = ModelExtensions(1,std::vector<bool>(of.newvars_pos.size()));
        helper.global_model[index+1] = &(helper.model_extensions[index][0]);
        index++;
    }
    int right_newvars = 0;
    for (OtherVarorderFormula &of : helper.other_varorder_right) {
        for (size_t i = 0; i < of.formula->vars.size(); ++i) {
            int var = of.formula->vars[i];
            auto pos = var_occurence_map.find(var);
            if (pos == var_occurence_map.end()) {
                /*
                 * All formulas on the right must contain the same vars
                 * --> only the first can have new vars.
                 */
                assert(&of == &(helper.other_varorder_right[0]));
                GlobalModelVarOcc occ(index+1,right_newvars++);
                pos = var_occurence_map.insert({var,occ}).first;
                of.newvars_pos.push_back(i);
            }
            of.var_occurences.push_back(pos->second);
        }
    }
    helper.model_extensions[index] = ModelExtensions(1, std::vector<bool>(right_newvars));
    helper.global_model[index+1] = &(helper.model_extensions[index][0]);

    return helper;
}

bool ModsUtil::is_model_contained(const Model &model,
                                      SubsetCheckHelper &helper) {
    // For same varorder formulas no transformation is needed.
    for (SSFMods *formula : helper.same_varorder_left) {
        if (!formula->is_contained(model)) {
            return true;
        }
    }
    for (SSFMods *formula : helper.same_varorder_right) {
        if(formula->is_contained(model)) {
            return true;
        }
    }

    /*
     * other_formulas contain vars not occuring in varorder. We need to get all
     * models of the conjunction.
     * For example:
     *  - current model vars = {0,1,3}, current model = {t,f,f}
     *  - other formula vars = {0,2,3,4}
     * --> The conjunction contains current model combined with
     *     *all* models of other formula which fit {t,*,f,*}
     */
    helper.global_model[0] = &model;
    std::vector<int> pos(helper.other_varorder_left.size(), -1);
    int f_index = 0;
    bool done = false;

    /*
     * We expect f_index to index the next other_formula we need to check
     * (and get model_extensions from).
     * If f_index = other_formula.size(), we need to check the disjunctions.
     * Furthermore, we expect all pos[i] with i < f_index to point to valid positions
     * in addon, and global_model to point to those positions.
     */
    while(!done) {
        // check if contained in right side
        if (f_index == helper.other_varorder_left.size()) {
            Model &right_model = helper.model_extensions[f_index][0];
            // go over all assignments of vars only occuring right
            for (int count = 0; count < (1 << right_model.size()); ++count) {
                for (size_t i = 0; i < right_model.size(); ++i) {
                    right_model[i] = ((count >> i) % 2 == 1);
                }
                bool contained = false;
                for (OtherVarorderFormula &f : helper.other_varorder_right) {
                    Model f_model = get_transformed_model(helper.global_model, f.var_occurences);
                    if (f.formula->is_contained(f_model)) {
                        contained = true;
                        break;
                    }
                }
                if (!contained) {
                    return false;
                }
            }
            f_index--;
        } else {
            OtherVarorderFormula &f = helper.other_varorder_left[f_index];
            Model f_model = get_transformed_model(helper.global_model, f.var_occurences);
            helper.model_extensions[f_index] = f.formula->get_missing_var_values(
                        f_model, f.newvars_pos);
            pos[f_index] = -1;
        }
        while (f_index != -1 && pos[f_index] == helper.model_extensions[f_index].size()-1) {
            f_index--;
        }
        if (f_index == -1) {
            done = true;
        } else {
            pos[f_index]++;
            helper.global_model[f_index+1] = &(helper.model_extensions[f_index][pos[f_index]]);
            f_index++;
        }
    }
    return true;
}
