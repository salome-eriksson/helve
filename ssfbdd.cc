#include "ssfbdd.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include "dddmp.h"

#include "global_funcs.h"
#include "ssfhorn.h"
#include "ssfexplicit.h"

std::unordered_map<std::vector<int>, BDDUtil, VectorHasher> BDDFile::utils;
std::vector<int> BDDFile::compose;

BDDFile::BDDFile(Task &task, std::string filename) {
    if (compose.empty()) {
        /*
         * The dumped BDDs only contain the original variables.
         * Since we need also primed variables for checking
         * statements B4 and B5 (pro/regression), we move the
         * variables in such a way that a primed variable always
         * occurs directly after its unprimed version
         * (Example: BDD dump with vars "a b c": "a a' b b' c c'")
         */
        compose.resize(task.get_number_of_facts());
        for(int i = 0; i < task.get_number_of_facts(); ++i) {
            compose[i] = 2*i;
        }

    }
    // we need to read in the file as FILE* since dddmp uses this
    fp = fopen(filename.c_str(), "r");
    if(!fp) {
        std::cerr << "could not open bdd file " << filename << std::endl;
    }

    // the first line contains the variable order, separated by space
    std::vector<int> varorder;
    varorder.reserve(task.get_number_of_facts());
    // read in line char by char into a stringstream
    std::stringstream ss;
    char c;
    while((c = fgetc (fp)) != '\n') {
        ss << c;
    }
    // read out the same line from the string stream int by int
    // TODO: what happens if it cannot interpret the next word as int?
    // TODO: can we do this more directly? Ie. get the ints while reading the first line.
    int n;
    while (ss >> n){
        varorder.push_back(n);
    }
    assert(varorder.size() == task.get_number_of_facts());
    auto it = utils.find(varorder);
    if (it == utils.end()) {
        it = utils.emplace(std::piecewise_construct,
                      std::forward_as_tuple(varorder),
                      std::forward_as_tuple(task,varorder)).first;
    }
    util = &(it->second);

}

DdNode *BDDFile::get_ddnode(int index) {
    auto it = ddnodes.find(index);
    if (it != ddnodes.end()) {

    }

    bool found = false;
    if (it != ddnodes.end()) {
        found = true;
    }
    while(!found) {
        std::vector<int> indices;
        // next line contains indices
        std::stringstream ss;
        char c;
        while((c = fgetc (fp)) != '\n') {
            ss << c;
        }
        // read out the same line from the string stream int by int
        // TODO: what happens if it cannot interpret the next word as int?
        // TODO: can we do this more directly? Ie. get the ints while reading the first line.
        int n;
        while (ss >> n){
            indices.push_back(n);
        }
        DdNode **tmp_array;
        /* read in the BDDs into an array of DdNodes. The parameters are as follows:
         *  - manager
         *  - how to match roots: we want them to be matched by id
         *  - root names: only needed when you want to match roots by name
         *  - how to match variables: since we want to permute the BDDs in order to
         *    allow primed variables in between the original one, we take COMPOSEIDS
         *  - varnames: needed if you want to match vars according to names
         *  - varmatchauxids: needed if you want to match vars according to auxidc
         *  - varcomposeids: the variable permutation if you want to permute the BDDs
         *  - mode: if the file was dumped in text or in binary mode
         *  - filename: needed if you don't directly pass the FILE *
         *  - FILE*
         *  - Pointer to array where the DdNodes should be saved to
         */
        int nRoots = Dddmp_cuddBddArrayLoad(manager.getManager(),DDDMP_ROOT_MATCHLIST,NULL,
            DDDMP_VAR_COMPOSEIDS,NULL,NULL,compose.data(),DDDMP_MODE_TEXT,NULL,fp,&tmp_array);

        assert(indices.size() == nRoots);
        for(int i = 0; i < nRoots; ++i) {
            if (indices[i] == index) {
                found = true;
                it = ddnodes.insert(std::make_pair(indices[i],tmp_array[i])).first;
            } else {
                ddnodes.insert(std::make_pair(indices[i],tmp_array[i]));
            }
        }
        // we do not need to free the memory for tmp_array, memcheck detected no leaks
    }
    DdNode *ret = it->second;
    //ddnodes.erase(it);
    return ret;
}

