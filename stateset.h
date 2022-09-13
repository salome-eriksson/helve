#ifndef STATESET_H
#define STATESET_H

#include "task.h"

#include <deque>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ProofChecker;
class StateSet;
class StateSetVariable;
using StateSetConstructor = std::function<std::unique_ptr<StateSet>(std::stringstream &input, Task &task)>;
using StateSetConstructorMap = std::unordered_map<std::string, StateSetConstructor>;

class StateSet {
private:
    static StateSetConstructorMap &get_stateset_constructors_map();
public:
    virtual ~StateSet() = 0;

    static void register_stateset_constructor(std::string key,
                                       StateSetConstructor constructor);
    static StateSetConstructor get_stateset_constructor(std::string key);

    /*
     * Called on state sets of the form \bigcup L, where L is a StateSetVariable
     * or a negation thereof. Collects all StateSetVariables occuring
     * positively (negatively) in positive (negative).
     * Returns false if the state set does not fit the required form.
     *
     * Default implementation returns false.
     */
    virtual bool gather_union_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative) const;

    /*
     * Called on state sets of the form \bigcap L, where L is a StateSetVariable
     * or a negation thereof. Collects all StateSetVariables occuring
     * positively (negatively) in positive (negative).
     * Returns false if the state set does not fit the required form.
     *
     * Default implementation returns false.
     */
    virtual bool gather_intersection_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative) const;
};


class StateSetVariable : public StateSet
{
public:
    virtual ~StateSetVariable () = 0;
    virtual bool gather_union_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative) const override;
    virtual bool gather_intersection_variables(
            const ProofChecker &proof_checker,
            std::vector<const StateSetVariable *> &positive,
            std::vector<const StateSetVariable *> &negative) const override;
};


template<class T>
class StateSetBuilder {
public:
    StateSetBuilder(std::string key) {
        StateSetConstructor constructor = [](std::stringstream &input, Task &task) -> std::unique_ptr<StateSet> {
            return std::unique_ptr<T>(new T(input, task));
        };
        StateSet::register_stateset_constructor(key, constructor);
    }

    ~StateSetBuilder() = default;
    StateSetBuilder(const StateSetBuilder<T> &other) = delete;
};

#endif // STATESET_H
