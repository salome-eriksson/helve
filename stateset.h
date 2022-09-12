#ifndef STATESET_H
#define STATESET_H

#include "task.h"

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

class ProofChecker;
class StateSet;
class StateSetVariable;
typedef std::function<std::unique_ptr<StateSet>(std::stringstream &input, Task &task)> StateSetConstructor;

class StateSet
{
public:
    virtual ~StateSet() = 0;

    static std::map<std::string, StateSetConstructor> *get_stateset_constructors();
    virtual bool gather_union_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const;
    virtual bool gather_intersection_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const;
};


class StateSetVariable : public StateSet
{
public:
    virtual ~StateSetVariable () = 0;
    virtual bool gather_union_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const override;
    virtual bool gather_intersection_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative,
            bool must_be_variable = false) const override;
};


// TODO: can we force each class derived from StateSet to have a fitting constructor (not just inherit, but dedicated implementation)
template<class T>
class StateSetBuilder {
public:
    StateSetBuilder(std::string key) {
        StateSetConstructor constructor = [](std::stringstream &input, Task &task) -> std::unique_ptr<StateSet> {
            return std::unique_ptr<T>(new T(input, task));
        };
        StateSet::get_stateset_constructors()->insert(std::make_pair(key, constructor));
    }

    ~StateSetBuilder() = default;
    StateSetBuilder(const StateSetBuilder<T> &other) = delete;
};

#endif // STATESET_H
