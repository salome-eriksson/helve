#include "actionset.h"

#include "proofchecker.h"

#include <cassert>

ActionSet::ActionSet()
{

}

ActionSetBasic::ActionSetBasic(std::unordered_set<size_t> &action_indices)
    : action_indices(action_indices) {

}
void ActionSetBasic::get_actions(const ProofChecker &,
                                 std::unordered_set<size_t> &indices) const {
    indices.insert(action_indices.begin(), action_indices.end());
}
bool ActionSetBasic::is_constantall() const {
    return false;
}

ActionSetUnion::ActionSetUnion(SetID id_left, SetID id_right)
    : id_left(id_left), id_right(id_right) {}

void ActionSetUnion::get_actions(const ProofChecker &proof_checker,
                                 std::unordered_set<size_t> &indices) const {
    proof_checker.get_set<ActionSet>(id_left)->get_actions(proof_checker, indices);
    proof_checker.get_set<ActionSet>(id_right)->get_actions(proof_checker, indices);
}
bool ActionSetUnion::is_constantall() const {
    return false;
}
SetID ActionSetUnion::get_left_id() const {
    return id_left;
}
SetID ActionSetUnion::get_right_id() const {
    return id_right;
}

ActionSetConstantAll::ActionSetConstantAll(Task &task) {
    for (int i = 0; i < task.get_number_of_actions(); ++i) {
        action_indices.insert(i);
    }
}
void ActionSetConstantAll::get_actions(const ProofChecker &,
                                       std::unordered_set<size_t> &indices) const {
    indices.insert(action_indices.begin(), action_indices.end());
}
bool ActionSetConstantAll::is_constantall() const {
    return true;
}
