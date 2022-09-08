#include "../rules.h"

#include "../../knowledge.h"
#include "../../statesetcompositions.h"

namespace rules{
// Regression to Progression: Given (1) [A]S \subseteq S',
// then S'_not[A] \subseteq S_not.
std::unique_ptr<Knowledge> regression_to_progression (
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids,
        const ProofChecker &proof_checker) {
    if (premise_ids.size() != 1) {
        throw std::runtime_error("Not exactly one premise given.");
    }

    SetID s0,s1,a0;
    const StateSetProgression *progression =
            proof_checker.get_set<StateSetProgression>(left_id);
    const StateSetNegation *negation =
            proof_checker.get_set<StateSetNegation>(progression->get_stateset_id());
    s1 = negation->get_child_id();
    a0 = progression->get_actionset_id();
    negation = proof_checker.get_set<StateSetNegation>(right_id);
    s0 = negation->get_child_id();

    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);
    const StateSetRegression *regression =
            proof_checker.get_set<StateSetRegression>(subset_knowledge->get_left_id());
    if (regression->get_actionset_id() != a0 || regression->get_stateset_id() != s0
            || subset_knowledge->get_right_id() != s1) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not state ([A]S \\subseteq S').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

SubsetRule _rp_plugin("rp", regression_to_progression);
}
