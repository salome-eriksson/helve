#include "../rules.h"

#include "../../knowledge.h"
#include "../../setcompositions.h"

namespace rules {
// Union Left: Without premises, E \subseteq E' \cup E.
template<class T>
std::unique_ptr<Knowledge> union_left(SetID left_id, SetID right_id,
                                      std::vector<KnowledgeID> & premise_ids,
                                      const ProofChecker &proof_checker) {
    if (!premise_ids.empty()) {
        throw std::runtime_error("Premise list is not empty.");
    }

    const T *right = proof_checker.get_set<T>(right_id);
    const SetUnion *runion = dynamic_cast<const SetUnion *>(right);
    if (!runion) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not a union.");
    }

    if (runion->get_right_id() != left_id) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " ist not a union with set expression #"
                                 + std::to_string(left_id) + " on its right side.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id, right_id));
}

SubsetRule _uls_plugin("uls", union_left<StateSet>);
SubsetRule _ula_plugin("ula", union_left<ActionSet>);
}
