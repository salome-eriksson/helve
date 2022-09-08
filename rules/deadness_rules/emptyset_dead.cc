#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"

namespace rules {
// Without premises, \emptyset is dead
std::unique_ptr<Knowledge> emptyset_dead(SetID stateset_id,
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
    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}

DeadnessRule _ed_plugin("ed", emptyset_dead);
}

