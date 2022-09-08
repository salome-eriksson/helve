#ifndef SSFBDD_H
#define SSFBDD_H

#include "../stateset.h"
#include "../task.h"

#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <vector>
#include "cuddObj.hh"

class BDDFile;
class BDDUtil;

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
                                    std::unordered_set<size_t> &action_indices) const;
    virtual bool check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<size_t> &action_indices) const;
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

#endif // SSFBDD_H
