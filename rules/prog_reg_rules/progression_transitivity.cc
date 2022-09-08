#include "../rules.h"

#include "../../knowledge.h"
#include "../../statesetcompositions.h"

namespace rules{
// Progression Transitivity: Given (1) S[A] \subseteq S'' and (2) S' \subseteq S,
// then S'[A] \subseteq S''.
std::unique_ptr<Knowledge> progression_transitivity(SetID left_id, SetID right_id,
                                                    std::vector<KnowledgeID> &premise_ids,
                                                    const ProofChecker &proof_checker) {
     if (premise_ids.size() != 2) {
         throw std::runtime_error("Not exactly two premises given.");
     }

    SetID s0,s1,s2,a0;
    s2 = right_id;
    const StateSetProgression *progression =
            proof_checker.get_set<StateSetProgression>(left_id);
    s1 = progression->get_stateset_id();
    a0 = progression->get_actionset_id();

    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);
    progression =
            proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
    if (progression->get_actionset_id() != a0
            || subset_knowledge->get_right_id() != s2) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0]) +
                                 " does not state (S[A] \\subseteq S'').");
    }
    s0 = progression->get_stateset_id();

    subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>> (premise_ids[1]);
    if (subset_knowledge->get_left_id() != s1
            || subset_knowledge->get_right_id() != s0) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state (S' \\subseteq S).");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

SubsetRule _pt_plugin("pt", progression_transitivity);
}
