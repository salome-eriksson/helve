#include "basic_statement_functions.h"
#include "../rules.h"

#include "../../statesetcompositions.h"

namespace rules{
// B3: [A](\bigcap_{X \in \mathcal X} X) \land \bigcap_{L \in \mathcal L} L
//     \subseteq \bigcup_{L' \in \mathcal L'} L'
std::unique_ptr<Knowledge> basic_statement_3(SetID left_id, SetID right_id,
                                             std::vector<KnowledgeID> &,
                                             const ProofChecker &proof_checker) {
    std::vector<const StateSetVariable *> reg;
    std::vector<const StateSetVariable *> left;
    std::vector<const StateSetVariable *> right;
    std::unordered_set<size_t> actions;
    const StateSet *regressed_set = nullptr;
    const ActionSet *action_set = nullptr;
    const StateSet *left_set = nullptr;
    const StateSet *right_set = proof_checker.get_set<StateSet>(right_id);

    /*
     * We expect the left side to either be a regression
     * or an intersection with a regression on the left side.
     */
    const StateSetRegression *regression =
            dynamic_cast<const StateSetRegression *>(proof_checker.get_set<StateSet>(left_id));
    if (!regression) { // Left side is not a regression, is it an intersection?
        const StateSetIntersection *intersection =
                dynamic_cast<const StateSetIntersection *>(proof_checker.get_set<StateSet>(left_id));
        if (!intersection) {
            throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                     + " is not a regression or intersection.");
        }
        const StateSet *intersection_left =
                proof_checker.get_set<StateSet>(intersection->get_left_id());
        regression =
                dynamic_cast<const StateSetRegression *>(intersection_left);
        if (!regression) {
            throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                     + " is an intersection but not with a"
                                     " regression on the left side.");
        }
        left_set = proof_checker.get_set<StateSet>(intersection->get_right_id());
    }
    action_set = proof_checker.get_set<ActionSet>(regression->get_actionset_id());
    action_set->get_actions(proof_checker, actions);
    regressed_set = proof_checker.get_set<StateSet>(regression->get_stateset_id());


    bool valid_syntax =
            regressed_set->gather_intersection_variables(proof_checker, reg, right);
    /*
     * In the regression, we only allow set variables, not set literals.
     * If right is not empty, then reg contianed set literals.
     */
    if (!valid_syntax || !right.empty()) {
        throw std::runtime_error("The regression  is not an interesction of "
                                 "set variables.");
    }

    /*
     * left_set is the optional part of the left side of the subset relation
     * which is not regressed.
     */
    if(left_set) {
        valid_syntax = left_set->gather_intersection_variables(proof_checker, left, right);
        if(!valid_syntax) {
            throw std::runtime_error("The non-regressed part in set expression #"
                                     + std::to_string(left_id)
                                     + " is not an intersection of set literals.");
        }
    }
    valid_syntax = right_set->gather_union_variables(proof_checker, right, left);
    if(!valid_syntax) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not a union of set literals.");
    }

    std::vector<std::vector<const StateSetVariable *>> collection = {reg, left, right};
    const StateSetFormalism *reference = get_formalism(collection);
    if (!reference) {
        throw std::runtime_error("Statement consists of only constant sets.");
    }

    if(!reference->check_statement_b3(reg, left, right, actions)) {
        throw std::runtime_error("Statement B3 is false.");
    }
    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}

SubsetRule _b3_plugin("b3", basic_statement_3);
}
