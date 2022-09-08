#include "statesetcompositions.h"

#include "global_funcs.h"
#include "proofchecker.h"

StateSetUnion::StateSetUnion(std::stringstream &input, Task &)
    : id_left(read_uint(input)), id_right(read_uint(input)) {
}

SetID StateSetUnion::get_left_id() const {
    return id_left;
}

SetID StateSetUnion::get_right_id() const {
    return id_right;
}

bool StateSetUnion::gather_union_variables(
        const ProofChecker &proof_checker,
        std::vector<const StateSetVariable *> &positive,
        std::vector<const StateSetVariable *> &negative,
        bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    const StateSet *left = proof_checker.get_set<StateSet>(id_left);
    const StateSet *right = proof_checker.get_set<StateSet>(id_right);
    return left->gather_union_variables(proof_checker, positive, negative, false)
           && right->gather_union_variables(proof_checker, positive, negative, false);
}
StateSetBuilder<StateSetUnion> union_builder("u");


StateSetIntersection::StateSetIntersection(std::stringstream &input, Task &)
    : id_left(read_uint(input)), id_right(read_uint(input)) {
}

SetID StateSetIntersection::get_left_id() const {
    return id_left;
}

SetID StateSetIntersection::get_right_id() const {
    return id_right;
}

bool StateSetIntersection::gather_intersection_variables(
        const ProofChecker &proof_checker,
        std::vector<const StateSetVariable *> &positive,
        std::vector<const StateSetVariable *> &negative,
        bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    const StateSet *left = proof_checker.get_set<StateSet>(id_left);
    const StateSet *right = proof_checker.get_set<StateSet>(id_right);
    return left->gather_intersection_variables(proof_checker, positive, negative, false)
           && right->gather_intersection_variables(proof_checker, positive, negative, false);
}
StateSetBuilder<StateSetIntersection> intersection_builder("i");


StateSetNegation::StateSetNegation(std::stringstream &input, Task &)
    : id_child(read_uint(input)) {
}

SetID StateSetNegation::get_child_id() const {
    return id_child;
}

bool StateSetNegation::gather_union_variables(
        const ProofChecker &proof_checker,
        std::vector<const StateSetVariable *> &positive,
        std::vector<const StateSetVariable *> &negative,
        bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    const StateSet *child = proof_checker.get_set<StateSet>(id_child);
    return child->gather_union_variables(proof_checker, negative, positive, true);
}

bool StateSetNegation::gather_intersection_variables(
        const ProofChecker &proof_checker,
        std::vector<const StateSetVariable *> &positive,
        std::vector<const StateSetVariable *> &negative,
        bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    const StateSet *child = proof_checker.get_set<StateSet>(id_child);
    return child->gather_intersection_variables(proof_checker, negative, positive, true);
}
StateSetBuilder<StateSetNegation> negation_builder("n");


StateSetProgression::StateSetProgression(std::stringstream &input, Task &)
    : id_stateset(read_uint(input)), id_actionset(read_uint(input)) {
}

SetID StateSetProgression::get_actionset_id() const {
    return id_actionset;
}

SetID StateSetProgression::get_stateset_id() const {
    return id_stateset;
}
StateSetBuilder<StateSetProgression> progression_builder("p");


StateSetRegression::StateSetRegression(std::stringstream &input, Task &)
    : id_stateset(read_uint(input)), id_actionset(read_uint(input)) {
}

SetID StateSetRegression::get_actionset_id() const {
    return id_actionset;
}

SetID StateSetRegression::get_stateset_id() const {
    return id_stateset;
}
StateSetBuilder<StateSetRegression> regression_builder("r");
