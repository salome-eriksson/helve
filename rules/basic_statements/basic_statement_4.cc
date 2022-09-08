#include "basic_statement_functions.h"
#include "../rules.h"

#include "../../statesetcompositions.h"

namespace rules{
// B4: L \subseteq L', where L and L' are represented by arbitrary formalisms
std::unique_ptr<Knowledge> basic_statement_4(SetID left_id, SetID right_id,
                                             std::vector<KnowledgeID> &,
                                             const ProofChecker &proof_checker) {
    bool left_positive = true;
    bool right_positive = true;
    const StateSet *left_generic = proof_checker.get_set<StateSet>(left_id);
    const StateSet *right_generic = proof_checker.get_set<StateSet>(right_id);

    const StateSetFormalism *left =
            dynamic_cast<const StateSetFormalism *>(left_generic);
    if (!left) {
        const StateSetNegation *tmp =
            dynamic_cast<const StateSetNegation *>(right_generic);
        if (tmp) {
            left = dynamic_cast<const StateSetFormalism *>
                    (proof_checker.get_set<StateSet>(tmp->get_child_id()));
        }
        if (!left) {
            throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                     + " is not a non-constant set literal.");
        }
        left_positive = false;
    }

    const StateSetFormalism *right =
            dynamic_cast<const StateSetFormalism *>(right_generic);
    if (!right) {
        const StateSetNegation *tmp =
                dynamic_cast<const StateSetNegation *>(right_generic);
        if (tmp) {
            right = dynamic_cast<const StateSetFormalism *>
                    (proof_checker.get_set<StateSet>(tmp->get_child_id()));
        }
        if (!right) {
            throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                     + " is not a non-constant set literal.");
        }
        right_positive = false;
    }

    if(!left->check_statement_b4(right, left_positive, right_positive)) {
        throw std::runtime_error("Statement B4 is false.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}

SubsetRule _b4_plugin("b4", basic_statement_4);
}
