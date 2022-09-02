#include "ssfmods.h"

#include "modsutil.h"

#include "../global_funcs.h"
#include "../ssvconstant.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>

std::unique_ptr<ModsUtil> SSFMods::util;

SSFMods::SSFMods() {

}

SSFMods::SSFMods(std::vector<int> &&vars,
                                       std::unordered_set<std::vector<bool>> &&entries)
    : vars(vars), models(entries) {
    for (std::vector<bool> entry : this->models) {
        assert(vars.size() == entry.size());
    }

}

SSFMods::SSFMods(std::vector<int> &varorder, std::vector<SSFMods *> &disjuncts)
    : vars(varorder) {
    for (SSFMods *formula : disjuncts) {
        if(formula->vars != vars) {
            std::cerr << "Tried to build Explicit union with different varorder." << std::endl;
            return;
        }
        for (Model model : formula->models) {
            models.insert(model);
        }
    }
}

SSFMods::SSFMods(std::stringstream &input, Task &task) {
    if(!util) {
        util = std::unique_ptr<ModsUtil>(new ModsUtil(task));
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
        std::cerr << "Error when parsing MODS Formula: varamount not correct." << std::endl;
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

bool SSFMods::check_statement_b1(std::vector<const StateSetVariable *> &left,
                                     std::vector<const StateSetVariable *> &right) const {

    std::vector<SSFMods *> left_mods = convert_to_formalism<SSFMods>(left, this);
    std::vector<SSFMods *> right_mods = convert_to_formalism<SSFMods>(right, this);

    right_mods.erase(std::remove(right_mods.begin(), right_mods.end(),
                                     &(util->emptyformula)), right_mods.end());

    SSFMods right_union;
    // build 1 formula for the disjunction
    if(right_mods.size() > 1) {
        for (SSFMods *formula: right_mods) {
            if (formula->vars != right_mods[0]->vars) {
                // TODO: in theory, we could allow different varorder as long as the same vars are used
                std::cerr << "Union of explicit sets with different varorder not allowed." << std::endl;
                return false;
            }
        }
        right_union = SSFMods(right_mods[0]->vars, right_mods);
        right_mods.clear();
        right_mods.push_back(&right_union);
    }

    // left empty -> right side must be valid
    if(left_mods.empty()) {
        left_mods.push_back(&(util->trueformula));
    }

    // use first formula on left as reference
    SSFMods *reference = left_mods.front();
    left_mods.erase(left_mods.begin());

    ModsUtil::SubsetCheckHelper helper =
            util->get_subset_checker_helper(reference->vars, left_mods, right_mods);

    // loop over each model of the reference formula
    for (const std::vector<bool> &model : reference->models) {
        if(!util->is_model_contained(model,helper)) {
            return false;
        }
    }
    return true;
}

bool SSFMods::check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                     std::vector<const StateSetVariable *> &left,
                                     std::vector<const StateSetVariable *> &right,
                                     std::unordered_set<int> &action_indices) const {
    assert(!progress.empty());
    std::vector<SSFMods *> left_mods = convert_to_formalism<SSFMods>(left, this);
    std::vector<SSFMods *> right_mods = convert_to_formalism<SSFMods>(right, this);
    std::vector<SSFMods *> prog_mods = convert_to_formalism<SSFMods>(progress, this);

    right_mods.erase(std::remove(right_mods.begin(), right_mods.end(),
                                     &(util->emptyformula)), right_mods.end());

    // the union formulas must all talk about the same variables
    if(!util->check_same_vars(right_mods)) {
        std::cerr << "Union of explicit sets contains different variables." << std::endl;
        return false;
    }

    SSFMods *prog_singular = prog_mods[0];
    SSFMods dummy;
    if(prog_mods.size() > 1) {
        dummy = SSFMods(prog_mods[0]->vars, prog_mods);
        prog_singular = &dummy;
    }
    std::vector<int> varorder;
    ModsUtil::SubsetCheckHelper helper;
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
            helper = util->get_subset_checker_helper(varorder,left_mods, right_mods);
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

bool SSFMods::check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                     std::vector<const StateSetVariable *> &left,
                                     std::vector<const StateSetVariable *> &right,
                                     std::unordered_set<int> &action_indices) const {
    assert(!regress.empty());
    std::vector<SSFMods *> left_mods = convert_to_formalism<SSFMods>(left, this);
    std::vector<SSFMods *> right_mods = convert_to_formalism<SSFMods>(right, this);
    std::vector<SSFMods *> reg_mods = convert_to_formalism<SSFMods>(regress, this);

    right_mods.erase(std::remove(right_mods.begin(), right_mods.end(),
                                     &(util->emptyformula)), right_mods.end());

    // the union formulas must all talk about the same variables
    if(!util->check_same_vars(right_mods)) {
        std::cerr << "Union of explicit sets contains different variables." << std::endl;
        return false;
    }

    SSFMods *reg_singular = reg_mods[0];
    SSFMods dummy;
    if(reg_mods.size() > 1) {
        dummy = SSFMods(reg_mods[0]->vars, reg_mods);
        reg_singular = &dummy;
    }
    std::vector<int> varorder;
    ModsUtil::SubsetCheckHelper helper;
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
            helper = util->get_subset_checker_helper(varorder,left_mods, right_mods);
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
                    /*for (bool val : model) {
                        std::cout << val << " ";
                    } std::cout << std::endl;*/
                    return false;
                }
            }


        } // end loop over models
    } // end loop over actions
    return true;
}

bool SSFMods::check_statement_b4(const StateSetFormalism *right, bool left_positive, bool right_positive) const {
    const std::vector<int> &superset_varorder = right->get_varorder();
    bool superset_varorder_is_subset = true;
    std::cout << "SDFS" << std::endl;
    std::vector<int> var_pos;
    std::cout << "SDFDSFSDFSF" << std::endl;
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

const SSFMods *SSFMods::get_compatible(const StateSetVariable *stateset) const {
    const SSFMods *ret = dynamic_cast<const SSFMods *>(stateset);
    if (ret) {
        return ret;
    }
    const SSVConstant *cformula = dynamic_cast<const SSVConstant *>(stateset);
    if (cformula) {
        return get_constant(cformula->get_constant_type());
    }
    return nullptr;
}

const SSFMods *SSFMods::get_constant(ConstantType ctype) const {
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


const std::vector<int> &SSFMods::get_varorder() const {
    return vars;
}

std::vector<Model> SSFMods::get_missing_var_values(Model &model, const std::vector<int> &missing_vars_pos) const {
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

bool SSFMods::is_contained(const std::vector<bool> &model) const {
    return (models.find(model) != models.end());
}

bool SSFMods::is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) const {
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

bool SSFMods::is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const {
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

bool SSFMods::get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) const {
    return false;
}

int SSFMods::get_model_count() const {
    return models.size();
}

StateSetBuilder<SSFMods> mods_builder("e");
