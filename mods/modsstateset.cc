#include "modsstateset.h"

#include "actionapplier.h"
#include "modsconjunction.h"
#include "taskinformation.h"

#include "../cnf/cnfformula.h"

#include <algorithm>
#include <cassert>
#include <math.h>
#include <unordered_map>

namespace mods {


std::vector<const ModsFormula *> get_formulas(
        std::vector<const ModsStateSet *>&state_sets) {
    std::vector<const ModsFormula *>ret;
    ret.reserve(state_sets.size());
    for (const ModsStateSet *state_set : state_sets) {
        ret.push_back(state_set->get_formula());
    }
    return ret;
}

ModsStateSet::ModsStateSet(std::stringstream &input, const Task &task)
    : task_information(TaskInformation::get_task_information(task)),
      formula(input) {
}

ModsStateSet::ModsStateSet(const VariableOrder &variable_order,
                           std::unordered_set<Model> &&models,
                           const TaskInformation &task_information)
    : formula(variable_order, std::move(models)),
      task_information(task_information) {

}

const ModsFormula *ModsStateSet::get_formula() const {
    return &formula;
}

cnf::Disjunction ModsStateSet::get_cnf_disjunction(
        const std::vector<const ModsFormula *> &formulas,
        std::vector<cnf::CNFFormula> &cnf_formula_storage) const {
    std::vector<const cnf::CNFFormula *>pointer_vector;
    cnf_formula_storage.reserve(cnf_formula_storage.size() + formulas.size());
    pointer_vector.reserve(formulas.size());
    for (const ModsFormula *f : formulas) {
        cnf_formula_storage.push_back(f->transform_to_cnf());
        pointer_vector.push_back(&cnf_formula_storage.back());
    }
    return cnf::Disjunction(pointer_vector);
}

// TODO: maybe make this a method of Disjunction
bool ModsStateSet::cnf_disjunction_contains_model(
        const Model &model, const VariableOrder &variable_order,
        const cnf::Disjunction &disjunction) const {

    for (const Clause &clause : disjunction) {
        bool contained = false;
        for (size_t i = 0; i < model.size(); ++i) {
            if (clause.count(variable_order[i])
                    && (clause.at(variable_order[i]) == model[i])) {
                contained = true;
                break;
            }
        }
        if (!contained) {
            return false;
        }
    }
    return true;
}

bool ModsStateSet::check_statement_b1(std::vector<const StateSetVariable *> &left,
                                      std::vector<const StateSetVariable *> &right) const {

    std::vector<const ModsStateSet *> left_mods = const_convert_to_formalism(left, this);
    std::vector<const ModsStateSet *> right_mods = const_convert_to_formalism(right, this);
    std::vector<const ModsFormula *> left_formulas = get_formulas(left_mods);
    std::vector<const ModsFormula *> right_formulas = get_formulas(right_mods);
    std::vector<ModsFormula> new_formulas;
    ModsFormula::aggregate_by_varorder(right_formulas, new_formulas, false);

    if (right_formulas.size() == 1) {
        const ModsFormula &right_singular = *right_formulas[0];
        ModsConjunction left_conjunction(left_formulas,
                                         right_singular.get_variable_order());
        for (const Model &model : left_conjunction) {
            if (right_singular.contains(model) == 0) {
                return false;
            }
        }
        return true;
    } else { // right has multiple variable orders
        std::vector<cnf::CNFFormula> cnf_formula_storage;
        cnf::Disjunction disjunction = get_cnf_disjunction(right_formulas,
                                                           cnf_formula_storage);
        ModsConjunction left_conjunction(left_formulas);
        const VariableOrder &variable_order = left_conjunction.get_variable_order();
        for (const Model & model : left_conjunction) {
            if (!cnf_disjunction_contains_model(model, variable_order, disjunction)) {
                return false;
            }
        }
        return true;
    }
}

// TODO: make this (and b3) nicer / more compact
bool ModsStateSet::check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                      std::vector<const StateSetVariable *> &left,
                                      std::vector<const StateSetVariable *> &right,
                                      std::unordered_set<size_t> &action_indices) const {
    std::vector<const ModsStateSet *> prog_mods = const_convert_to_formalism(progress, this);
    std::vector<const ModsStateSet *> left_mods = const_convert_to_formalism(left, this);
    std::vector<const ModsStateSet *> right_mods = const_convert_to_formalism(right, this);
    std::vector<const ModsFormula *> prog_formulas = get_formulas(prog_mods);
    std::vector<const ModsFormula *> left_formulas = get_formulas(left_mods);
    std::vector<const ModsFormula *> right_formulas = get_formulas(right_mods);

    std::vector<ModsFormula> new_formulas;
    ModsFormula::aggregate_by_varorder(right_formulas, new_formulas, false);
    cnf::Disjunction disjunction({});
    std::vector<cnf::CNFFormula> cnf_formula_storage;
    if(right_formulas.size() > 1) {
        disjunction = get_cnf_disjunction(right_formulas, cnf_formula_storage);
    }

    ModsConjunction prog_conjunction(prog_formulas);
    std::vector<ModsConjunction> left_conjunctions;
    left_conjunctions.reserve(action_indices.size());
    std::vector<ActionApplier> action_appliers;
    for (size_t action_index : action_indices) {
        action_appliers.emplace_back(prog_conjunction.get_variable_order(),
                                     task_information.get_task().get_action(action_index),
                                     true);
        if (right_formulas.size() == 1) {
            left_conjunctions.emplace_back(
                        left_formulas, right_formulas[0]->get_variable_order(),
                        action_appliers.back().get_variable_order());
        } else {
            left_conjunctions.emplace_back(
                        left_formulas, VariableOrder({}),
                        action_appliers.back().get_variable_order());
        }
    }

    for (const Model &prog_model : prog_conjunction) {
        for (size_t i = 0; i < action_appliers.size(); ++i) {
            if (action_appliers[i].is_applicable(prog_model)) {
                Model applied_model = action_appliers[i].apply(prog_model);
                left_conjunctions[i].set_restriction(applied_model);
                for (const Model &left_model : left_conjunctions[i]) {
                    if (right_formulas.size() == 1) {
                        if (!right_formulas[0]->contains(left_model)) {
                            return false;
                        }
                    } else { // right has multiple variable orders
                        if (!cnf_disjunction_contains_model(left_model, left_conjunctions[i].get_variable_order(), disjunction)) {
                            return false;
                        }
                    }
                } // end looping over left models
            } // end if action was applicable
        }// end looping over actions
    }// end looping over prog models
    return true;
}

