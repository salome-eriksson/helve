#include "../rules.h"

#include "../../knowledge.h"
#include "../../ssvconstant.h"

namespace rules{
// Conclusion Initial: Given (1) {I} has a lower bound on goal cost of <bound>,
// then the optimal plan cost is at least <bound>
std::unique_ptr<Knowledge> bound_initial(unsigned bound, KnowledgeID premise_id,
                                              const ProofChecker &proof_checker) {
    // Check that premise says that {I} has a lower bound on goal cost of <bound>.
    const BoundKnowledge *bound_knowledge =
            proof_checker.get_knowledge<BoundKnowledge>(premise_id);

    const SSVConstant *init =
            proof_checker.get_set<SSVConstant>(bound_knowledge->get_set_id());
    if (init->get_constant_type() != ConstantType::INIT) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_id)
                                 + " does not state that the constant "
                                 "initial set is dead.");
    }
    if (bound_knowledge->get_bound() != bound) {
        throw std::runtime_error("Knowledge #" + std::to_string(premise_id)
                                 + "does not state that the initial state"
                                 "set has a lower goal cost bound of "
                                 + std::to_string(bound));
    }

    return std::unique_ptr<Knowledge>(new UnsolvableKnowledge());
}

OptimalityRule _bi_plugin("bi", bound_initial);
}
