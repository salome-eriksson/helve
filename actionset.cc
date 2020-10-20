#include "actionset.h"

#include <assert.h>

ActionSet::ActionSet()
{

}

ActionSetBasic::ActionSetBasic(std::unordered_set<int> &action_indices)
    : action_indices(action_indices) {

}
void ActionSetBasic::get_actions(const std::deque<std::unique_ptr<ActionSet>> &, std::unordered_set<int> &indices) const {
    indices.insert(action_indices.begin(), action_indices.end());
}
bool ActionSetBasic::is_constantall() const {
    return false;
}

ActionSetUnion::ActionSetUnion(int id_left, int id_right)
    : id_left(id_left), id_right(id_right) {}

void ActionSetUnion::get_actions(const std::deque<std::unique_ptr<ActionSet>> &action_sets, std::unordered_set<int> &indices) const {
    assert(id_left < action_sets.size() && id_right < action_sets.size());
    action_sets[id_left]->get_actions(action_sets, indices);
    action_sets[id_right]->get_actions(action_sets, indices);
}
bool ActionSetUnion::is_constantall() const {
    return false;
}
int ActionSetUnion::get_left_id() const {
    return id_left;
}
int ActionSetUnion::get_right_id() const {
    return id_right;
}

ActionSetConstantAll::ActionSetConstantAll(Task &task) {
    for (int i = 0; i < task.get_number_of_actions(); ++i) {
        action_indices.insert(i);
    }
}
void ActionSetConstantAll::get_actions(const std::deque<std::unique_ptr<ActionSet>> &, std::unordered_set<int> &indices) const {
    indices.insert(action_indices.begin(), action_indices.end());
}
bool ActionSetConstantAll::is_constantall() const {
    return true;
}
