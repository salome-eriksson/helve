#ifndef SSFBDD_H
#define SSFBDD_H

#include "stateset.h"
#include "ssvconstant.h"

#include <unordered_map>
#include <sstream>
#include <memory>
#include <stdio.h>
#include "cuddObj.hh"

class BDDUtil;

// TODO: remove and use IntVectorHasher (identical) from global_funcs.h
struct VectorHasher {
    int operator()(const std::vector<int> &V) const {
        int hash=0;
        for(int i=0;i<V.size();i++) {
            hash+=V[i]; // Can be anything
        }
        return hash;
    }
};

class BDDFile {
private:
    static std::unordered_map<std::vector<int>, BDDUtil, VectorHasher> utils;
    static std::vector<int> compose;
    BDDUtil *util;
    FILE *fp;
    std::unordered_map<int, DdNode *> ddnodes;
public:
    BDDFile() {}
    BDDFile(Task &task, std::string filename);
    // expects caller to take ownership and call deref!
    DdNode *get_ddnode(int index);
    BDDUtil *get_util();
};

struct BDDAction {
    BDD pre;
    BDD eff;
};

class SSFBDD : public StateSetFormalism
{
    friend class BDDUtil;
private:
    static std::unordered_map<std::string, BDDFile> bddfiles;
    static std::vector<int> prime_permutation;
    // TODO: can we change this to a reference?
    BDDUtil *util;
    BDD bdd;

    SSFBDD(BDDUtil *util, BDD bdd);
public:
    SSFBDD() = delete;
    SSFBDD(std::stringstream &input, Task &task);

    virtual bool check_statement_b1(std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right) const;
    virtual bool check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) const;
    virtual bool check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) const;
    virtual bool check_statement_b4(const StateSetFormalism *right,
                                    bool left_positive, bool right_positive) const;

    virtual bool supports_mo() const { return true; }
    virtual bool supports_ce() const { return true; }
    virtual bool supports_im() const { return true; }
    virtual bool supports_me() const { return true; }
    virtual bool supports_todnf() const { return false; }
    virtual bool supports_tocnf() const { return false; }
    virtual bool supports_ct() const { return true; }
    virtual bool is_nonsuccint() const { return false; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const;
    virtual bool is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) const;
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const;
    virtual bool get_clause(int i, std::vector<int> &varorder, std::vector<bool> &clause) const;
    virtual int get_model_count() const;

    virtual const std::vector<int> &get_varorder() const;

    virtual const SSFBDD *get_compatible(const StateSetVariable *stateset) const override;
    virtual const SSFBDD *get_constant(ConstantType ctype) const override;

private:
    // used for checking statement B1 when the left side is an explicit StateSetFormalism
    bool contains(const Cube &statecube) const;

};

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
    std::vector<int> other_varorder;
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


#endif // SSFBDD_H
