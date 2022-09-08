#include "../rules.h"

#include "../../actionset.h"
#include "../../knowledge.h"
#include "../../statesetcompositions.h"

namespace rules{
// Action Transitivity: Given (1) S[A] \subseteq S' and (2) A' \subseteq A,
// then S[A'] \subseteq S'.
std::unique_ptr<Knowledge> action_transitivity(SetID left_id, SetID right_id,
                                               std::vector<KnowledgeID> &premise_ids,
                                               const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    SetID s0, s1, a0, a1;
    s1 = right_id;
    const StateSetProgression *progression =
            proof_checker.get_set<StateSetProgression>(left_id);
    s0 = progression->get_stateset_id();
    a1 = progression->get_actionset_id();

    const SubsetKnowledge<StateSet> *subset_knowledege_s =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);
    progression =
            proof_checker.get_set<StateSetProgression>(subset_knowledege_s->get_left_id());
    if (progression->get_stateset_id() != s0
            || subset_knowledege_s->get_right_id() != s1) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not state (S[A] \\subseteq S').");
    }
    a0 = progression->get_actionset_id();

    const SubsetKnowledge<ActionSet> *subset_knowledege_a =
            proof_checker.get_knowledge<SubsetKnowledge<ActionSet>>(premise_ids[1]);
    if (subset_knowledege_a->get_left_id() != a1
            || subset_knowledege_a->get_right_id() != a0) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state (A' \\subseteq A).");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

SubsetRule _at_plugin("at", action_transitivity);
}