BDDUtil *BDDFile::get_util() {
    return util;
}

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
    for(int i = 0; i < task.get_number_of_actions(); ++i) {
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

std::unordered_map<std::string, BDDFile> SSFBDD::bddfiles;
std::vector<int> SSFBDD::prime_permutation;

SSFBDD::SSFBDD(BDDUtil *util, BDD bdd)
    : util(util), bdd(bdd) {
}

SSFBDD::SSFBDD(std::stringstream &input, Task &task) {
    std::string filename;
    int bdd_index;
    input >> filename;
    input >> bdd_index;
    if(bddfiles.find(filename) == bddfiles.end()) {
        if(bddfiles.empty()) {
            prime_permutation.resize(task.get_number_of_facts()*2, -1);
            for(int i = 0 ; i < task.get_number_of_facts(); ++i) {
              prime_permutation[2*i] = (2*i)+1;
              prime_permutation[(2*i)+1] = 2*i;
            }
        }
        bddfiles.emplace(std::piecewise_construct,
                         std::forward_as_tuple(filename),
                         std::forward_as_tuple(task, filename));
    }
    BDDFile &bddfile = bddfiles[filename];
    util = bddfile.get_util();

    assert(bdd_index >= 0);
    bdd = BDD(manager, bddfile.get_ddnode(bdd_index));
    Cudd_RecursiveDeref(manager.getManager(), bdd.getNode());


    std::string declaration_end;
    input >> declaration_end;
    assert(declaration_end == ";");
}

bool SSFBDD::check_statement_b1(std::vector<const StateSetVariable *> &left,
                                std::vector<const StateSetVariable *> &right) const {
    std::vector<SSFBDD *> left_bdds = convert_to_formalism<SSFBDD>(left, this);
    std::vector<SSFBDD *> right_bdds = convert_to_formalism<SSFBDD>(right, this);

    BDD left_singular = manager.bddOne();
    for(size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= left_bdds[i]->bdd;
    }
    BDD right_singular = manager.bddZero();
    for(size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += right_bdds[i]->bdd;
    }
    return left_singular.Leq(right_singular);
}

bool SSFBDD::check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                std::vector<const StateSetVariable *> &left,
                                std::vector<const StateSetVariable *> &right,
                                std::unordered_set<int> &action_indices) const {

    std::vector<SSFBDD *> left_bdds = convert_to_formalism<SSFBDD>(left, this);
    std::vector<SSFBDD *> right_bdds = convert_to_formalism<SSFBDD>(right, this);
    std::vector<SSFBDD *> prog_bdds = convert_to_formalism<SSFBDD>(progress, this);

    BDD left_singular = manager.bddOne();
    for (size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= left_bdds[i]->bdd;
    }
    BDD right_singular = manager.bddZero();
    for (size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += right_bdds[i]->bdd;
    }

    BDD neg_left_or_right = (!left_singular) + right_singular;

    BDD prog_singular = manager.bddOne();
    for (size_t i = 0; i < prog_bdds.size(); ++i) {
        prog_singular *= prog_bdds[i]->bdd;
    }
    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int a : action_indices) {
        const Action &action = util->task.get_action(a);
        BDD prog_rn = prog_singular * util->actionformulas[a].pre;

        for (int var = 0; var < util->task.get_number_of_facts(); ++var) {
            int local_var = util->varorder[var];
            if (action.change[var] != 0) {
                prime_permutation[2*local_var] = 2*local_var+1;
                prime_permutation[2*local_var+1] = 2*local_var;
            } else {
                prime_permutation[2*local_var] = 2*local_var;
                prime_permutation[2*local_var+1] = 2*local_var+1;
            }
        }
        prog_rn = (prog_rn.Permute(&prime_permutation[0]));
        if ( !((prog_rn* util->actionformulas[a].eff).Leq(neg_left_or_right)) ) {
            return false;
        }
    }
    return true;
}

