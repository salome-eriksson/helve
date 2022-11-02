#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"
#include "../../statesetcompositions.h"

namespace rules {
/*
 * Progression Goal: Given (0) A^Pi \subseteq \bigcup_{j=1}^{m} A_j,
 * (1) S \cap S_G^\Pi \subseteq \emptyset, and (2j) gc(S_j) > x_j and
 * (2j+1) S[A_j] \subseteq S \cup S_j for j=1...m,
 * then gc(S) = min_{j=1..m}(cost(A_j)+x_j)
 */
std::unique_ptr<Knowledge> progression_costbound(SetID stateset_id, unsigned bound,
                                            std::vector<KnowledgeID> &premise_ids,
                                            const ProofChecker &proof_checker) {
    throw std::runtime_error("RULE PC NOT IMPLEMENTED");
    /*if (premise_ids.size() != 3) {
        throw std::runtime_error("Not exactly three premises given.");
    }

    // Check if premise_ids[0] says that S[A] \subseteq S \cup S'.
    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[0]);

    // Check if the left side of premise_ids[0] is S[A].
    const StateSetProgression *s_prog =
            proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
    if (s_prog->get_stateset_id() != stateset_id) {
        throw std::runtime_error("The left side of subset knowledge #"
                                 + std::to_string(premise_ids[0])
                                 + " is not the progression of set expression #"
                                 + std::to_string(stateset_id) + ".");
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
            proof_checker.get_set<StateSetUnion>(subset_knowledge->get_right_id());
    if(s_cup_sp->get_left_id() != stateset_id) {
        throw std::runtime_error("The right side of subset knowledge #"
                                 + std::to_string(premise_ids[0])
                                 + " is not a union containing set expression #"
                                 + std::to_string(stateset_id) + "on the left side.");
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

    // Check if premise_ids[2] says that S \cap S_G^\Pi is dead.
    dead_knowledge =
            proof_checker.get_knowledge<DeadKnowledge>(premise_ids[2]);
    const StateSetIntersection *s_and_goal =
            proof_checker.get_set<StateSetIntersection>(dead_knowledge->get_set_id());

    // Check if left side of s_and_goal is S.
    if (s_and_goal->get_left_id() != stateset_id) {
        throw std::runtime_error("The set expression declared dead in knowledge #"
                                 + std::to_string(premise_ids[2]) +
                                 + " is not an intersection with set expression #"
                                 + std::to_string(stateset_id) +
                                 + " on the left side.");
    }

    // Check if right side of s_and_goal is goal
    const SSVConstant *goal =
            proof_checker.get_set<SSVConstant>(s_and_goal->get_right_id());
    if(goal->get_constant_type() != ConstantType::GOAL) {
        throw std::runtime_error("The set expression declared dead in knowledge #"
                                 + std::to_string(premise_ids[2])
                                 + " is not an intersection with "
                                 + "the constant goal set on the right side.");
    }*/

    return std::unique_ptr<Knowledge>(new BoundKnowledge(stateset_id, bound));
}

BoundRule _pc_plugin("pc", progression_costbound);
}
