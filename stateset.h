#ifndef STATESET_H
#define STATESET_H

#include "task.h"

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class ProofChecker;
class StateSetVariable;

class StateSet {
public:
    using Constructor = std::function<std::unique_ptr<StateSet>(std::stringstream &input, Task &task)>;
private:
    using ConstructorMap = std::unordered_map<std::string, Constructor>;
    static ConstructorMap &get_stateset_constructors_map();
public:
    virtual ~StateSet() = 0;

    static void register_stateset_constructor(std::string key,
                                       Constructor constructor);
    static Constructor get_stateset_constructor(std::string key);

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
        StateSet::Constructor constructor = [](std::stringstream &input, Task &task) -> std::unique_ptr<StateSet> {
            return std::unique_ptr<T>(new T(input, task));
        };
        StateSet::register_stateset_constructor(key, constructor);
    }

    ~StateSetBuilder() = default;
    StateSetBuilder(const StateSetBuilder<T> &other) = delete;
};

#endif // STATESET_H
