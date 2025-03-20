#ifndef KNOWLEDGE_H
#define KNOWLEDGE_H

#include "global_funcs.h"

class Knowledge
{
public:
    virtual ~Knowledge() = 0;
};

template<class T>
class SubsetKnowledge : public Knowledge
{
private:
    SetID left_id;
    SetID right_id;
public:
    SubsetKnowledge(SetID left_id, SetID right_id);
    virtual ~SubsetKnowledge() {}

    SetID get_left_id() const;
    SetID get_right_id() const;
};

// no template needed - only state sets can be dead (so far...)
class DeadKnowledge : public Knowledge
{
private:
    SetID set_id;
public:
    DeadKnowledge(SetID set_id);
    virtual ~DeadKnowledge() {}

    SetID get_set_id() const;
};

class UnsolvableKnowledge : public Knowledge
{
public:
    virtual ~UnsolvableKnowledge() {}
};

class BoundKnowledge : public Knowledge
{
private:
    SetID set_id;
    unsigned bound;
public:
    BoundKnowledge(SetID set_id, unsigned bound);
    virtual ~BoundKnowledge() {}

    SetID get_set_id() const;
    unsigned get_bound() const;
};

class OptimalCostKnowledge : public Knowledge
{
private:
    unsigned lower_bound;
public:
    OptimalCostKnowledge(unsigned lower_bound);
    virtual ~OptimalCostKnowledge() {}

    unsigned get_lower_bound() const;
};

#endif // KNOWLEDGE_H
