#include "stateset.h"

StateSet::~StateSet() {}
StateSetVariable::~StateSetVariable() {}

std::map<std::string, StateSetConstructor> *StateSet::get_stateset_constructors() {
    static std::map<std::string, StateSetConstructor> stateset_constructors;
    return &stateset_constructors;
}


bool StateSet::gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &,
                                      std::vector<const StateSetVariable *> &,
                                      std::vector<const StateSetVariable *> &, bool) const {
    return false;
}

bool StateSet::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &,
                                             std::vector<const StateSetVariable *> &,
                                             std::vector<const StateSetVariable *> &, bool) const {
    return false;
}


bool StateSetVariable::gather_union_variables(const std::deque<std::unique_ptr<StateSet> > &,
                                              std::vector<const StateSetVariable *> &positive,
                                              std::vector<const StateSetVariable *> &, bool) const {
    positive.push_back(this);
    return true;
}

bool StateSetVariable::gather_intersection_variables(const std::deque<std::unique_ptr<StateSet> > &,
                                                     std::vector<const StateSetVariable *> &positive,
                                                     std::vector<const StateSetVariable *> &, bool) const {
    positive.push_back(this);
    return true;
}
