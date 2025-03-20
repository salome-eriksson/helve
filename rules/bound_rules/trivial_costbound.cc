#include "../rules.h"

#include "../../knowledge.h"

namespace rules {
// Without premises, any set has a cost bound of 0.
std::unique_ptr<Knowledge> trivial_costbound(SetID stateset_id, unsigned bound,
                                         std::vector<KnowledgeID> &premise_ids,
                                         const ProofChecker &) {
    if (!premise_ids.empty()) {
        throw std::runtime_error("Premise list is not empty.");
    }
    if (bound != 0) {
        throw std::runtime_error("The given bound is not 0.");
    }
    return std::unique_ptr<Knowledge>(new BoundKnowledge(stateset_id, bound));
}

BoundRule _tc_plugin("tc", trivial_costbound);
}