bool ModsStateSet::check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                      std::vector<const StateSetVariable *> &left,
                                      std::vector<const StateSetVariable *> &right,
                                      std::unordered_set<size_t> &action_indices) const {
    std::vector<const ModsStateSet *> reg_mods = const_convert_to_formalism(regress, this);
    std::vector<const ModsStateSet *> left_mods = const_convert_to_formalism(left, this);
    std::vector<const ModsStateSet *> right_mods = const_convert_to_formalism(right, this);
    std::vector<const ModsFormula *> reg_formulas = get_formulas(reg_mods);
    std::vector<const ModsFormula *> left_formulas = get_formulas(left_mods);
    std::vector<const ModsFormula *> right_formulas = get_formulas(right_mods);

    std::vector<ModsFormula> new_formulas;
    ModsFormula::aggregate_by_varorder(right_formulas, new_formulas, false);
    cnf::Disjunction disjunction({});
    std::vector<cnf::CNFFormula> cnf_formula_storage;
    if(right_formulas.size() > 1) {
        disjunction = get_cnf_disjunction(right_formulas, cnf_formula_storage);
    }

    ModsConjunction reg_conjunction(reg_formulas);
    std::vector<ModsConjunction> left_conjunctions;
    left_conjunctions.reserve(action_indices.size());
    std::vector<ActionApplier> action_appliers;
    for (size_t action_index : action_indices) {
        action_appliers.emplace_back(reg_conjunction.get_variable_order(),
                                     task_information.get_task().get_action(action_index),
                                     false);
        if (right_formulas.size() == 1) {
            left_conjunctions.emplace_back(
                        left_formulas, right_formulas[0]->get_variable_order(),
                        action_appliers.back().get_variable_order());
        } else {
            left_conjunctions.emplace_back(
                        left_formulas, VariableOrder({}),
                        action_appliers.back().get_variable_order());
        }
    }

    for (const Model &prog_model : reg_conjunction) {
        for (size_t i = 0; i < action_appliers.size(); ++i) {
            if (action_appliers[i].is_applicable(prog_model)) {
                Model applied_model = action_appliers[i].apply(prog_model);
                left_conjunctions[i].set_restriction(applied_model);
                for (const Model &left_model : left_conjunctions[i]) {
                    if (right_formulas.size() == 1) {
                        if (!right_formulas[0]->contains(left_model)) {
                            return false;
                        }
                    } else { // right has multiple variable orders
                        if (!cnf_disjunction_contains_model(left_model, left_conjunctions[i].get_variable_order(), disjunction)) {
                            return false;
                        }
                    }
                } // end looping over left models
            } // end if action was applicable
        }// end looping over actions
    }// end looping over prog models
    return true;
}

