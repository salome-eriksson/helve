#include "bddutil.h"

#include "../global_funcs.h"

#include <cassert>

BDDUtil::BDDUtil(Task &task, std::vector<int> &varorder)
    : task(task), varorder(varorder),
      initformula(this, build_bdd_from_cube(task.get_initial_state())),
      goalformula(this, build_bdd_from_cube(task.get_goal())),
      emptyformula(this, BDD(manager.bddZero())) {

    assert(varorder.size() == task.get_number_of_facts());
    other_varorder.resize(varorder.size(),-1);
    for (size_t i = 0; i < varorder.size(); ++i) {
        other_varorder[varorder[i]] = i;
    }
}

// TODO: returning a new object is not a very safe way (see for example contains())
BDD BDDUtil::build_bdd_from_cube(const Cube &cube) {
    std::vector<int> local_cube(cube.size()*2,2);
    for(size_t i = 0; i < cube.size(); ++i) {
        //permute both accounting for primed variables and changed var order
        local_cube[varorder[i]*2] = cube[i];
    }
    return BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), &local_cube[0]));
}

void BDDUtil::build_actionformulas() {
    actionformulas.reserve(task.get_number_of_actions());
    for(size_t i = 0; i < task.get_number_of_actions(); ++i) {
        BDDAction bddaction;
        const Action &action = task.get_action(i);
        bddaction.pre = manager.bddOne();
        for (int var : action.pre) {
            bddaction.pre *= manager.bddVar(varorder[var]*2);
        }
        bddaction.eff = manager.bddOne();
        for (size_t var = 0; var < action.change.size(); ++var) {
            if (action.change[var] == 0) {
                continue;
            } else if (action.change[var] == 1) {
                bddaction.eff *= manager.bddVar(varorder[var]*2);
            } else {
                bddaction.eff *= !manager.bddVar(varorder[var]*2);
            }
        }
        actionformulas.push_back(bddaction);
    }
}
