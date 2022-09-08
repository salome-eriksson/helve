#include "../rules.h"

#include "../../knowledge.h"
#include "../../statesetcompositions.h"

namespace rules{
// Union Dead: Given (1) S is dead and (2) S' is dead, then S \cup S' is dead.
std::unique_ptr<Knowledge> union_dead(SetID stateset_id,
                                      std::vector<KnowledgeID> &premise_ids,
                                      const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    // The stateset represents S \cup S'.
    const StateSetUnion *f = proof_checker.get_set<StateSetUnion>(stateset_id);
    SetID left_id = f->get_left_id();
    SetID right_id = f->get_right_id();

    // Check if premise_ids[0] says that S is dead.
    const DeadKnowledge *dead_knowledge =
            proof_checker.get_knowledge<DeadKnowledge>(premise_ids[0]);
    if (dead_knowledge->get_set_id() != left_id) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not state that set expression #"
                                 + std::to_string(left_id) + " is dead.");
    }

    // Check if premise_ids[1] says that S' is dead.
    dead_knowledge = proof_checker.get_knowledge<DeadKnowledge>(premise_ids[1]);
    if (dead_knowledge->get_set_id() != right_id) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state that set expression #"
                                 + std::to_string(right_id) + " is dead.");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}

DeadnessRule _ud_plugin("ud", union_dead);
}
