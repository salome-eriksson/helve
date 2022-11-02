#include "../rules.h"

#include "../../knowledge.h"

namespace rules {
// If (1) gc(S) >= bound and (2) S' \susbeteq S, then gc(S') >= bound
std::unique_ptr<Knowledge> subset_costbound(SetID stateset_id, unsigned bound,
                                         std::vector<KnowledgeID> &premise_ids,
                                         const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    // Check if premise_ids[0] says that some set S' has a cost bound of bound.
    const BoundKnowledge *bound_knowledge =
            proof_checker.get_knowledge<BoundKnowledge>(premise_ids[0]);
    if (bound_knowledge->get_bound() != bound) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not talk about bound "
                                 + std::to_string(bound));
    }

    // Check if premise_ids[1] says that S is a subset of some S'.
    const SubsetKnowledge<StateSet> *subset_knowledge =
            proof_checker.get_knowledge<SubsetKnowledge<StateSet>>(premise_ids[1]);
    if (subset_knowledge->get_left_id() != stateset_id
            || subset_knowledge->get_right_id() != bound_knowledge->get_set_id()) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state that set expression #"
                                 + std::to_string(stateset_id)
                                 + " is a subset of set expression #"
                                 + std::to_string(bound_knowledge->get_set_id()));
    }

    return std::unique_ptr<Knowledge>(new BoundKnowledge(stateset_id, bound));
}

BoundRule _sc_plugin("sc", subset_costbound);
}
