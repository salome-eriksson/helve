#include "../rules.h"

#include "../../knowledge.h"
#include "../../statesetcompositions.h"

namespace rules{
// Progression Union: Given (1) S[A] \subseteq S'' and (2) S'[A] \subseteq S'',
// then (S \cup S')[A] \subseteq S''.
std::unique_ptr<Knowledge> progression_union(SetID left_id, SetID right_id,
                                             std::vector<KnowledgeID> &premise_ids,
                                             const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
      throw std::runtime_error("Not exactly two premises given.");
    }


    SetID s0,s1,s2,a0;
    const StateSetProgression *progression =
            proof_checker.get_set<StateSetProgression>(left_id);
    const StateSetUnion *state_union =
            proof_checker.get_set<StateSetUnion>(progression->get_stateset_id());
    s0 = state_union->get_left_id();
    s1 = state_union->get_right_id();
    s2 = right_id;
    a0 = progression->get_actionset_id();

    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);
    progression =
            proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
    if (progression->get_actionset_id() != a0 || progression->get_stateset_id() != s0
            || subset_knowledge->get_right_id() != s2) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not state (S[A] \\subseteq S'').");
    }

    subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[1]);
    progression =
            proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
    if (progression->get_actionset_id() != a0 || progression->get_stateset_id() != s1
            || subset_knowledge->get_right_id() != s2) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state (S'[A] \\subseteq S'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

SubsetRule _pu_plugin("pu", progression_union);
}
