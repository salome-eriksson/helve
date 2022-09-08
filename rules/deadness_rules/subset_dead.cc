#include "../rules.h"

#include "../../knowledge.h"

namespace rules{
// Subset Dead: Given (1) S \subseteq S' and (2) S' is dead, set=S is dead.
std::unique_ptr<Knowledge> subset_dead(SetID stateset_id,
                                       std::vector<KnowledgeID> &premise_ids,
                                       const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    // Check if premise_ids[0] says that some set S' is dead.
    const DeadKnowledge *dead_knowledge =
            proof_checker.get_knowledge<DeadKnowledge>(premise_ids[0]);

    // Check if premise_ids[1] says that S is a subset of some S'.
    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[1]);
    if (subset_knowledge->get_left_id() != stateset_id
            || subset_knowledge->get_right_id() != dead_knowledge->get_set_id()) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state that set expression #"
                                 + std::to_string(stateset_id)
                                 + " is a subset of set expression #"
                                 + std::to_string(dead_knowledge->get_set_id()));
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}

DeadnessRule _sd_plugin("sd", subset_dead);
}
