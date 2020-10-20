#ifndef ACTIONSET_H
#define ACTIONSET_H

#include "setcompositions.h"
#include "task.h"

#include <deque>
#include <memory>
#include <unordered_set>

class ActionSet
{
public:
    ActionSet();
    virtual ~ActionSet() {}
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices) const = 0;
    virtual bool is_constantall() const = 0;
};

class ActionSetBasic : public ActionSet
{
private:
    std::unordered_set<int> action_indices;
public:
    ActionSetBasic(std::unordered_set<int> &action_indices);
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices) const;
    virtual bool is_constantall() const;
};

class ActionSetUnion : public ActionSet, public SetUnion
{
private:
    int id_left;
    int id_right;
public:
    ActionSetUnion(int id_left, int id_right);
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices) const;
    virtual bool is_constantall() const;
    virtual int get_left_id() const;
    virtual int get_right_id() const;
};

class ActionSetConstantAll : public ActionSet
{
    std::unordered_set<int> action_indices;
public:
    ActionSetConstantAll(Task &task);
    virtual void get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices) const;
    virtual bool is_constantall() const;
};

#endif // ACTIONSET_H
