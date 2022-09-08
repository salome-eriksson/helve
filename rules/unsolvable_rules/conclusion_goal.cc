#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"

namespace rules{
// Conclusion Goal: Given (1) S_G^\Pi is dead, then the task is unsolvable.
std::unique_ptr<Knowledge> conclusion_goal(KnowledgeID premise_id,
                                           const ProofChecker &proof_checker) {
    // Check that premise says that S_G^\Pi is dead.
    const DeadKnowledge *dead_knowledge =
            proof_checker.get_knowledge<DeadKnowledge>(premise_id);

    const SSVConstant *goal =
            proof_checker.get_set<SSVConstant>(dead_knowledge->get_set_id());
    if (goal->get_constant_type() != ConstantType::GOAL) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_id)
                                 + " does not state that the constant "
                                 "goal set is dead.");
    }

    return std::unique_ptr<Knowledge>(new UnsolvableKnowledge());
}

UnsolvableRule _cg_plugin("cg", conclusion_goal);
}
