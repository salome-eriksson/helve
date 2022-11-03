#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"

#include <limits>

namespace rules {
// Without premises, \emptyset has a cost bound of infinity
std::unique_ptr<Knowledge> emptyset_costbound(SetID stateset_id, unsigned bound,
                                         std::vector<KnowledgeID> &premise_ids,
                                         const ProofChecker &proof_checker) {
    if (!premise_ids.empty()) {
        throw std::runtime_error("Premise list is not empty.");
    }

    const SSVConstant *f = proof_checker.get_set<SSVConstant>(stateset_id);
    if (f->get_constant_type() != ConstantType::EMPTY) {
        throw std::runtime_error("Set expression #" +
                                 std::to_string(stateset_id) +
                                 " is not the constant empty set.");
    }

    if (bound != std::numeric_limits<unsigned>::max()) {
        throw std::runtime_error("Empty set bound is not infinity.");
    }

    return std::unique_ptr<Knowledge>(new BoundKnowledge(stateset_id, bound));
}

BoundRule _ec_plugin("ec", emptyset_costbound);
}
