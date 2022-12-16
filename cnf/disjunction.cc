#include "disjunction.h"

#include <cassert>

namespace cnf {
Disjunction::Disjunction(const std::vector<const CNFFormula *> &formulas)
    : varamount(0) {
    disjuncts.reserve(formulas.size());
    for (const CNFFormula *formula : formulas) {
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
        disjuncts.push_back(&(CNFFormula::get_unsatisfiable_formula()));
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
bool Disjunction::ClauseIterator::increase_iterator_vector() {
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

bool Disjunction::ClauseIterator::build_clause() {
    clause.clear();
    for(CNFFormula::ClauseIterator it : iterators) {
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

Disjunction::ClauseIterator::ClauseIterator(
        const Disjunction &disjunction, bool end)
    : disjunction(disjunction) {
    iterators.reserve(disjunction.disjuncts.size());
    for (const CNFFormula *formula : disjunction.disjuncts) {
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

const Clause &Disjunction::ClauseIterator::operator *() const {
    return clause;
}

Disjunction::ClauseIterator &Disjunction::ClauseIterator::operator ++() {
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

bool Disjunction::ClauseIterator::operator !=(
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

bool Disjunction::ClauseIterator::operator ==(
        const ClauseIterator &other) const {
    return !(*this != other);
}


Disjunction::ClauseIterator Disjunction::begin() const {
    return ClauseIterator(*this, false);
}

Disjunction::ClauseIterator Disjunction::end() const {
    return ClauseIterator(*this, true);
}

unsigned Disjunction::get_varamount() const {
    return varamount;
}
}
