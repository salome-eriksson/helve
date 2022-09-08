#include "basic_statement_functions.h"
#include "../rules.h"

namespace rules{
// B1: \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
std::unique_ptr<Knowledge> basic_statement_1(SetID left_id, SetID right_id,
                                             std::vector<KnowledgeID> & premise_ids,
                                             const ProofChecker &proof_checker) {
    if (!premise_ids.empty()) {
        throw std::runtime_error("Premise list is not empty.");
    }

    std::vector<const StateSetVariable *> left;
    std::vector<const StateSetVariable *> right;
    bool valid_syntax = false;


    const StateSet *left_set = proof_checker.get_set<StateSet>(left_id);
    valid_syntax = left_set->gather_intersection_variables(proof_checker, left, right);
    if (!valid_syntax) {
        throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                 + " is not an intersection of literals "
                                 "of the same type.");
    }
    const StateSet *right_set = proof_checker.get_set<StateSet>(right_id);
    valid_syntax =  right_set->gather_union_variables(proof_checker, right, left);
    if (!valid_syntax) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not a union of literals "
                                 "of the same type.");
    }

    std::vector<std::vector<const StateSetVariable *>> collection = {left, right};
    const StateSetFormalism *reference = get_formalism(collection);
    if (!reference) {
        throw std::runtime_error("Statement contains only constant sets.");
    }

    if (!reference->check_statement_b1(left, right)) {
        throw std::runtime_error("Statement B1 is false.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}

SubsetRule _b1_plugin("b1", basic_statement_1);
}
