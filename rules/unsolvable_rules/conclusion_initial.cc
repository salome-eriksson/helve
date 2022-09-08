#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"

namespace rules{
// Conclusion Initial: Given (1) {I} is dead, then the task is unsolvable.
std::unique_ptr<Knowledge> conclusion_initial(KnowledgeID premise_id,
                                              const ProofChecker &proof_checker) {
    // Check that premise says that {I} is dead.
    const DeadKnowledge *dead_knowledge =
            proof_checker.get_knowledge<DeadKnowledge>(premise_id);

    const SSVConstant *init =
            proof_checker.get_set<SSVConstant>(dead_knowledge->get_set_id());
    if (init->get_constant_type() != ConstantType::INIT) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_id)
                                 + " does not state that the constant "
                                 "initial set is dead.");
    }

    return std::unique_ptr<Knowledge>(new UnsolvableKnowledge());
}

UnsolvableRule _ci_plugin("ci", conclusion_initial);
}
