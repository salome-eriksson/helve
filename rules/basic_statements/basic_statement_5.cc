#include "basic_statement_functions.h"
#include "../rules.h"

namespace rules{
// check if A \subseteq A'
std::unique_ptr<Knowledge> basic_statement_5(SetID left_id, SetID right_id,
                                             std::vector<KnowledgeID> &,
                                             const ProofChecker &proof_checker) {

    std::unordered_set<size_t> left_indices, right_indices;
    const ActionSet *left_set = proof_checker.get_set<ActionSet>(right_id);
    const ActionSet *right_set = proof_checker.get_set<ActionSet>(right_id);
    left_set->get_actions(proof_checker, left_indices);
    right_set->get_actions(proof_checker, right_indices);

    for (int index: left_indices) {
        if (right_indices.find(index) == right_indices.end()) {
            throw std::runtime_error("Statement B5 is false.");
        }
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<ActionSet>(left_id,right_id));
}

SubsetRule _b5_plugin("b5", basic_statement_5);
}
