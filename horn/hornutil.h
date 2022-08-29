#ifndef HORNUTIL_H
#define HORNUTIL_H

#include "../task.h"

#include <forward_list>
#include <vector>

class SSFHorn;

struct HornConjunctionElement {
    const SSFHorn *formula;
    std::vector<bool> removed_implications;

    HornConjunctionElement(const SSFHorn *formula);
};

class HornUtil {
    friend class SSFHorn;
private:
    Task &task;
    SSFHorn *emptyformula;
    SSFHorn *initformula;
    SSFHorn *goalformula;
    SSFHorn *trueformula;
    HornUtil(Task &task);

    void build_actionformulas();

    /*
     * Perform unit propagation through the conjunction of the formulas, returning whether it is satisfiable or not.
     * Partial assignment serves both as input of a partial assignment, and as output indicating which variables
     * are forced true/false by the conjunction.
     */
    bool simplify_conjunction(std::vector<HornConjunctionElement> &conjuncts, Cube &partial_assignment);
    bool is_restricted_satisfiable(const SSFHorn *formula, Cube &restriction);

    bool conjunction_implies_disjunction(std::vector<SSFHorn *> &conjuncts,
                                         std::vector<SSFHorn *> &disjuncts);
};


#endif // HORNUTIL_H
