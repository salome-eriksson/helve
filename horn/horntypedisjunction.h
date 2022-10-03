#ifndef HORNTYPEDISJUNCTION_H
#define HORNTYPEDISJUNCTION_H

#include "horntypeformula.h"

#include <vector>

namespace horn {
class HornTypeDisjunction {
private:
    unsigned varamount;
    std::vector<const HornTypeFormula *> disjuncts;
public:
    HornTypeDisjunction(const std::vector<const HornTypeFormula *> &&formulas);
    class ClauseIterator {
    private:
        const HornTypeDisjunction &disjunction;
        std::vector<HornTypeFormula::ClauseIterator> iterators;
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
        ClauseIterator(const HornTypeDisjunction &disjunction, bool end);
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
