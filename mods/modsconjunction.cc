#include "modsconjunction.h"

#include <cassert>
#include <unordered_map>

namespace mods {
ModsConjunction::ModsConjunction(std::vector<const ModsFormula *> &elements,
                                 const VariableOrder &variable_order,
                                 const VariableOrder &restriction_variable_order)
    :variable_order(variable_order), conjuncts(elements),
     restriction_variable_order(restriction_variable_order),
     restriction_formula(restriction_variable_order,
                         std::move(std::unordered_set<Model>())),
     variable_order_index(-1), assignment_amount_of_new_variables(0) {

    /*
     * For efficiency reasons we combine elements with the same varorder into a
     * new ModsStateSet (stored in local_formulas) that contains the explicit
     * conjunction of these elements.
     */
    ModsFormula::aggregate_by_varorder(conjuncts, local_formulas, true);

    if (!this->restriction_variable_order.empty()) {
        conjuncts.insert(conjuncts.begin(), &restriction_formula);
    }

    // If no order was given, build one with all vars occuring in the conjunction.
    if(variable_order.empty()) {
        build_variable_order_from_conjunction();
    }

    /*
     * Build information on how we can extract a model in our variable order.
     * and save for each element which variables have been seen in a
     * previous element already (needed for checking consistenc.
     */
    std::unordered_map<unsigned, std::vector<VarPosition>> var_positions_map;
    for (size_t c_index = 0; c_index < conjuncts.size(); ++c_index) {

        const VariableOrder &conjunct_variable_order =
                conjuncts[c_index]->get_variable_order();
        std::vector<std::pair<size_t, VarPosition>> previous_varpos;

        //TODO: shouldn't we be able to reference member variables easier?
        if (conjunct_variable_order == this->variable_order) {
            variable_order_index = c_index;
        }

        for (size_t vo_index = 0; vo_index < conjunct_variable_order.size(); ++vo_index) {
            unsigned variable = conjunct_variable_order[vo_index];
            // variable order has been seen before -> save first occurence pos
            if (var_positions_map.count(variable) > 0) {
                previous_varpos.push_back({vo_index,var_positions_map[variable][0]});
            }
            var_positions_map[variable].push_back({c_index, vo_index});
        }
        previous_var_positions.push_back(previous_varpos);
    }

    /*
     * If no conjunct shares our variable order, we need to get the different
     * variable values of a model from different conjuncts. This information
     * is stored in variable_order_positions.
     * It can also happen that some variables in our variable order do not occur
     * in any conjunct. In this case, we encode their current assignment in
     * an integer, and assign a dummy Variable position <conjuncts.size(),x>
     * to each variable that denotes that the value for this variable can be
     * read in the x-th bit of the integer.
     */
    if (variable_order_index < 0) {
        variable_order_positions.reserve(this->variable_order.size());
        VariableOrder missing_variables;
        for (unsigned variable : this->variable_order) {
            if (var_positions_map.count(variable) > 0) {
                variable_order_positions.push_back(var_positions_map[variable][0]);
            } else {
                missing_variables.push_back(variable);
                variable_order_positions.push_back(
                            {conjuncts.size(), missing_variables.size()-1});
            }
        }

        if (!missing_variables.empty()) {
            assignment_amount_of_new_variables = (1 << missing_variables.size());
        }
    }

    assert(!conjuncts.empty());
    assert(conjuncts.size() == previous_var_positions.size());
    assert((variable_order_positions.size() == this->variable_order.size())
           || (variable_order_index >= 0));
}

void ModsConjunction::build_variable_order_from_conjunction() {
    assert(!conjuncts.empty());
    // Collect all variables occurring in the conjunction.
    std::unordered_set<unsigned> seen_variables;
    for (const ModsFormula *conjunct : conjuncts) {
        for (unsigned variable : conjunct->get_variable_order()) {
            auto res = seen_variables.insert(variable);
            if (res.second) {
                variable_order.push_back(variable);
            }
        }
    }

    /*
     * If one conjunct covers all seen variables, use its variable order.
     * Note that it is enough to check the sizes since all variables in
     * the variable order of the conjunct must be in seen_variables.
     */
    for (const ModsFormula *conjunct : conjuncts) {
        if (conjunct->get_variable_order().size() == seen_variables.size()) {
            variable_order = conjunct->get_variable_order();
            return;
        }
    }
}

ModsConjunction::ModelIterator ModsConjunction::begin() {
    return ModelIterator(*this, false);
}

ModsConjunction::ModelIterator ModsConjunction::end() {
    return ModelIterator(*this, true);
}

const VariableOrder &ModsConjunction::get_variable_order() const {
    return variable_order;
}

void ModsConjunction::set_restriction(const Model &restriction) {
    restriction_formula = ModsFormula(restriction_variable_order,
                                      std::move(std::unordered_set<Model>({restriction})));
}


ModsConjunction::ModelIterator::ModelIterator(const ModsConjunction &conjunction,
                                              bool end)
    : conjunction(conjunction), new_variables_assignment(0) {
    if (conjunction.variable_order_index == -1) {
        model.resize(conjunction.variable_order.size(),false);
    }
    iterators.reserve(conjunction.conjuncts.size());
    if(end) {
        for (const ModsFormula * conjunct : conjunction.conjuncts) {
            iterators.push_back(conjunct->get_cend());
        }
    } else {
        for (const ModsFormula * conjunct : conjunction.conjuncts) {
            iterators.push_back(conjunct->get_cbegin());
        }
        find_consistent_model_from_here(0);
    }

}

bool ModsConjunction::ModelIterator::find_consistent_model_from_here(size_t index) {
    while(index < conjunction.conjuncts.size()) {
        while((iterators[index] != conjunction.conjuncts[index]->get_cend())
              && !check_consistency(index)) {
            iterators[index]++;
        }
        if (iterators[index] == conjunction.conjuncts[index]->get_cend()) {
            if (index == 0) {
                for(size_t i = 0; i < iterators.size(); ++i) {
                    iterators[i] = conjunction.conjuncts[i]->get_cend();
                }
                return false;
            }
            index--;
            iterators[index]++;
        } else {
            index++;
            if (index < conjunction.conjuncts.size()) {
                iterators[index] = conjunction.conjuncts[index]->get_cbegin();
            }
        }
    }
    set_model();
    return true;
}

bool ModsConjunction::ModelIterator::check_consistency(size_t index) {
    for(auto entry : conjunction.previous_var_positions[index]) {
        size_t local_pos = entry.first;
        size_t other_index = entry.second.first;
        size_t other_pos = entry.second.second;
        if (iterators[index]->at(local_pos) != iterators[other_index]->at(other_pos)) {
            return false;
        }
    }
    return true;
}

void ModsConjunction::ModelIterator::set_model() {
    if (conjunction.variable_order_index >= 0) {
        return;
    }
    for (size_t i = 0; i < conjunction.variable_order_positions.size(); ++i) {
        size_t conjunct_index = conjunction.variable_order_positions[i].first;
        size_t conjunct_pos = conjunction.variable_order_positions[i].second;
        // variable doesn't occur in conjunction
        // -> take conjunction_pos bit of new_variable_assignment
        if(conjunct_index == iterators.size()) {
            model[i] = (new_variables_assignment >> conjunct_pos)%2;
        } else {
            model[i] = iterators[conjunct_index]->at(conjunct_pos);
        }
    }
}

const Model &ModsConjunction::ModelIterator::operator *() const {
    if (conjunction.variable_order_index >= 0) {
        return *(iterators[conjunction.variable_order_index]);
    } else {
        return model;
    }
}

ModsConjunction::ModelIterator &ModsConjunction::ModelIterator::operator ++() {
    if (new_variables_assignment < conjunction.assignment_amount_of_new_variables-1) {
        new_variables_assignment++;
        set_model();
    } else {
        new_variables_assignment = 0;
        iterators[iterators.size()-1]++;
        find_consistent_model_from_here(iterators.size()-1);
    }
    return *this;
}

bool ModsConjunction::ModelIterator::operator !=(const ModelIterator &other) const {
    if (iterators.size() != other.iterators.size()) {
        return true;
    }
    if (new_variables_assignment != other.new_variables_assignment) {
        return true;
    }
    for (size_t i = 0; i < iterators.size(); ++i) {
        iterators[i];
        other.iterators[i];
        if (iterators[i] != other.iterators[i]) {
            return true;
        }
    }
    return false;
}

bool ModsConjunction::ModelIterator::operator==(const ModelIterator &other) const {
    return !(*this != other);
}
}
