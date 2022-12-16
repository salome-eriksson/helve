#ifndef BDDUTIL_H
#define BDDUTIL_H

#include "ssfbdd.h"

#include "../task.h"

#include <vector>

/*
 * For each BDD file, a BDDUtil instance will be created
 * which stores the variable order, all BDDs declared in the file
 * as well as all constant and action formulas in the respective
 * variable order.
 *
 * Note: action formulas are only created on demand (when the method
 * pro/regression_is_union_subset is used the first time by some
 * SSFBDD using this particual BDDUtil)
 */
class BDDUtil {
    friend class SSFBDD;
private:
    Task &task;
    // TODO: fix varorder meaning across the code!
    // global variable i is at position varorder[i](*2)
    std::vector<int> varorder;
    // bdd variable i is global variable other_varorder[i]
    std::vector<unsigned> other_varorder;
    // constant formulas
    SSFBDD emptyformula;
    SSFBDD initformula;
    SSFBDD goalformula;
    std::vector<BDDAction> actionformulas;

    BDD build_bdd_from_cube(const Cube &cube);
    //BDD build_bdd_for_action(const Action &a);
    void build_actionformulas();
public:
    BDDUtil() = delete;
    BDDUtil(Task &task, std::vector<int> &varorder);
};

#endif // BDDUTIL_H