bool ModsStateSet::check_statement_b4(const StateSetFormalism *right,
                                      bool left_positive, bool right_positive) const {

    const VariableOrder &varorder = formula.get_variable_order();
    const VariableOrder &other_varorder = right->get_varorder();
    bool other_varorder_is_subset = true;
    std::vector<int> var_pos;
    for (unsigned var : other_varorder) {
        auto pos = std::find(varorder.begin(), varorder.end(), var);
        if (pos == varorder.end()) {
            other_varorder_is_subset = false;
            break;
        } else {
            var_pos.push_back(std::distance(varorder.begin(), pos));
        }
    }

    if (left_positive && right_positive) {
        // superset varorder does not contain new vars --> can do model check
        if (other_varorder_is_subset) {
            std::vector<bool> transformed_model(var_pos.size());
            for (auto it = formula.get_cbegin(); it != formula.get_cend(); ++it) {
                const Model &model = *it;
                for (size_t i = 0; i < transformed_model.size(); ++i) {
                    transformed_model[i] = model[var_pos[i]];
                }
                if (!right->is_contained(transformed_model)) {
                    return false;
                }
            }
            return true;
        } else if (right->supports_im()) {
            auto it = formula.get_cbegin();
            for (auto it = formula.get_cbegin(); it != formula.get_cend(); ++it) {
                const Model &model = *it;
                if (!right->is_implicant(varorder, model)) {
                    return false;
                }
            }
            return true;
        } else if (right->supports_tocnf()) {
            int count = 0;
            std::vector<unsigned> varorder;
            std::vector<bool> clause;
            while(right->get_clause(count, varorder, clause)) {
                if (!is_entailed(varorder, clause)) {
                    return false;
                }
                count++;
            }
            return true;
        } else {
            throw std::runtime_error("mixed representation subset check not possible");
            return false;
        }


    } else if (left_positive && !right_positive) {
        // superset varorder does not contain new vars --> can do model check
        if (other_varorder_is_subset) {
            std::vector<bool> transformed_model(var_pos.size());
            for (auto it = formula.get_cbegin(); it != formula.get_cend(); ++it) {
                const Model &model = *it;
                for (size_t i = 0; i < transformed_model.size(); ++i) {
                    transformed_model[i] = model[var_pos[i]];
                }
                if (!right->is_contained(transformed_model)) {
                    return false;
                }
            }
            return true;
        } else if (right->supports_ce()) {
            for (auto it = formula.get_cbegin(); it != formula.get_cend(); ++it) {
                const Model &model = *it;
                std::vector<bool> clause(model.size());
                for (size_t i = 0; i < model.size(); ++i) {
                    clause[i] = !model[i];
                }
                if (!right->is_entailed(varorder, clause)) {
                    return false;
                }
            }
        } else if (right->supports_todnf()) {
            return right->check_statement_b4(this, true, false);
        } else {
            throw std::runtime_error("mixed representation subset check not possible");
            return false;
        }


    } else if (!left_positive && right_positive) {
        // superset varorder does not contain new vars --> can do model check
        if (other_varorder_is_subset && right->supports_me()) {
            std::vector<bool> transformed_model(var_pos.size());
            int count;
            for (auto it = formula.get_cbegin(); it != formula.get_cend(); ++it) {
                const Model &model = *it;
                for (size_t i = 0; i < transformed_model.size(); ++i) {
                    transformed_model[i] = model[var_pos[i]];
                }
                if (!right->is_contained(transformed_model)) {
                    count++;
                }
            }
            int sup_nonmodels = pow(2.0,other_varorder.size()) - right->get_model_count();
            if (count != sup_nonmodels*pow(2.0,varorder.size())) {
                return false;
            }
            return true;
        } else if (right->supports_tocnf()) {
            return right->check_statement_b4(this, false, true);
        } else {
            throw std::runtime_error("mixed representation subset check not possible");
            return false;
        }
    } else { // both negative
        return right->check_statement_b4(this, true, true);
    }
    return false;
}

