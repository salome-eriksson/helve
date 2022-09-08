#ifndef STATESETCOMPOSITIONS_H
#define STATESETCOMPOSITIONS_H

#include "setcompositions.h"
#include "stateset.h"

#include <deque>
#include <memory>
#include <sstream>
#include <vector>

/*
 * COLLECTION OF ALL STATESET CLASSES THAT DERIVE FROM
 * SOME INTERFACE FROM SETCOMPOSITONS
 */

class StateSetUnion : public StateSet, public SetUnion
{
private:
    SetID id_left;
    SetID id_right;
public:
    StateSetUnion(std::stringstream &input, Task &task);
    virtual ~StateSetUnion() {}

    virtual SetID get_left_id() const;
    virtual SetID get_right_id() const;
    virtual bool gather_union_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const override;
};

class StateSetIntersection : public StateSet, public SetIntersection
{
private:
    SetID id_left;
    SetID id_right;
public:
    StateSetIntersection(std::stringstream &input, Task &task);
    virtual ~StateSetIntersection() {}

    virtual SetID get_left_id() const;
    virtual SetID get_right_id() const;
    virtual bool gather_intersection_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const override;
};

class StateSetNegation : public StateSet, public SetNegation
{
private:
    int id_child;
public:
    StateSetNegation(std::stringstream &input, Task &task);
    virtual ~StateSetNegation() {}

    virtual SetID get_child_id() const;
    virtual bool gather_union_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const override;
    virtual bool gather_intersection_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const override;
};

class StateSetProgression : public StateSet
{
private:
    SetID id_stateset;
    SetID id_actionset;
public:
    StateSetProgression(std::stringstream &input, Task &task);
    virtual ~StateSetProgression() {}

    virtual SetID get_stateset_id() const;
    virtual SetID get_actionset_id() const;
};

class StateSetRegression : public StateSet
{
private:
    SetID id_stateset;
    SetID id_actionset;
public:
    StateSetRegression(std::stringstream &input, Task &task);
    virtual ~StateSetRegression() {}

    virtual SetID get_stateset_id() const;
    virtual SetID get_actionset_id() const;
};


#endif // STATESETCOMPOSITIONS_H
