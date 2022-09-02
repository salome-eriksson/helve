#ifndef SETCOMPOSITIONS_H
#define SETCOMPOSITIONS_H

#include "global_funcs.h"

class SetUnion
{
public:
    virtual ~SetUnion() {}
    virtual SetID get_left_id() const = 0;
    virtual SetID get_right_id() const = 0;
};

class SetIntersection
{
public:
    virtual ~SetIntersection() {}
    virtual SetID get_left_id() const = 0;
    virtual SetID get_right_id() const = 0;
};

class SetNegation
{
public:
    virtual ~SetNegation() {}
    virtual SetID get_child_id() const = 0;
};

#endif // SETCOMPOSITIONS_H
