#include "hornstateset.h"

#include "taskinformation.h"

#include <algorithm> //TODO: remove after rewriting B4
#include <cassert>

namespace horn {
HornStateSet::HornStateSet(std::stringstream &input, const Task &task)
    : formula(input, false),
      task_information(TaskInformation::get_task_information(task)) {
}

std::vector<Literal> HornStateSet::get_precondition_literals(const Action &action) const {
    std::vector<Literal> ret;
    for (int precondition: action.pre) {
        ret.push_back({(unsigned) precondition, true});
    }
    return ret;
}

std::vector<Literal> HornStateSet::get_effect_literals(const Action &action) const {
    std::vector<Literal> ret;
    for (size_t i = 0; i < action.change.size(); ++i) {
        if (action.change[i] == 1) {
            ret.push_back({i, true});
        } else if (action.change[i] == -1) {
            ret.push_back({i, false});
        }
    }
    return ret;
}


HornStateSet::Disjunction::Disjunction(
        const std::vector<const HornTypeFormula *> &&formulas)
    : varamount(0) {
    disjuncts.reserve(formulas.size());
    for (const HornTypeFormula *formula : formulas) {
        // An empty formula is valid -> the entire disjunction is valid.
        if (formula->has_no_clauses()) {
            disjuncts.clear();
            disjuncts.push_back(formula);
            return;
        }

        // A formula with an empty clause is unsatisfiable -> can be ignored.
        if (!formula->contains_empty_clause()) {
            disjuncts.push_back(formula);
            varamount = std::max(varamount, formula->get_varamount());
        }
    }

    if (disjuncts.size() == 0) {
        disjuncts.push_back(&(HornTypeFormula::get_unsatisfiable_formula(false)));
        varamount = disjuncts[0]->get_varamount();
    }

    assert(disjuncts.size() > 0);
}

/*
 * Increase the vector of iterators to the next position. We increase the
 * latest iterator first, and if it hits max size go to the previous one.
 *
 * Example: 5 Iterators each with 8 elements, current position 2 7 4 7 7
 * -> next position: 2 7 5 0 0
 */
bool HornStateSet::Disjunction::ClauseIterator::increase_iterator_vector() {
    size_t pos = iterators.size();
    do {
        if (pos == 0) {
            return false;
        }
        ++iterators[--pos];
    } while (iterators[pos] == disjunction.disjuncts[pos]->end());
    while (pos < iterators.size()-1) {
        ++pos;
        iterators[pos].reset();
    }
    return true;
}

bool HornStateSet::Disjunction::ClauseIterator::build_clause() {
    clause.clear();
    for(HornTypeFormula::ClauseIterator it : iterators) {
        for (Literal literal : *it) {
            auto result = clause.insert(literal);
            // Negated literal already already occurs in clause -> valid.
            if (!result.second && result.first->second != literal.second) {
                return false;
            }
        }
    }
    return true;
}

HornStateSet::Disjunction::ClauseIterator::ClauseIterator(
        const Disjunction &disjunction, bool end)
    : disjunction(disjunction) {
    iterators.reserve(disjunction.disjuncts.size());
    for (const HornTypeFormula *formula : disjunction.disjuncts) {
        if (end) {
            iterators.push_back(formula->end());
        } else {
            iterators.push_back(formula->begin());
        }
    }
    if (!end) { //make begin() point to the first non-valid clause.
        /*
         * If the initial position results in a valid clause,
         * operator++ will find the next non-valid clause.
         */
        if(!build_clause()) {
            operator++();
        }
    }
}

const Clause &HornStateSet::Disjunction::ClauseIterator::operator *() const {
    return clause;
}

HornStateSet::Disjunction::ClauseIterator &HornStateSet::Disjunction::ClauseIterator::operator ++() {
    bool non_valid_clause_found = false;
    do {
        bool could_increase = increase_iterator_vector();
        if(!could_increase) {
            clause.clear();
            break;
        }
        non_valid_clause_found = build_clause();
    } while (!non_valid_clause_found);
    return *this;
}

bool HornStateSet::Disjunction::ClauseIterator::operator !=(
        const ClauseIterator &other) const {
    if (iterators.size() != other.iterators.size()) {
        return true;
    }
    for (size_t i = 0; i < iterators.size(); ++i) {
        if (iterators[i] != other.iterators[i]) {
            return true;
        }
    }
    return false;
}

bool HornStateSet::Disjunction::ClauseIterator::operator ==(
        const ClauseIterator &other) const {
    return !(*this != other);
}


HornStateSet::Disjunction::ClauseIterator HornStateSet::Disjunction::begin() const {
    return ClauseIterator(*this, false);
}

HornStateSet::Disjunction::ClauseIterator HornStateSet::Disjunction::end() const {
    return ClauseIterator(*this, true);
}

unsigned HornStateSet::Disjunction::get_varamount() const {
    return varamount;
}

std::vector<const HornTypeFormula *> HornStateSet::get_formulas(
        const std::vector<const HornStateSet *> &state_sets) {
    std::vector<const HornTypeFormula *>formulas;
    formulas.reserve(state_sets.size());
    for (const HornStateSet *state_set : state_sets) {
        formulas.push_back(&(state_set->formula));
    }
    return formulas;
}


bool HornStateSet::check_statement_b1(std::vector<const StateSetVariable *> &left,
                                      std::vector<const StateSetVariable *> &right) const {
    std::vector<const HornStateSet *> left_horn = const_convert_to_formalism(left, this);
    std::vector<const HornStateSet *> right_horn = const_convert_to_formalism(right, this);


    HornTypeFormula left_formula(std::move(get_formulas(left_horn)), false);
    // If left is the empty set, then left is a subset of anything.
    if (left_formula.contains_empty_clause()) {
        return true;
    }

    Disjunction right_disjunction(std::move(get_formulas(right_horn)));

    // phi implies (/\ c_i) is true iff phi implies c_i for each c_i.
    for (const Clause &clause : right_disjunction) {
        if (!left_formula.entails(clause)) {
            return false;
        }
    }
    return true;
}

bool HornStateSet::check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                      std::vector<const StateSetVariable *> &left,
                                      std::vector<const StateSetVariable *> &right,
                                      std::unordered_set<size_t> &action_indices) const {
    assert(!progress.empty());
    std::vector<const HornStateSet *> progress_horn =
            const_convert_to_formalism(progress, this);
    std::vector<const HornStateSet *> left_horn =
            const_convert_to_formalism(left, this);
    std::vector<const HornStateSet *> right_horn =
            const_convert_to_formalism(right, this);

    // We introduce fresh variables and thus need to know the total varamount.
    unsigned varamount = 0;

    HornTypeFormula progress_formula(std::move(get_formulas(progress_horn)), false);
    // Progressing the empty set with anything result in empty set.
    // -> emptyset \cap X \subseteq Y is always true.
    if (progress_formula.contains_empty_clause()) {
        return true;
    }
    varamount = std::max(varamount, progress_formula.get_varamount());

    HornTypeFormula left_formula(std::move(get_formulas(left_horn)), false);
    // If left is the empty set, then X \cap left \subseteq Y is always true.
    if  (left_formula.contains_empty_clause()) {
        return true;
    }
    varamount = std::max(varamount, left_formula.get_varamount());

    Disjunction right_disjunction(std::move(get_formulas(right_horn)));
    // If the conjunction of clauses representing the disjunction is empty,
    // the disjunction is valid.
    if (right_disjunction.begin() == right_disjunction.end()) {
        return true;
    }
    varamount = std::max(varamount, right_disjunction.get_varamount());

    for(size_t action_index : action_indices) {
        const Action &action = task_information.get_task().get_action(action_index);
        unsigned fresh_variable = varamount;

        /*
         * Build transition formula and rename map for the progressed formula.
         * All variables that occur in the effect get renamed in the progressed
         * formula, and the transition formula uses renamed variables for the
         * preconditions (if the variable occurs in the effect).
         */
        std::vector<std::pair<unsigned, unsigned>> renames;
        PartialAssignment transition;
        for (Literal effect : get_effect_literals(action)) {
            renames.push_back({effect.first, fresh_variable});
            transition[effect.first] = effect.second;
            fresh_variable++;
        }
        for (Literal precondition : get_precondition_literals(action)) {
            if (transition.count(precondition.first) > 0) {
                unsigned renamed_var = -1;
                for (std::pair<unsigned,unsigned> rename : renames) {
                    if(rename.first == precondition.first) {
                        renamed_var = rename.second;
                        break;
                    }
                }
                assert(renamed_var >= 0);
                transition[renamed_var] = precondition.second;
            } else {
                transition[precondition.first] = precondition.second;
            }
        }

        HornTypeFormula progress_renamed(progress_formula);
        progress_renamed.rename(renames);
        HornTypeFormula transition_formula(transition, false);
        HornTypeFormula left_combined({&progress_renamed, &transition_formula,
                                       &left_formula}, false);

        for (const Clause &clause : right_disjunction) {
            if (!left_combined.entails(clause)) {
                return false;
            }
        }
    }
    return true;
}

