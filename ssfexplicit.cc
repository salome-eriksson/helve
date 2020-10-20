#include "ssfexplicit.h"
#include "ssfhorn.h"
#include "ssfbdd.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <math.h>

#include "global_funcs.h"


ExplicitUtil::ExplicitUtil(Task &task)
    : task(task) {

    std::vector<int> varorder(1,0);
    std::unordered_set<std::vector<bool>> entries;
    emptyformula = SSFExplicit(std::move(varorder), std::move(entries));
    varorder = std::vector<int> {0};
    entries = std::unordered_set<std::vector<bool>> {{true},{false}};
    trueformula = SSFExplicit(std::move(varorder), std::move(entries));
    varorder = std::vector<int>();
    int var = 0;
    for ( int val : task.get_goal()) {
        if (val == 1) {
            varorder.push_back(var);
        }
        var++;
    }
    entries = std::unordered_set<std::vector<bool>> {std::vector<bool>(varorder.size(), true)};
    goalformula = SSFExplicit(std::move(varorder),std::move(entries));
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
    initformula = SSFExplicit(std::move(varorder),std::move(entries));


    hex.reserve(16);
    for(int i = 0; i < 16; ++i) {
        std::vector<bool> tmp = { (bool)((i >> 3)%2), (bool)((i >> 2)%2),
                                 (bool)((i >> 1)%2), (bool)(i%2)};
        hex.push_back(tmp);
    }

}