bool SSFBDD::check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                std::vector<const StateSetVariable *> &left,
                                std::vector<const StateSetVariable *> &right,
                                std::unordered_set<int> &action_indices) const {
    std::vector<SSFBDD *> left_bdds = convert_to_formalism<SSFBDD>(left, this);
    std::vector<SSFBDD *> right_bdds = convert_to_formalism<SSFBDD>(right, this);
    std::vector<SSFBDD *> reg_bdds = convert_to_formalism<SSFBDD>(regress, this);

    BDD left_singular = manager.bddOne();
    for (size_t i = 0; i < left_bdds.size(); ++i) {
        left_singular *= left_bdds[i]->bdd;
    }
    BDD right_singular = manager.bddZero();
    for (size_t i = 0; i < right_bdds.size(); ++i) {
        right_singular += right_bdds[i]->bdd;
    }

    BDD neg_left_or_right = (!left_singular) + right_singular;

    BDD reg_singular = manager.bddOne();
    for (size_t i = 0; i < reg_bdds.size(); ++i) {
        reg_singular *= reg_bdds[i]->bdd;
    }
    if(util->actionformulas.size() == 0) {
        util->build_actionformulas();
    }

    for (int a : action_indices) {
        const Action &action = util->task.get_action(a);
        BDD reg_rn = reg_singular * util->actionformulas[a].eff;
        for (int var = 0; var < util->task.get_number_of_facts(); ++var) {
            int local_var = util->varorder[var];
            if (action.change[var] != 0) {
                prime_permutation[2*local_var] = 2*local_var+1;
                prime_permutation[2*local_var+1] = 2*local_var;
            } else {
                prime_permutation[2*local_var] = 2*local_var;
                prime_permutation[2*local_var+1] = 2*local_var+1;
            }
        }
        reg_rn = reg_rn.Permute(&prime_permutation[0]);
        if (!( (reg_rn*util->actionformulas[a].pre).Leq(neg_left_or_right) )) {
            return false;
        }
    }
    return true;
}