bool HornStateSet::check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                      std::vector<const StateSetVariable *> &left,
                                      std::vector<const StateSetVariable *> &right,
                                      std::unordered_set<size_t> &action_indices) const {
    assert(!regress.empty());
    std::vector<const HornStateSet *> regress_horn =
            const_convert_to_formalism(regress, this);
    std::vector<const HornStateSet *> left_horn =
            const_convert_to_formalism(left, this);
    std::vector<const HornStateSet *> right_horn =
            const_convert_to_formalism(right, this);

    // We introduce fresh variables and thus need to know the total varamount.
    unsigned varamount = 0;

    HornTypeFormula regress_formula(std::move(get_formulas(regress_horn)), false);
    // Regressing the empty set with anything result in empty set.
    // -> emptyset \cap X \subseteq Y is always true.
    if (regress_formula.contains_empty_clause()) {
        return true;
    }
    varamount = std::max(varamount, regress_formula.get_varamount());

    HornTypeFormula left_formula(std::move(get_formulas(left_horn)), false);
    // If left is the empty set, then X \cap left \subseteq Y is always true.
    if  (left_formula.contains_empty_clause()) {
        return true;
    }
    varamount = std::max(varamount, left_formula.get_varamount());

    Disjunction right_disjunction(std::move(get_formulas(right_horn)));
    // If the conjunction of clauses representing the disjunction is empty,
    // the disjunction is valid.
    if (right_disjunction.begin() == right_disjunction.end()) {
        return true;
    }
    varamount = std::max(varamount, right_disjunction.get_varamount());

    for(size_t action_index : action_indices) {
        const Action &action = task_information.get_task().get_action(action_index);
        unsigned fresh_variable = varamount;

        /*
         * Build transition formula and rename map for the regressed formula.
         * All variables that occur in the effect get renamed in the regressed
         * formula, and the transition formula uses renamed variables for the
         * effects.
         */
        std::vector<std::pair<unsigned, unsigned>> renames;
        PartialAssignment transition;
        for (Literal effect : get_effect_literals(action)) {
            renames.push_back({effect.first, fresh_variable});
            transition[fresh_variable] = effect.second;
            fresh_variable++;
        }
        for (Literal precondition : get_precondition_literals(action)) {
            transition[precondition.first] = precondition.second;
        }

        HornTypeFormula regress_renamed(regress_formula);
        regress_renamed.rename(renames);
        HornTypeFormula transition_formula(transition, false);
        HornTypeFormula left_combined({&regress_renamed, &transition_formula,
                                       &left_formula}, false);

        for (const Clause &clause : right_disjunction) {
            if (!left_combined.entails(clause)) {
                return false;
            }
        }
    }
    return true;
}

