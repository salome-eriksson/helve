#include "basic_statement_functions.h"
#include "../rules.h"

#include "../../statesetcompositions.h"

namespace rules{
// B2: (\bigcap_{X \in \mathcal X} X)[A] \land \bigcap_{L \in \mathcal L} L
//     \subseteq \bigcup_{L' \in \mathcal L'} L'
std::unique_ptr<Knowledge> basic_statement_2(SetID left_id, SetID right_id,
                                             std::vector<KnowledgeID> &,
                                             const ProofChecker &proof_checker) {

    std::vector<const StateSetVariable *> prog;
    std::vector<const StateSetVariable *> left;
    std::vector<const StateSetVariable *> right;
    std::unordered_set<size_t> actions;
    const StateSet *progressed_set = nullptr;
    const ActionSet *action_set = nullptr;
    const StateSet *left_set = nullptr;
    const StateSet *right_set = proof_checker.get_set<StateSet>(right_id);

    /*
     * We expect the left side to either be a progression
     * or an intersection with a progression on the left side
     */
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(proof_checker.get_set<StateSet>(left_id));
    if (!progression) { // Left side is not a progression, is it an intersection?
        const StateSetIntersection *intersection =
                dynamic_cast<const StateSetIntersection *>(proof_checker.get_set<StateSet>(left_id));
        if (!intersection) {
            throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                     + " is not a progression or intersection.");
        }
        const StateSet *intersection_left =
                proof_checker.get_set<StateSet>(intersection->get_left_id());
        progression =
                dynamic_cast<const StateSetProgression *>(intersection_left);
        if (!progression) {
            throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                     + " is an interesction but not with a"
                                     " progression on the left side");
        }
        left_set = proof_checker.get_set<StateSet>(intersection->get_right_id());
    }
    action_set = proof_checker.get_set<ActionSet>(progression->get_actionset_id());
    action_set->get_actions(proof_checker, actions);
    progressed_set = proof_checker.get_set<StateSet>(progression->get_stateset_id());

    bool valid_syntax =
            progressed_set->gather_intersection_variables(proof_checker,
                                                          prog, right);
    /*
     * In the progression, we only allow set variables, not set literals.
     * If right is not empty, then the progression contained set literals.
     */
    if (!valid_syntax || !right.empty()) {
        throw std::runtime_error("The progression is not an intersection of "
                                 "set variables");
    }

    /*
     * left_set is the optional part of the left side of the subset relation
     * which is not progressed.
     */
    if(left_set) {
        valid_syntax = left_set->gather_intersection_variables(proof_checker,
                                                               left, right);
        if(!valid_syntax) {
            throw std::runtime_error("The non-progressed part in "
                                     "set expression #" + std::to_string(left_id)
                                     + " is not an intersection of set literals.");
        }
    }
    valid_syntax = right_set->gather_union_variables(proof_checker, right, left);
    if(!valid_syntax) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not a union of set literals.");
    }

    std::vector<std::vector<const StateSetVariable *>> collection = {prog, left, right};
    const StateSetFormalism *reference = get_formalism(collection);
    if (!reference) {
        throw std::runtime_error("Statement contains only constant sets.");
    }

    if(!reference->check_statement_b2(prog, left, right, actions)) {
        throw std::runtime_error("Statement B2 is false.");
    }
    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}
;
SubsetRule _b2_plugin("b2", basic_statement_2);
}
