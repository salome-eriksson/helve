#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"
#include "../../statesetcompositions.h"

namespace rules{
// Regression Goal: Given (1)[A]S \subseteq S \cup S', (2) S' is dead and
// (3) S_not \cap S_G^\Pi is dead, then S_not is dead.
std::unique_ptr<Knowledge> regression_goal(SetID stateset_id,
                                           std::vector<KnowledgeID> &premise_ids,
                                           const ProofChecker &proof_checker) {
    if (premise_ids.size() != 3) {
        throw std::runtime_error("Not exactly three premises given.");
    }

    // check if set corresponds to s_not
    const StateSetNegation *s_not =
            proof_checker.get_set<StateSetNegation>(stateset_id);
    SetID s_id = s_not->get_child_id();

    // Check if premise_ids[0] says that [A]S \subseteq S \cup S'.
    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);

    // Check if the left side of premise_ids[0] is [A]S.
    const StateSetRegression *s_reg =
            proof_checker.get_set<StateSetRegression>(subset_knowledge->get_left_id());
    if (s_reg->get_stateset_id() != s_id) {
        throw std::runtime_error("The left side of subset knowledge #"
                                 + std::to_string(premise_ids[0])
                                 + " is not the regeression of set expression #"
                                 + std::to_string(s_id) + ".");
    }

    // Check if the regression is over the constant *all actions* set
    const ActionSetConstantAll *action_set =
            proof_checker.get_set<ActionSetConstantAll>(s_reg->get_actionset_id());
    if(!action_set->is_constantall()) {
        throw std::runtime_error("The regression is not over all actions.");
    }

    // Check if the right side of premise_ids[0] is S \cup S'.
    const StateSetUnion *s_cup_sp =
            proof_checker.get_set<StateSetUnion>(subset_knowledge->get_right_id());
    if (s_cup_sp->get_left_id() != s_id) {
        throw std::runtime_error("The right side of subset knowledge #"
                                 + std::to_string(premise_ids[0])
                                 + " is not a union containing set expression #"
                                 + std::to_string(s_id) + ".");
    }

    SetID sp_id = s_cup_sp->get_right_id();

    // Check if premise_ids[1] says that S' is dead.
    const DeadKnowledge *dead_knowledge =
            proof_checker.get_knowledge<DeadKnowledge>(premise_ids[1]);
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state that set expression #"
                                 + std::to_string(sp_id) + " is dead.");
    }

    // Check if premise_ids[2] says that S_not \cap S_G(\Pi) is dead.
    dead_knowledge = proof_checker.get_knowledge<DeadKnowledge>(premise_ids[2]);
    const StateSetIntersection *s_not_and_goal =
            proof_checker.get_set<StateSetIntersection>(dead_knowledge->get_set_id());

    // Check if the left side of s_not_and goal is S_not.
    if (s_not_and_goal->get_left_id() != stateset_id) {
        throw std::runtime_error("The set expression declared dead in knowledge #"
                                 + std::to_string(premise_ids[2])
                                 + " is not an intersection with set expression #"
                                 + std::to_string(stateset_id)
                                 + " on the left side.");
    }

    // Check if the right side of s_not_and goal is goal
    const SSVConstant *goal =
            proof_checker.get_set<SSVConstant>(s_not_and_goal->get_right_id());
    if(goal->get_constant_type() != ConstantType::GOAL) {
        throw std::runtime_error("The set expression declared dead in knowledge #"
                                 + std::to_string(premise_ids[2])
                                 + " is not an intersection "
                                 + "with the constant goal set on the right side.");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}

DeadnessRule _rg_plugin("rg", regression_goal);
}
