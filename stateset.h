#ifndef STATESET_H
#define STATESET_H

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_set>

#include "task.h"


enum class ConstantType {
    EMPTY,
    GOAL,
    INIT
};

// TODO: can we avoid these forward declarations? I don't think so
class StateSet;
typedef std::function<std::unique_ptr<StateSet>(std::stringstream &input, Task &task)> StateSetConstructor;


class StateSetVariable;
class StateSet
{
public:
    virtual ~StateSet() = 0;

    static std::map<std::string, StateSetConstructor> *get_stateset_constructors();
    virtual bool gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                std::vector<const StateSetVariable *> &positive,
                                std::vector<const StateSetVariable *> &negative,
                                bool must_be_variable = false) const;
    virtual bool gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                std::vector<const StateSetVariable *> &positive,
                                std::vector<const StateSetVariable *> &negative,
                                bool must_be_variable = false) const;
    // TODO: is it problematic to have the formulas passed like this? We could probably also pass the (const?) proofchecker to call get_set_expression)
};


class StateSetVariable : public StateSet
{
public:
    virtual ~StateSetVariable () = 0;
    virtual bool gather_union_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                        std::vector<const StateSetVariable *> &positive,
                                        std::vector<const StateSetVariable *> &negative,
                                        bool must_be_variable = false) const override;
    virtual bool gather_intersection_variables(const std::deque<std::unique_ptr<StateSet>> &formulas,
                                               std::vector<const StateSetVariable *> &positive,
                                               std::vector<const StateSetVariable *> &negative,
                                               bool must_be_variable = false) const override;
};


class StateSetFormalism : public StateSetVariable
{
public:
    virtual bool check_statement_b1(std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right) const = 0;
    virtual bool check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) const = 0;
    virtual bool check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) const = 0;
    virtual bool check_statement_b4(const StateSetFormalism *right, bool left_positive,
                                    bool right_positive) const = 0;

    // TODO: remodel this
    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const = 0; // TODO: this could be covered by is_entailed
    virtual bool is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) const = 0; // TODO: use global varorder
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const = 0; // TODO: use global varorder
    // returns false if no clause with index i exists
    virtual bool get_clause(int i, std::vector<int> &varorder, std::vector<bool> &clause) const = 0; // TODO: remove
    virtual int get_model_count() const = 0;

    virtual const std::vector<int> &get_varorder() const = 0; // TODO: remove


    virtual bool supports_mo() const = 0;
    virtual bool supports_ce() const = 0;
    virtual bool supports_im() const = 0;
    virtual bool supports_me() const = 0;
    virtual bool supports_todnf() const = 0;
    virtual bool supports_tocnf() const = 0;
    virtual bool supports_ct() const = 0;
    virtual bool is_nonsuccint() const = 0;


    /*
     * checks whether stateset is compatible with this (using covariance)
     *  - if yes, return a casted pointer to the subclass of this
     *  - if no, return nullpointer
     */
    virtual const StateSetFormalism *get_compatible(const StateSetVariable *stateset) const = 0;
    /*
     * return a constant formula in the formalism of this (using covariance)
     */
    virtual const StateSetFormalism *get_constant(ConstantType ctype) const = 0;

    // TODO: think about error handling!
    // TOOD: do we need reference? after all we are calling it from a T * formula
    // TODO: const_cast is a HACK; remove!
    template <class T>
    std::vector<T *> convert_to_formalism(std::vector<const StateSetVariable *> &vector, const T *reference) const {
        std::vector<T *>ret;
        ret.reserve(vector.size());
        for (const StateSetVariable *formula : vector) {
            T *element = const_cast<T *>(reference->get_compatible(formula));
            if (!element) {
                std::string msg = "could not convert vector to specific formalism, incompatible formulas!";
                throw std::runtime_error(msg);
            }
            ret.push_back(element);
        }
        return std::move(ret);
    }
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
