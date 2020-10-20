#include "statesetcompositions.h"

StateSetUnion::StateSetUnion(std::stringstream &input, Task &) {
    input >> id_left;
    input >> id_right;
}

int StateSetUnion::get_left_id() const {
    return id_left;
}

int StateSetUnion::get_right_id() const {
    return id_right;
}

bool StateSetUnion::gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                           std::vector<const StateSetVariable *> &positive,
                                           std::vector<const StateSetVariable *> &negative,
                                           bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_left]->gather_union_variables(formulas, positive, negative, false) &&
           formulas[id_right]->gather_union_variables(formulas, positive, negative, false);
}
StateSetBuilder<StateSetUnion> union_builder("u");


StateSetIntersection::StateSetIntersection(std::stringstream &input, Task &) {
    input >> id_left;
    input >> id_right;
}

int StateSetIntersection::get_left_id() const {
    return id_left;
}

int StateSetIntersection::get_right_id() const {
    return id_right;
}

bool StateSetIntersection::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                                         std::vector<const StateSetVariable *> &positive,
                                                         std::vector<const StateSetVariable *> &negative,
                                                         bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_left]->gather_intersection_variables(formulas, positive, negative, false) &&
           formulas[id_right]->gather_intersection_variables(formulas, positive, negative, false);
}
StateSetBuilder<StateSetIntersection> intersection_builder("i");


StateSetNegation::StateSetNegation(std::stringstream &input, Task &) {
    input >> id_child;
}

int StateSetNegation::get_child_id() const {
    return id_child;
}

bool StateSetNegation::gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                              std::vector<const StateSetVariable *> &positive,
                                              std::vector<const StateSetVariable *> &negative,
                                              bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_child]->gather_union_variables(formulas, negative, positive, true);
}

bool StateSetNegation::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                                     std::vector<const StateSetVariable *> &positive,
                                                     std::vector<const StateSetVariable *> &negative,
                                                     bool must_be_variable) const {
    if (must_be_variable) {
        return false;
    }
    return formulas[id_child]->gather_intersection_variables(formulas, negative, positive, true);
}
StateSetBuilder<StateSetNegation> negation_builder("n");


StateSetProgression::StateSetProgression(std::stringstream &input, Task &) {
    input >> id_stateset;
    input >> id_actionset;
}

int StateSetProgression::get_actionset_id() const {
    return id_actionset;
}

int StateSetProgression::get_stateset_id() const {
    return id_stateset;
}
StateSetBuilder<StateSetProgression> progression_builder("p");


StateSetRegression::StateSetRegression(std::stringstream &input, Task &) {
    input >> id_stateset;
    input >> id_actionset;
}

int StateSetRegression::get_actionset_id() const {
    return id_actionset;
}

int StateSetRegression::get_stateset_id() const {
    return id_stateset;
}
StateSetBuilder<StateSetRegression> regression_builder("r");
