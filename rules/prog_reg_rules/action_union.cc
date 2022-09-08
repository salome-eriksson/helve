#include "../rules.h"

#include "../../actionset.h"
#include "../../knowledge.h"
#include "../../statesetcompositions.h"

namespace rules{
// Action Union: Given (1) S[A] \subseteq S' and (2) S[A'] \subseteq S',
// then S[A \cup A'] \subseteq S'.
std::unique_ptr<Knowledge> action_union(SetID left_id, SetID right_id,
                                        std::vector<KnowledgeID> &premise_ids,
                                        const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
     throw std::runtime_error("Not exactly two premises given.");
    }

    SetID s0,s1,a0,a1;
    s1 = right_id;
    const StateSetProgression *progression =
            proof_checker.get_set<StateSetProgression>(left_id);
    s0 = progression->get_stateset_id();

    const ActionSetUnion *action_union =
            proof_checker.get_set<ActionSetUnion>(progression->get_actionset_id());
    a0 = action_union->get_left_id();
    a1 = action_union->get_right_id();

    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);
    progression =
            proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
    if (progression->get_actionset_id() != a0 || progression->get_stateset_id() != s0
            || subset_knowledge->get_right_id() != s1) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not state (S[A] \\subseteq S').");
    }

    subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[1]);
    progression =
            proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
    if (progression->get_actionset_id() != a1 || progression->get_stateset_id() != s0
            || subset_knowledge->get_right_id() != s1) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state (S[A'] \\subseteq S').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

SubsetRule _au_plugin("au", action_union);
}