bool ExplicitUtil::check_same_vars(std::vector<SSFExplicit *> &formulas) {
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

ExplicitUtil::SubsetCheckHelper ExplicitUtil::get_subset_checker_helper(
        std::vector<int> &varorder,
        std::vector<SSFExplicit *> &left_formulas,
        std::vector<SSFExplicit *> &right_formulas) {

    SubsetCheckHelper helper;
    helper.varorder = varorder;

    for (SSFExplicit *formula : left_formulas) {
        if (formula->vars == helper.varorder) {
            helper.same_varorder_left.push_back(formula);
        } else {
            helper.other_varorder_left.push_back(OtherVarorderFormula(formula));
        }
    }
    for (SSFExplicit *formula : right_formulas) {
        if (formula->vars == helper.varorder) {
            helper.same_varorder_right.push_back(formula);
        } else {
            helper.other_varorder_right.push_back(OtherVarorderFormula(formula));
        }
    }

    helper.global_model.resize(helper.other_varorder_left.size()+2, nullptr);
    helper.model_extensions.resize(helper.other_varorder_left.size()+1);

    // Stores for each variable where in the "global model" it occurs.
    std::unordered_map <int,GlobalModelVarOcc> var_occurence_map;
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

bool ExplicitUtil::is_model_contained(const Model &model,
                                      SubsetCheckHelper &helper) {
    // For same varorder formulas no transformation is needed.
    for (SSFExplicit *formula : helper.same_varorder_left) {
        if (!formula->is_contained(model)) {
            return true;
        }
    }
    for (SSFExplicit *formula : helper.same_varorder_right) {
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

std::unique_ptr<ExplicitUtil> SSFExplicit::util;

SSFExplicit::SSFExplicit() {

}

SSFExplicit::SSFExplicit(std::vector<int> &&vars,
                                       std::unordered_set<std::vector<bool>> &&entries)
    : vars(vars), models(entries) {
    for (std::vector<bool> entry : this->models) {
        assert(vars.size() == entry.size());
    }

}

SSFExplicit::SSFExplicit(std::vector<int> &varorder, std::vector<SSFExplicit *> &disjuncts)
    : vars(varorder) {
    for (SSFExplicit *formula : disjuncts) {
        if(formula->vars != vars) {
            std::cerr << "Tried to build Explicit union with different varorder." << std::endl;
            return;
        }
        for (Model model : formula->models) {
            models.insert(model);
        }
    }
}

SSFExplicit::SSFExplicit(std::stringstream &input, Task &task) {
    if(!util) {
        util = std::unique_ptr<ExplicitUtil>(new ExplicitUtil(task));
    }

    int varamount;
    input >> varamount;
    vars.reserve(varamount);
    for(int i = 0; i < varamount ; ++i) {
        int var;
        input >> var;
        vars.push_back(var);
    }

    std::string s;
    input >> s;

    if(s.compare(":") != 0) {
        std::cerr << "Error when parsing Explicit Formula: varamount not correct." << std::endl;
        exit_with(ExitCode::PARSING_ERROR);
    }

    // TODO: would a reserve make sense? but then we need to know the number of models

    input >> s;
    while(s.compare(";") != 0) {
        std::vector<bool> entry(varamount);
        int pos = 0;
        const std::vector<bool> *vec;
        for (int i = 0; i < (varamount/4); ++i) {
            if (s.at(i) < 'a') {
                vec = &(util->hex.at(s.at(i)-'0'));
            } else {
                vec = &(util->hex.at(s.at(i)-'a'+10));
            }
            for (int i = 0; i < 4; ++i) {
                entry[pos++] = vec->at(i);
            }
        }
        if(varamount%4 != 0) {
            if (s.at(varamount/4) < 'a') {
                vec = &(util->hex.at(s.at(varamount/4)-'0'));
            } else {
                vec = &(util->hex.at(s.at(varamount/4)-'a'+10));
            }
            for (int i = 0; i < (varamount%4); ++i) {
                entry[pos++] = vec->at(i);
            }
        }
        models.insert(std::move(entry));
        input >> s;
    }
}

bool SSFExplicit::check_statement_b1(std::vector<const StateSetVariable *> &left,
                                     std::vector<const StateSetVariable *> &right) const {

    std::vector<SSFExplicit *> left_explicit = convert_to_formalism<SSFExplicit>(left, this);
    std::vector<SSFExplicit *> right_explicit = convert_to_formalism<SSFExplicit>(right, this);

    right_explicit.erase(std::remove(right_explicit.begin(), right_explicit.end(),
                                     &(util->emptyformula)), right_explicit.end());

    SSFExplicit right_union;
    // build 1 formula for the disjunction
    if(right_explicit.size() > 1) {
        for (SSFExplicit *formula: right_explicit) {
            if (formula->vars != right_explicit[0]->vars) {
                // TODO: in theory, we could allow different varorder as long as the same vars are used
                std::cerr << "Union of explicit sets with different varorder not allowed." << std::endl;
                return false;
            }
        }
        right_union = SSFExplicit(right_explicit[0]->vars, right_explicit);
        right_explicit.clear();
        right_explicit.push_back(&right_union);
    }

    // left empty -> right side must be valid
    if(left_explicit.empty()) {
        left_explicit.push_back(&(util->trueformula));
    }

    // use first formula on left as reference
    SSFExplicit *reference = left_explicit.front();
    left_explicit.erase(left_explicit.begin());

    ExplicitUtil::SubsetCheckHelper helper =
            util->get_subset_checker_helper(reference->vars, left_explicit, right_explicit);

    // loop over each model of the reference formula
    for (const std::vector<bool> &model : reference->models) {
        if(!util->is_model_contained(model,helper)) {
            return false;
        }
    }
    return true;
}

bool SSFExplicit::check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                     std::vector<const StateSetVariable *> &left,
                                     std::vector<const StateSetVariable *> &right,
                                     std::unordered_set<int> &action_indices) const {
    assert(!progress.empty());
    std::vector<SSFExplicit *> left_explicit = convert_to_formalism<SSFExplicit>(left, this);
    std::vector<SSFExplicit *> right_explicit = convert_to_formalism<SSFExplicit>(right, this);
    std::vector<SSFExplicit *> prog_explicit = convert_to_formalism<SSFExplicit>(progress, this);

    right_explicit.erase(std::remove(right_explicit.begin(), right_explicit.end(),
                                     &(util->emptyformula)), right_explicit.end());

    // the union formulas must all talk about the same variables
    if(!util->check_same_vars(right_explicit)) {
        std::cerr << "Union of explicit sets contains different variables." << std::endl;
        return false;
    }

    SSFExplicit *prog_singular = prog_explicit[0];
    SSFExplicit dummy;
    if(prog_explicit.size() > 1) {
        dummy = SSFExplicit(prog_explicit[0]->vars, prog_explicit);
        prog_singular = &dummy;
    }
    std::vector<int> varorder;
    ExplicitUtil::SubsetCheckHelper helper;
    for (int action_index : action_indices) {

        // check if varorder stays the same; if not get a new helper
        std::vector<int> new_varorder(prog_singular->vars);
        const Action &action = util->task.get_action(action_index);
        std::vector<int> add_pos;
        std::vector<int> del_pos;
        std::vector<int> pre_to_check;
        std::vector<int> pre_to_add;

        for (size_t var = 0; var < action.change.size(); ++var) {
            if (action.change[var] != 0) {
                auto var_pos = std::find(new_varorder.begin(), new_varorder.end(), var);
                if (var_pos == new_varorder.end()) {
                    if (action.change[var] == 1) {
                        add_pos.push_back(new_varorder.size());
                    } else {
                        del_pos.push_back(new_varorder.size());
                    }
                    new_varorder.push_back(var);
                } else {
                    if (action.change[var] == 1) {
                        add_pos.push_back(std::distance(new_varorder.begin(), var_pos));
                    } else {
                        del_pos.push_back(std::distance(new_varorder.begin(), var_pos));
                    }
                }

            }
        }
        // do this after add/del since vars that are not in prog but both pre and add/del
        // only get set by add/del
        for (int var : action.pre) {
            auto var_pos = std::find(new_varorder.begin(), new_varorder.end(), var);
            if (var_pos == new_varorder.end()) {
                pre_to_add.push_back(new_varorder.size());
                new_varorder.push_back(var);
            } else {
                int var_pos_int = std::distance(new_varorder.begin(), var_pos);
                if(var_pos_int < prog_singular->vars.size()) {
                    pre_to_check.push_back(var_pos_int);
                }
            }
        }

        if (new_varorder != varorder) {
            std::swap(varorder,new_varorder);
            helper = util->get_subset_checker_helper(varorder,left_explicit, right_explicit);
        }

        Model model;
        model.reserve(varorder.size());

        for (Model prog_model : prog_singular->models) {
            // check preconditions
            bool preconditions_met = true;
            for (int pos : pre_to_check) {
                if (prog_model[pos] == false) {
                    preconditions_met = false;
                    break;
                }
            }
            // preconditions not true -> action not applicable
            if (!preconditions_met) {
                continue;
            }

            model.clear();
            model.insert(std::end(model), std::begin(prog_model), std::end(prog_model));
            model.resize(varorder.size());

            // apply changes from action and check if the model is contained
            for (int pos : pre_to_add) {
                model[pos] = true;
            }
            for (int pos : add_pos) {
                model[pos] = true;
            }
            for (int pos : del_pos) {
                model[pos] = false;
            }

            if(!util->is_model_contained(model,helper)) {
                return false;
            }

        } // end loop over models
    } // end loop over actions
    return true;
}

bool SSFExplicit::check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                     std::vector<const StateSetVariable *> &left,
                                     std::vector<const StateSetVariable *> &right,
                                     std::unordered_set<int> &action_indices) const {
    assert(!regress.empty());
    std::vector<SSFExplicit *> left_explicit = convert_to_formalism<SSFExplicit>(left, this);
    std::vector<SSFExplicit *> right_explicit = convert_to_formalism<SSFExplicit>(right, this);
    std::vector<SSFExplicit *> reg_explicit = convert_to_formalism<SSFExplicit>(regress, this);

    right_explicit.erase(std::remove(right_explicit.begin(), right_explicit.end(),
                                     &(util->emptyformula)), right_explicit.end());

    // the union formulas must all talk about the same variables
    if(!util->check_same_vars(right_explicit)) {
        std::cerr << "Union of explicit sets contains different variables." << std::endl;
        return false;
    }

    SSFExplicit *reg_singular = reg_explicit[0];
    SSFExplicit dummy;
    if(reg_explicit.size() > 1) {
        dummy = SSFExplicit(reg_explicit[0]->vars, reg_explicit);
        reg_singular = &dummy;
    }
    std::vector<int> varorder;
    ExplicitUtil::SubsetCheckHelper helper;
    for (int action_index : action_indices) {

        // check if varorder stays the same; if not get a new helper
        std::vector<int> new_varorder(reg_singular->vars);
        const Action &action = util->task.get_action(action_index);

        std::vector<int> add_pos;
        std::vector<int> del_pos;
        std::vector<int> pre_pos;
        std::vector<int> eff_not_pre;

        for (int var : action.pre) {
            auto var_pos = std::find(new_varorder.begin(), new_varorder.end(), var);
            if (var_pos == new_varorder.end()) {
                pre_pos.push_back(new_varorder.size());
                new_varorder.push_back(var);
            } else {
                pre_pos.push_back(std::distance(new_varorder.begin(), var_pos));
            }
        }
        for (size_t var = 0; var < action.change.size(); ++var) {
            if (action.change[var] != 0) {
                int var_pos_int= std::distance(new_varorder.begin(), std::find(new_varorder.begin(), new_varorder.end(), var));
                // we can ignore add/delete effects of vars not occuring in the model
                if (var_pos_int < reg_singular->vars.size()) {
                    if (action.change[var] == 1) {
                        add_pos.push_back(var_pos_int);
                    } else {
                        del_pos.push_back(var_pos_int);
                    }
                    if (std::find(action.pre.begin(), action.pre.end(), var) == action.pre.end()) {
                        eff_not_pre.push_back(var);
                    }
                }
            }
        }

        if (new_varorder != varorder) {
            std::swap(varorder,new_varorder);
            helper = util->get_subset_checker_helper(varorder,left_explicit, right_explicit);
        }

        Model model;
        model.reserve(varorder.size());

        for (Model reg_model : reg_singular->models) {
            // check add/del
            bool add_del_met = true;
            for (int pos : add_pos) {
                if (reg_model[pos] == false) {
                    add_del_met = false;
                    break;
                }
            }
            for (int pos : del_pos) {
                if (reg_model[pos] == true) {
                    add_del_met = false;
                    break;
                }
            }
            // add/del not correct -> action not backwards applicable
            if (!add_del_met) {
                continue;
            }

            model.clear();
            model.insert(std::end(model), std::begin(reg_model), std::end(reg_model));
            model.resize(varorder.size());

            // apply changes from action and check if the model is contained
            for (int pos : pre_pos) {
                model[pos] = true;
            }

            // check all combinations for eff_not_pre
            for (int count = 0; count < (1 << eff_not_pre.size()); ++count) {
                for (size_t i = 0; i < eff_not_pre.size(); ++i) {
                    model[eff_not_pre[i]] = ((count >> i) % 2 == 1);
                }
                if(!util->is_model_contained(model,helper)) {
                    for (bool val : model) {
                        std::cout << val << " ";
                    } std::cout << std::endl;
                    return false;
                }
            }


        } // end loop over models
    } // end loop over actions
    return true;
}

bool SSFExplicit::check_statement_b4(const StateSetFormalism *right, bool left_positive, bool right_positive) const {
    const std::vector<int> &superset_varorder = right->get_varorder();
    bool superset_varorder_is_subset = true;
    std::vector<int> var_pos;
    for (int var : superset_varorder) {
        auto pos = std::find(vars.begin(), vars.end(), var);
        if (pos == vars.end()) {
            superset_varorder_is_subset = false;
            break;
        } else {
            var_pos.push_back(std::distance(vars.begin(), pos));
        }
    }

    if (left_positive && right_positive) {
        // superset varorder does not contain new vars --> can do model check
        if (superset_varorder_is_subset) {
            std::vector<bool> transformed_model(var_pos.size());
            for (Model model : models) {
                for (size_t i = 0; i < transformed_model.size(); ++i) {
                    transformed_model[i] = model[var_pos[i]];
                }
                if (!right->is_contained(transformed_model)) {
                    return false;
                }
            }
            return true;
        } else if (right->supports_im()) {
            for (Model model : models) {
                if (!right->is_implicant(vars, model)) {
                    return false;
                }
            }
            return true;
        } else if (right->supports_tocnf()) {
            int count = 0;
            std::vector<int> varorder;
            std::vector<bool> clause;
            while(right->get_clause(count, varorder, clause)) {
                if (!is_entailed(varorder, clause)) {
                    return false;
                }
                count++;
            }
            return true;
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }


    } else if (left_positive && !right_positive) {
        // superset varorder does not contain new vars --> can do model check
        if (superset_varorder_is_subset) {
            std::vector<bool> transformed_model(var_pos.size());
            for (Model model : models) {
                for (size_t i = 0; i < transformed_model.size(); ++i) {
                    transformed_model[i] = model[var_pos[i]];
                }
                if (!right->is_contained(transformed_model)) {
                    return false;
                }
            }
            return true;
        } else if (right->supports_ce()) {
            for (Model model : models) {
                std::vector<bool> clause(model.size());
                for (size_t i = 0; i < model.size(); ++i) {
                    clause[i] = !model[i];
                }
                if (!right->is_entailed(vars, clause)) {
                    return false;
                }
            }
        } else if (right->supports_todnf()) {
            return right->check_statement_b4(this, true, false);
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }


    } else if (!left_positive && right_positive) {
        // superset varorder does not contain new vars --> can do model check
        if (superset_varorder_is_subset && right->supports_me()) {
            std::vector<bool> transformed_model(var_pos.size());
            int count;
            for (Model model : models) {
                for (size_t i = 0; i < transformed_model.size(); ++i) {
                    transformed_model[i] = model[var_pos[i]];
                }
                if (!right->is_contained(transformed_model)) {
                    count++;
                }
            }
            int sup_nonmodels = pow(2.0,superset_varorder.size()) - right->get_model_count();
            if (count != sup_nonmodels*pow(2.0,vars.size())) {
                return false;
            }
            return true;
        } else if (right->supports_tocnf()) {
            return right->check_statement_b4(this, false, true);
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }
    } else { // both negative
        return right->check_statement_b4(this, true, true);
    }
}

const SSFExplicit *SSFExplicit::get_compatible(const StateSetVariable *stateset) const {
    const SSFExplicit *ret = dynamic_cast<const SSFExplicit *>(stateset);
    if (ret) {
        return ret;
    }
    const SSVConstant *cformula = dynamic_cast<const SSVConstant *>(stateset);
    if (cformula) {
        return get_constant(cformula->get_constant_type());
    }
    return nullptr;
}

const SSFExplicit *SSFExplicit::get_constant(ConstantType ctype) const {
    switch (ctype) {
    case ConstantType::EMPTY:
        return &(util->emptyformula);
        break;
    case ConstantType::INIT:
        return &(util->initformula);
        break;
    case ConstantType::GOAL:
        return &(util->goalformula);
        break;
    default:
        std::cerr << "Unknown Constant type: " << std::endl;
        return nullptr;
        break;
    }
}


const std::vector<int> &SSFExplicit::get_varorder() const {
    return vars;
}

std::vector<Model> SSFExplicit::get_missing_var_values(Model &model, const std::vector<int> &missing_vars_pos) const {
    std::vector<std::vector<bool>> ret;
    for (int count = 0; count < (1 << missing_vars_pos.size()); ++count) {
        std::vector<bool> entry(missing_vars_pos.size());
        for (size_t i = 0; i < missing_vars_pos.size(); ++i) {
            entry[i] = ((count >> i) % 2 == 1);
            model[missing_vars_pos[i]] = entry[i];
        }
        if (is_contained(model)) {
            ret.emplace_back(entry);
        }
    }
    return ret;
}

bool SSFExplicit::is_contained(const std::vector<bool> &model) const {
    return (models.find(model) != models.end());
}

bool SSFExplicit::is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) const {
    std::vector<bool> model(vars.size());
    std::vector<int> vars_to_fill;
    for (int var : vars) {
        auto pos = std::find(varorder.begin(), varorder.end(), var);
        if (pos == varorder.end()) {
            vars_to_fill.push_back(var);
        } else {
            model[var] = implicant[std::distance(varorder.begin(), pos)];
        }
    }

    for (int count = 0; count < (1 << vars_to_fill.size()); ++count) {
        for (size_t i = 0; i < vars_to_fill.size(); ++i) {
            model[vars_to_fill[i]] = ((count >> i) % 2 == 1);
        }
        if (!is_contained(model)) {
            return false;
        }
    }
    return true;
}

bool SSFExplicit::is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const {
    for (Model model : models) {
        bool found = false;
        for (size_t i = 0; i < clause.size(); ++i) {
            if (model[varorder[i]] == clause[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool SSFExplicit::get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) const {
    return false;
}

int SSFExplicit::get_model_count() const {
    return models.size();
}

StateSetBuilder<SSFExplicit> explicit_builder("e");
