#include "../rules.h"

#include "../../knowledge.h"
#include "../../setcompositions.h"

namespace rules {
// Subset Intersection: Given (1) E \subseteq E' and (2) E \subseteq E'',
// then E \subseteq (E' \cap E'').
template<class T>
std::unique_ptr<Knowledge> subset_intersection(SetID left_id, SetID right_id,
                                               std::vector<KnowledgeID> &premise_ids,
                                               const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    SetID e0,e1,e2;
    const T* tmp = proof_checker.get_set<T>(right_id);
    const SetIntersection *si =
            dynamic_cast<const SetIntersection*>(tmp);
    if (!si) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not an intersection.");
    }
    e0 = left_id;
    e1 = si->get_left_id();
    e2 = si->get_right_id();

    const SubsetKnowledge<T> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<T>>(premise_ids[0]);
    if (subset_knowledge->get_left_id() != e0 ||
            subset_knowledge->get_right_id() != e1) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not state (E \\subseteq E').");
    }

    subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<T>>(premise_ids[1]);
    if (subset_knowledge->get_left_id() != e0 ||
            subset_knowledge->get_right_id() != e2) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state (E \\subseteq E'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

SubsetRule _sis_plugin("sis", subset_intersection<StateSet>);
SubsetRule _sia_plugin("sia", subset_intersection<ActionSet>);
}
