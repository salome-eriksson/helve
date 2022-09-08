#include "../rules.h"

#include "../../knowledge.h"
#include "../../setcompositions.h"

namespace rules {
// Subset Transitivity: Given (1) E \subseteq E' and (2) E' \subseteq E'',
// then E \subseteq E''.
template<class T>
std::unique_ptr<Knowledge> subset_transitivity(SetID left_id, SetID right_id,
                                               std::vector<KnowledgeID> &premise_ids,
                                               const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    SetID e0,e1,e2;
    e0 = left_id;
    e2 = right_id;

    const SubsetKnowledge<T> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<T>>(premise_ids[0]);
    if (subset_knowledge->get_left_id() != e0) {
        throw std::runtime_error("Knwoledge #" + std::to_string(premise_ids[0])
                                 + " does not state (E \\subseteq E').");
    }
    e1 = subset_knowledge->get_right_id();

    subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<T>>(premise_ids[1]);
    if (subset_knowledge->get_left_id() != e1 ||
            subset_knowledge->get_right_id()!= e2) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state (E' \\subseteq E'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

SubsetRule _sts_plugin("sts", subset_transitivity<StateSet>);
SubsetRule _sta_pluign("sta", subset_transitivity<ActionSet>);
}
