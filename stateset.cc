#include "stateset.h"

#include "proofchecker.h"

StateSet::~StateSet() {}
StateSetVariable::~StateSetVariable() {}

StateSetConstructorMap &StateSet::get_stateset_constructors_map() {
    static StateSetConstructorMap constructors_map = {};
    return constructors_map;
}

void StateSet::register_stateset_constructor(std::string key,
                                                    StateSetConstructor constructor) {
    if (get_stateset_constructors_map().count(key) != 0) {
        throw std::runtime_error("State set expression type" + key
                                 + "already exists.");
    }
    get_stateset_constructors_map().insert(std::make_pair(key, constructor));
}

StateSetConstructor StateSet::get_stateset_constructor(std::string key) {
    if (get_stateset_constructors_map().count(key) == 0) {
        throw std::runtime_error("State set expression type " + key
                                 + " does not exist.");
    }
    return get_stateset_constructors_map().at(key);
}

bool StateSet::gather_union_variables(
        const ProofChecker &, std::vector<const StateSetVariable *> &,
        std::vector<const StateSetVariable *> &) const {
    return false;
}

bool StateSet::gather_intersection_variables(
        const ProofChecker &, std::vector<const StateSetVariable *> &,
        std::vector<const StateSetVariable *> &) const {
    return false;
}


bool StateSetVariable::gather_union_variables(
        const ProofChecker &, std::vector<const StateSetVariable *> &positive,
        std::vector<const StateSetVariable *> &) const {
    positive.push_back(this);
    return true;
}

bool StateSetVariable::gather_intersection_variables(
        const ProofChecker &, std::vector<const StateSetVariable *> &positive,
        std::vector<const StateSetVariable *> &) const {
    positive.push_back(this);
    return true;
}
