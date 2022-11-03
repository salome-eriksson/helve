#ifndef ACTIONSET_H
#define ACTIONSET_H

#include "setcompositions.h"
#include "task.h"

#include <unordered_set>

class ProofChecker;

class ActionSet
{
public:
    ActionSet();
    virtual ~ActionSet() {}
    virtual void get_actions(const ProofChecker &proof_checker,
                             std::unordered_set<size_t> &indices) const = 0;
    virtual bool is_constantall() const = 0;
    unsigned get_min_cost(const ProofChecker &proof_checker) const;
};

class ActionSetBasic : public ActionSet
{
private:
    std::unordered_set<size_t> action_indices;
public:
    ActionSetBasic(std::unordered_set<size_t> &action_indices);
    virtual void get_actions(const ProofChecker &proof_checker,
                             std::unordered_set<size_t> &indices) const override;
    virtual bool is_constantall() const override;
};

class ActionSetUnion : public ActionSet, public SetUnion
{
private:
    SetID id_left;
    SetID id_right;
public:
    ActionSetUnion(SetID id_left, SetID id_right);
    virtual void get_actions(const ProofChecker &action_sets,
                             std::unordered_set<size_t> &indices) const override;
    virtual bool is_constantall() const override;
    virtual SetID get_left_id() const override;
    virtual SetID get_right_id() const override;
};

class ActionSetConstantAll : public ActionSet
{
    std::unordered_set<int> action_indices;
public:
    ActionSetConstantAll(Task &task);
    virtual void get_actions(const ProofChecker &proof_checker,
                             std::unordered_set<size_t> &indices) const override;
    virtual bool is_constantall() const override;
};

#endif /* ACTIONSET_H */
