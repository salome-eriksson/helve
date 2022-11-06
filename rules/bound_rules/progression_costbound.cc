#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"
#include "../../statesetcompositions.h"

#include <limits>

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
    if (premise_ids.size() < 4) {
        throw std::runtime_error("Less than four premises given.");
    }
    if (premise_ids.size() % 2 == 1) {
        throw std::runtime_error("Uneven number of premises given.");
    }

    // We do premise_id[0] last because we need to know all A_j first

    // premise_ids[1]
    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[1]);

    // Check if left set of premise_ids[1] is S \cap S_G^Pi
    const StateSetIntersection *s_and_goal =
            proof_checker.get_set<StateSetIntersection>(subset_knowledge->get_left_id());
    if (s_and_goal->get_left_id() != stateset_id) {
        throw std::runtime_error("The set expression on the left side of subset"
                                 " knowledge #" + std::to_string(premise_ids[1])
                                 + " is not an intersection with set expression #"
                                 + std::to_string(stateset_id) +
                                 + " on the left side.");
    }
    const SSVConstant *goal =
            proof_checker.get_set<SSVConstant>(s_and_goal->get_right_id());
    if(goal->get_constant_type() != ConstantType::GOAL) {
        throw std::runtime_error("The set expression on the left side of subset"
                                 " knowledge #" + std::to_string(premise_ids[1])
                                 + " is not an intersection with "
                                 + "the constant goal set on the right side.");
    }

    // Check if right set of premise_ids[1] is the empty set
    const SSVConstant *empty =
            proof_checker.get_set<SSVConstant>(subset_knowledge->get_right_id());
    if(empty->get_constant_type() != ConstantType::EMPTY) {
        throw std::runtime_error("The set expression on the right side of subset"
                                 " knowledge #" + std::to_string(premise_ids[1])
                                 + " is not the constant empty set");
    }

    // premise_ids[2] ... premise_ids[n]
    std::unordered_set<SetID> covered_action_set_ids;
    unsigned bound_from_premises = std::numeric_limits<unsigned>::max();
    for (size_t i = 1; i < (premise_ids.size()/2); ++i) {
        // S[A_j] \subseteq S \cup S_j
        subset_knowledge =
                proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[2*i]);
        // Check if the left side is a progression of S.
        const StateSetProgression *s_prog =
                proof_checker.get_set<StateSetProgression>(subset_knowledge->get_left_id());
        if (s_prog->get_stateset_id() != stateset_id) {
            throw std::runtime_error("The left side of subset knowledge #"
                                     + std::to_string(premise_ids[0])
                                     + " is not the progression of set expression #"
                                     + std::to_string(stateset_id) + ".");
        }
        // Store the action set id.
        SetID action_set_id = s_prog->get_actionset_id();
        // Check if the right side is a union with S on the left side.
        const StateSetUnion *s_cup_sp =
                proof_checker.get_set<StateSetUnion>(subset_knowledge->get_right_id());
        if (s_cup_sp->get_left_id() != stateset_id) {
            throw std::runtime_error("The right side of subset knowledge #"
                                     + std::to_string(premise_ids[0])
                                     + " is not a union containing set expression #"
                                     + std::to_string(stateset_id) + "on the left side.");
        }
        SetID sj_id = s_cup_sp->get_right_id();

        // gc(S_j) > x
        const BoundKnowledge *bound_knowledge =
                proof_checker.get_knowledge<BoundKnowledge>(premise_ids[2*i+1]);
        // Check if the knowledge talks about S_j
        if (bound_knowledge->get_set_id() != sj_id) {
            throw std::runtime_error("Bound knowledge #"
                                     + std::to_string(premise_ids[2*i+1])
                                     + " does not talk about set #"
                                     + std::to_string(sj_id));
        }
        // Update the minimum bound and covered actions
        const ActionSet *action_set =
                proof_checker.get_set<ActionSet>(action_set_id);
        if (bound_knowledge->get_bound() != std::numeric_limits<unsigned>::max()) {
            bound_from_premises =
                    std::min(bound_from_premises,
                             bound_knowledge->get_bound()+action_set->get_min_cost(proof_checker));
        }
        covered_action_set_ids.insert(action_set_id);
    }

    // A^Pi \subseteq \bigcup A_j
    const SubsetKnowledge<ActionSet> *action_subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<ActionSet>>(premise_ids[0]);
    const ActionSetConstantAll *all_actions =
            proof_checker.get_set<ActionSetConstantAll>
            (action_subset_knowledge->get_left_id());

    std::deque<SetID> id_queue;
    id_queue.push_back(action_subset_knowledge->get_right_id());
    while (!id_queue.empty()) {
        SetID id = id_queue.front();
        id_queue.pop_front();
        if(covered_action_set_ids.erase(id) == 0) {
            // id not in covered_action_set_ids
            const ActionSetUnion *action_union =
                    proof_checker.get_set<ActionSetUnion>(id);
            id_queue.push_back(action_union->get_left_id());
            id_queue.push_back(action_union->get_right_id());
        }
    }
    if (!covered_action_set_ids.empty()) {
        throw std::runtime_error("Not all action sets have been covered in premises.");
    }

    // Check if bound matches
    // TODO: can we change this to != by writing the certifiate differently?
    if (bound > bound_from_premises) {
        throw std::runtime_error("Bound " + std::to_string(bound) + " does not"
                                 " match the bound derived from the premises ("
                                 + std::to_string(bound_from_premises) +")");
    }

    return std::unique_ptr<Knowledge>(new BoundKnowledge(stateset_id, bound));
}

BoundRule _pc_plugin("pc", progression_costbound);
}
