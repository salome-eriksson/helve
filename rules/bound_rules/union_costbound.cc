#include "../rules.h"

#include "../../knowledge.h"
#include "../../statesetcompositions.h"

namespace rules {
// If (1) gc(S) >= x and (2) gc(S') >= y, then gc(S \cup S') >= min(x,x') = bound
std::unique_ptr<Knowledge> union_costbound(SetID stateset_id, unsigned bound,
                                         std::vector<KnowledgeID> &premise_ids,
                                         const ProofChecker &proof_checker) {
    if (premise_ids.size() != 2) {
        throw std::runtime_error("Not exactly two premises given.");
    }

    // The stateset represents S \cup S'.
    const StateSetUnion *f = proof_checker.get_set<StateSetUnion>(stateset_id);
    SetID left_id = f->get_left_id();
    SetID right_id = f->get_right_id();

    // Check if premise_ids[0] is bound knowledge about S.
    const BoundKnowledge *bound_knowledge =
            proof_checker.get_knowledge<BoundKnowledge>(premise_ids[0]);
    if (bound_knowledge->get_set_id() != left_id) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[0])
                                 + " does not state that set expression #"
                                 + std::to_string(left_id) + " is dead.");
    }
    unsigned s_bound = bound_knowledge->get_bound();

    // Check if premise_ids[1] s bound knowledge about S'.
    bound_knowledge = proof_checker.get_knowledge<BoundKnowledge>(premise_ids[1]);
    if (bound_knowledge->get_set_id() != right_id) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_ids[1])
                                 + " does not state that set expression #"
                                 + std::to_string(right_id) + " is dead.");
    }
    unsigned sp_bound = bound_knowledge->get_bound();

    // Check that bound = min(s_bound,sp_bound)
    if (bound != std::min(s_bound, sp_bound)) {
        throw std::runtime_error("bound=" + std::to_string(bound) + " is not"
                                 "the minimum of " + std::to_string(s_bound)
                                 + " and " + std::to_string(sp_bound));
    }

    return std::unique_ptr<Knowledge>(new BoundKnowledge(stateset_id, bound));
}

BoundRule _uc_plugin("uc", union_costbound);
}
