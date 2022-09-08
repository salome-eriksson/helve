#include "../rules.h"

#include "../../knowledge.h"
#include "../../setcompositions.h"

namespace rules {
// Subset Union: Given (1) E \subseteq E'' and (2) E' \subseteq E'',
// then (E \cup E') \subseteq E''.
template<class T>
std::unique_ptr<Knowledge> subset_union(SetID left_id, SetID right_id,
                                        std::vector<KnowledgeID> & premise_ids,
                                        const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    SetID e0,e1,e2;
    const T* tmp = proof_checker.get_set<T>(left_id);
    const SetUnion *su = dynamic_cast<const SetUnion *>(tmp);
    if (!su) {
        throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                 + " is not a union.");
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = right_id;

    const SubsetKnowledge<T> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<T>>(premise_ids[0]);
    if (subset_knowledge->get_left_id() != e0 || subset_knowledge->get_right_id() != e2) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0]) +
                                 + " does not state (E \\subseteq E'').");
    }

    subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<T>>(premise_ids[1]);
    if (subset_knowledge->get_left_id() != e1 ||
            subset_knowledge->get_right_id() != e2) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state (E' \\subseteq E'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

SubsetRule _sus_plugin("sus", subset_union<StateSet>);
SubsetRule _sua_plugin("sua", subset_union<ActionSet>);
}