// TODO: reimplement
bool HornStateSet::check_statement_b4(const StateSetFormalism *right_const, bool left_positive, bool right_positive) const {
    // TODO: HACK - remove!
    StateSetFormalism *right = const_cast<StateSetFormalism *>(right_const);

    std::vector<int> varorder(formula.get_varamount(), -1);
    for (size_t i = 0; i < varorder.size(); ++i) {
        varorder[i] = i;
    }

    if (left_positive && right_positive) {
        if (right->supports_tocnf()) {
            int count = 0;
            std::vector<int> varorder;
            std::vector<bool> clause;
            while (right->get_clause(count, varorder, clause)) {
                if (!is_entailed(varorder, clause)) {
                    return false;
                }
                count++;
            }
            return true;
        } else if (right->is_nonsuccint()) {
            const std::vector<int> &sup_varorder = right->get_varorder();
            std::vector<bool> model(sup_varorder.size());
            std::vector<int> var_transform(varorder.size(), -1);
            std::vector<int> vars_to_fill;
            for (size_t i = 0; i < varorder.size(); ++i) {
                auto pos_it = std::find(sup_varorder.begin(), sup_varorder.end(), varorder[i]);
                if (pos_it == sup_varorder.end()) {
                    throw std::runtime_error("mixed representation subset check not possible");
                }
                var_transform[i] = std::distance(sup_varorder.begin(), pos_it);
            }
            for (size_t i = 0; i < sup_varorder.size(); ++i) {
                if(sup_varorder[i] >= varorder.size()) {
                    vars_to_fill.push_back(i);
                }
            }

            std::vector<bool> mark(varorder.size(),false);
            std::vector<int> old_solution(varorder.size(),2);
            PartialAssignment partial_assignment;
            std::vector<const CNFFormula *>vec({&formula});

            bool solution_found = CNFFormula::unit_propagation(vec, partial_assignment);
            while(solution_found) {
                for(size_t i = 0; i < old_solution.size(); ++i) {
                    if(old_solution[i] == 2) {
                        old_solution[i] = 0;
                    }
                }

                for (size_t i = 0; i < varorder.size(); ++i) {
                    if (old_solution[i] == 1) {
                        model[var_transform[i]] = true;
                    } else {
                        model[var_transform[i]] = false;
                    }
                }

                for (int count = 0; count < (1 << vars_to_fill.size()); ++count) {
                    for (size_t i = 0; i < vars_to_fill.size(); ++i) {
                        model[vars_to_fill[i]] = ((count >> i) % 2 == 1);
                    }
                    if (!right->is_contained(model)) {
                        return false;
                    }
                }

                // get next solution
                solution_found = false;
                for(size_t i = varorder.size()-1; i >= 0; --i) {
                    if (!mark[i]) {
                        old_solution[i] = 1 - old_solution[i];
                        partial_assignment.clear();
                        for (size_t i = 0; i < old_solution.size(); ++i) {
                            if (old_solution[i] == 1) {
                                partial_assignment[i] = true;
                            } else if (old_solution[i] == 0) {
                                partial_assignment[i] = false;
                            }
                        }
                        if (CNFFormula::unit_propagation(vec, partial_assignment)) {
                            solution_found = true;
                            mark[i] = true;
                            for (int j = i+1; j < mark.size(); ++j) {
                                mark[j] = false;
                            }
                            break;
                        }
                    } else {
                        old_solution[i] = 2;
                    }
                }
            }
            return true;
        } else {
            throw std::runtime_error("mixed representation subset check not possible");
            return false;
        }
    } else if (left_positive && !right_positive) {
        if (right->supports_todnf()) {
            return right->check_statement_b4(this, true, false);
        } else if (right->is_nonsuccint()) {
            return right->check_statement_b4(this, true, false);
        } else {
            throw std::runtime_error("mixed representation subset check not possible");
            return false;
        }
    } else if (!left_positive && right_positive) {
        if (right->supports_im()) {
            std::vector<int> vars(0);
            vars.reserve(formula.get_varamount());
            std::vector<bool> implicant(0);
            implicant.reserve(formula.get_varamount());

            for (const Clause &clause : formula) {
                vars.clear();
                implicant.clear();

                for (Literal literal : clause) {
                    vars.push_back(literal.first);
                    implicant.push_back(!literal.second);
                }

                if (!right->is_implicant(vars, implicant)) {
                    return false;
                }
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
}

const HornStateSet *HornStateSet::get_compatible(const StateSetVariable *stateset) const {
    const HornStateSet *ret = dynamic_cast<const HornStateSet *>(stateset);
    if (ret) {
        return ret;
    }
    const SSVConstant *cformula = dynamic_cast<const SSVConstant *>(stateset);
    if (cformula) {
        return get_constant(cformula->get_constant_type());
    }
    return nullptr;
}

const HornStateSet *HornStateSet::get_constant(ConstantType ctype) const {
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

std::string HornStateSet::print() {
    return formula.print();
}

// TODO: remove all these once B4 is overhauled
std::vector<int> get_increasing_vector(size_t size) {
    std::vector<int> ret(size, -1);
    for (size_t i = 0; i < size; ++i) {
        ret[i] = i;
    }
    return ret;
}

const std::vector<int> &HornStateSet::get_varorder() const {
    static std::vector<int> varorder(get_increasing_vector(formula.get_varamount()));
    return varorder;
}

bool HornStateSet::is_contained(const std::vector<bool> &model) const {
    PartialAssignment partial_assignment;
    for (size_t i = 0; i < model.size(); ++i) {
        partial_assignment[i] = model[i];
    }
    std::vector<const CNFFormula *> vec({&formula});
    return CNFFormula::unit_propagation(vec, partial_assignment);
}

bool HornStateSet::is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) const {
    PartialAssignment left_pa;
    for (size_t i = 0; i < varorder.size(); ++i) {
        left_pa[varorder[i]] = implicant[i];
    }
    HornTypeFormula left(left_pa, false);

    for (const Clause &clause : formula) {
        PartialAssignment right_pa;
        for (Literal literal : clause) {
            right_pa[literal.first] = !literal.second;
        }
        if (CNFFormula::unit_propagation({&left}, right_pa)) {
            return false;
        }
    }
    return true;
}

bool HornStateSet::is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const {
    PartialAssignment partial_assignment;
    for (size_t i = 0; i < clause.size(); ++i) {
        partial_assignment[varorder[i]] = !clause[i];
    }
    return formula.entails(partial_assignment);
}

bool HornStateSet::get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) const {
    int count = 0;
    vars.clear();
    clause.clear();
    for (const Clause &c : formula) {
        if (count == i) {
            for (Literal literal : c) {
                vars.push_back(literal.first);
                clause.push_back(literal.second);
                return true;
            }
        }
    }
    return false;
}

int HornStateSet::get_model_count() const {
    throw std::runtime_error("Horn state set does not support model count");
}

}

StateSetBuilder<horn::HornStateSet> horn_builder("h");
