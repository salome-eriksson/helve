#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"
#include "../../statesetcompositions.h"

namespace rules{
// Progression Initial: Given (1) S[A] \subseteq S \cup S', (2) S' is dead and
// (3) {I} \subseteq S_not, then set=S_not is dead.
std::unique_ptr<Knowledge> progression_initial(SetID stateset_id,
                                               std::vector<KnowledgeID> &premise_ids,
                                               const ProofChecker &proof_checker) {
    if (premise_ids.size() != 3) {
        throw std::runtime_error("Not exactly three premises given.");
    }

    // Check if stateset corresponds to S_not.
    const StateSetNegation *s_not =
            proof_checker.get_set<StateSetNegation>(stateset_id);
    SetID s_id = s_not->get_child_id();

    // Check if premise_ids[0] says that S[A] \subseteq S \cup S'.
    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);

    // Check if the left side of premise_ids[0] is S[A].
    const StateSetProgression *s_prog =
            proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
    if (s_prog->get_stateset_id() != s_id) {
        throw std::runtime_error("The left side of subset knowledge #"
                                 + std::to_string(premise_ids[0])
                                 + " is not the progression of set expression #"
                                 + std::to_string(s_id) + ".");
    }

    // Check if the progression is over the constant *all actions* set
    const ActionSetConstantAll *action_set =
            proof_checker.get_set<ActionSetConstantAll>(s_prog->get_actionset_id());
    if(!action_set->is_constantall()) {
        throw std::runtime_error("The progression in the left side of subset "
                                 "knowledge #" + std::to_string(premise_ids[0])
                                 + " is not over all actions.");
    }

    // Check if the right side of premise_ids[0] is S \cup S'.
    const StateSetUnion *s_cup_sp =
            proof_checker.get_set<StateSetUnion>(subset_knowledge->get_right_id());;
    if(s_cup_sp->get_left_id() != s_id) {
        throw std::runtime_error("The right side of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a union containing set expression #" +
                                 std::to_string(s_id) + "on the left side.");
    }

    SetID sp_id = s_cup_sp->get_right_id();

    // Check if premise_ids[1] says that S' is dead.
    const DeadKnowledge *dead_knowledge =
            proof_checker.get_knowledge<DeadKnowledge>(premise_ids[1]);
    if (dead_knowledge->get_set_id() != sp_id) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state that set expression #"
                                 + std::to_string(sp_id) + " is dead.");
    }

    // Check if premise_ids[2] says that {I} \subseteq S.
    subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[2]);

    // Check that the left side of premise_ids[2] is {I}.
    const SSVConstant *init =
            proof_checker.get_set<SSVConstant>(subset_knowledge->get_left_id());
    if (init->get_constant_type() != ConstantType::INIT) {
        throw std::runtime_error("The left side of subset knowledge #"
                                 + std::to_string(premise_ids[2])
                                 + " is not the constant initial set.");
    }
    // Check that the right side of pemise_ids[2] is S.
    if(subset_knowledge->get_right_id() != s_id) {
        throw std::runtime_error("The right side of subset knowledge #"
                                 + std::to_string(premise_ids[2])
                                 + " is not set expression #"
                                 + std::to_string(s_id) + ".");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}

DeadnessRule _pi_plugin("pi", progression_initial);
}