bool SSFBDD::check_statement_b4(const StateSetFormalism *right_const, bool left_positive, bool right_positive) const {

    if (!left_positive && !right_positive) {
        return right_const->check_statement_b4(this, true, true);
    } else if (left_positive && !right_positive) {
        if (right_const->supports_todnf() || right_const->is_nonsuccint()) {
            return right_const->check_statement_b4(this, true, false);
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }
    }

    // TODO: HACK - remove!
    StateSetFormalism *right = const_cast<StateSetFormalism *>(right_const);
    // right positive

    BDD pos_bdd = bdd;
    if (!left_positive) {
        pos_bdd = !bdd;
    }

    // left positive
    if (pos_bdd.IsZero()) {
        return true;
    }

    if (right->supports_tocnf()) {
        int count = 0;
        std::vector<int> varorder;
        std::vector<bool> clause;
        while (right->get_clause(count, varorder, clause)) {
            BDD clause_bdd = manager.bddZero();
            for (size_t i = 0; i < clause.size(); ++i) {
                if (varorder[i] > util->varorder.size()) {
                    continue;
                }
                int bdd_var = 2*util->varorder[varorder[i]];
                if (clause[i]) {
                    clause_bdd += manager.bddVar(bdd_var);
                } else {
                    clause_bdd += !(manager.bddVar(bdd_var));
                }
            }
            if(!pos_bdd.Leq(clause_bdd)) {
                return false;
            }
            count++;
        }
        return true;

    // enumerate models (only works with right if right is nonsuccinct has all vars this has)
    } else if (right->is_nonsuccint()) {
        const std::vector<int> sup_varorder = right->get_varorder();
        std::vector<bool> model(sup_varorder.size());
        std::vector<int> var_transform(util->varorder.size(), -1);
        std::vector<int> vars_to_fill_base;
        for (size_t i = 0; i < util->varorder.size(); ++i) {
            auto pos_it = std::find(sup_varorder.begin(), sup_varorder.end(), i);
            if (pos_it == sup_varorder.end()) {
                std::cerr << "mixed representation subset check not possible" << std::endl;
                return false;
            }
            var_transform[i] = std::distance(sup_varorder.begin(), pos_it);
        }
        for (size_t i = 0; i < sup_varorder.size(); ++i) {
            if(sup_varorder[i] >= util->varorder.size()) {
                vars_to_fill_base.push_back(i);
            }
        }

        //loop over all models of the BDD
        int* bdd_model;
        CUDD_VALUE_TYPE value_type;
        DdGen *cubegen = Cudd_FirstCube(manager.getManager(),pos_bdd.getNode(),&bdd_model, &value_type);
        // TODO: can the models contain don't cares?
        // Since we checked for ZeroBDD above we will always have at least 1 cube.
        do{
            std::vector<int> vars_to_fill(vars_to_fill_base);
            for (size_t i = 0; i < util->varorder.size(); ++i) {
                // TODO: implicit transformation with primed
                int cube_val = bdd_model[2*util->varorder[i]];
                if (cube_val == 1) {
                    model[var_transform[i]] = true;
                } else  if (cube_val == 0) {
                    model[var_transform[i]] = false;
                } else {
                    vars_to_fill.push_back(i);
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
        } while(Cudd_NextCube(cubegen,&bdd_model,&value_type) != 0);
        Cudd_GenFree(cubegen);
        return true;
    } else {
        std::cerr << "mixed representation subset check not possible" << std::endl;
        return false;
    }
}

const SSFBDD *SSFBDD::get_compatible(const StateSetVariable *stateset) const {
    const SSFBDD *ret = dynamic_cast<const SSFBDD *>(stateset);
    if (ret) {
        if (ret->get_varorder() == get_varorder()) {
            return ret;
        } else {
            return nullptr;
        }
    }
    const SSVConstant *cformula = dynamic_cast<const SSVConstant *>(stateset);
    if (cformula) {
        return get_constant(cformula->get_constant_type());
    }
    return nullptr;
}

const SSFBDD *SSFBDD::get_constant(ConstantType ctype) const {
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

const std::vector<int> &SSFBDD::get_varorder() const {
    return util->other_varorder;
}

bool SSFBDD::contains(const Cube &statecube) const {
    return util->build_bdd_from_cube(statecube).Leq(bdd);
}

bool SSFBDD::is_contained(const std::vector<bool> &model) const {
    assert(model.size() == util->varorder.size());
    Cube cube(util->varorder.size()*2);
    for (size_t i = 0; i < model.size(); ++i) {
        if(model[i]) {
            cube[2*i] = 1;
        } else {
            cube[2*i] = 0;
        }
    }
    BDD model_bdd(manager, Cudd_CubeArrayToBdd(manager.getManager(), &cube[0]));
    return model_bdd.Leq(bdd);

}

bool SSFBDD::is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) const {
    assert(varorder.size() == implicant.size());
    Cube cube(util->varorder.size(), 2);
    for (size_t i = 0; i < varorder.size(); ++i) {
        int var = varorder[i];
        auto pos_it = std::find(util->varorder.begin(), util->varorder.end(), var);
        if(pos_it != util->varorder.end()) {
            int var_pos = std::distance(util->varorder.begin(), pos_it);
            if (implicant[i]) {
                cube[var_pos] = 1;
            } else {
                cube[var_pos] = 0;
            }
        }
    }
    return util->build_bdd_from_cube(cube).Leq(bdd);
}

bool SSFBDD::is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const {
    assert(varorder.size() == clause.size());
    BDD clause_bdd = manager.bddZero();
    for (size_t i = 0; i < clause.size(); ++i) {
        if (varorder[i] > util->varorder.size()) {
            continue;
        }
        int bdd_var = 2*util->varorder[varorder[i]];
        if (clause[i]) {
            clause_bdd += manager.bddVar(bdd_var);
        } else {
            clause_bdd += !(manager.bddVar(bdd_var));
        }
    }
    return bdd.Leq(clause_bdd);
}

bool SSFBDD::get_clause(int i, std::vector<int> &varorder, std::vector<bool> &clause) const {
    return false;
}

int SSFBDD::get_model_count() const {
    // TODO: not sure if this is correct
    return bdd.CountMinterm(util->varorder.size());
}

StateSetBuilder<SSFBDD> bdd_builder("b");
