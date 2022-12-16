#ifndef HORNTYPEDISJUNCTION_H
#define HORNTYPEDISJUNCTION_H

#include "cnfformula.h"

#include <vector>

namespace cnf {
class Disjunction {
private:
    unsigned varamount;
    std::vector<const CNFFormula *> disjuncts;
public:
    Disjunction(const std::vector<const CNFFormula *> &formulas);
    class ClauseIterator {
    private:
        const Disjunction &disjunction;
        std::vector<CNFFormula::ClauseIterator> iterators;
        Clause clause;

        /*
         * Explicitly builds the clause which the iterators point to.
         * Returns false if the clause is valid, in which case we should
         * continue incresing the iterator vector.
         */
        bool build_clause();
        /*
         * Returns true if the iterators point to a valid position, or
         * false if all iterators point to end().
         */
        bool increase_iterator_vector();
    public:
        ClauseIterator(const Disjunction &disjunction, bool end);
        const Clause &operator *() const;
        /*
         * Increase the iterator vector until a non-valid clause is found.
         */
        ClauseIterator &operator++();
        bool operator!=(const ClauseIterator &other) const;
        bool operator==(const ClauseIterator &other) const;
    };
    ClauseIterator begin() const;
    ClauseIterator end() const;

    unsigned get_varamount() const;
};
}

#endif // HORNTYPEDISJUNCTION_H