bool ModsStateSet::is_contained(const Model &model) const {
    const VariableOrder &local_order = formula.get_variable_order();
    Model local_model(local_order.size());
    for (size_t i = 0; i < local_order.size(); ++i) {
        local_model[i] = model[local_order[i]];
    }
    return formula.contains(local_model);
}

bool ModsStateSet::is_implicant(const VariableOrder &varorder,
                                const std::vector<bool> &implicant) const {
    std::vector<bool> global_model;
    for (size_t i = 0; i < varorder.size(); ++i) {
        int var = varorder[i];
        if (var >= global_model.size()) {
            global_model.resize(-1, var+1);
        }
        global_model[var] = implicant[i];
    }
    return is_contained(global_model);
}

bool ModsStateSet::is_entailed(const VariableOrder &varorder,
                               const std::vector<bool> &clause) const {
    std::vector<std::pair<int,int>> possible_checks;
    for (size_t i = 0; i < varorder.size(); ++i) {
        for (size_t j = 0; j < formula.get_variable_order().size(); ++j) {
            if (varorder[i] == formula.get_variable_order()[j]) {
                possible_checks.push_back({i,j});
                break;
            }
        }
    }
    auto it = formula.get_cbegin();
    while (it != formula.get_cend()) {
        const Model &model = *it;
        bool covered = false;
        for (std::pair<int,int> check : possible_checks) {
            if (model[check.second] == clause[check.first]) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            return false;
        }
        it++;
    }
    return true;
}

bool ModsStateSet::get_clause(int, VariableOrder &, std::vector<bool> &) const {
    throw std::runtime_error("Not implemented yet");
    return false;
}

int ModsStateSet::get_model_count() const {
    return formula.get_model_count();
}

const std::vector<unsigned> &ModsStateSet::get_varorder() const {
    return formula.get_variable_order();
}

const ModsStateSet *ModsStateSet::get_compatible(const StateSetVariable *stateset) const {
    const ModsStateSet *ret = dynamic_cast<const ModsStateSet *>(stateset);
    if (ret) {
        return ret;
    }
    const SSVConstant *cformula = dynamic_cast<const SSVConstant *>(stateset);
    if (cformula) {
        return get_constant(cformula->get_constant_type());
    }
    return nullptr;
}

const ModsStateSet *ModsStateSet::get_constant(ConstantType ctype) const {
    switch (ctype) {
    case ConstantType::EMPTY:
        return task_information.get_empty_set();
        break;
    case ConstantType::INIT:
        return task_information.get_initial_state_set();
        break;
    case ConstantType::GOAL:
        return task_information.get_goal_set();
        break;
    default:
        throw std::runtime_error("Unknown Constant type");
    }
}

StateSetBuilder<ModsStateSet> mods_builder("e");
}

