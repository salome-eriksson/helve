#include "../rules.h"

#include "../../knowledge.h"
#include "../../setcompositions.h"

namespace rules{
// Intersection Right: Without premises, E \cap E' \subseteq E.
template<class T>
std::unique_ptr<Knowledge> intersection_right(SetID left_id, SetID right_id,
                                              std::vector<KnowledgeID> & premise_ids,
                                              const ProofChecker &proof_checker) {
    if (!premise_ids.empty()) {
        throw std::runtime_error("Premise list is not empty.");
    }

    const T *left = proof_checker.get_set<T>(left_id);
    const SetIntersection *lintersection =
            dynamic_cast<const SetIntersection *>(left);
    if (!lintersection) {
        throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                 + " is not an intersection.");
    }

    if (lintersection->get_left_id() != right_id) {
        throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                 + " is not an intersection with set expression #"
                                 + std::to_string(right_id) + " on its left side.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

SubsetRule _irs_plugin("irs", intersection_right<StateSet>);
SubsetRule _ira_plugin("ira", intersection_right<ActionSet>);
}
