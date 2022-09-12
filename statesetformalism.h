#ifndef STATESETFORMALISM_H
#define STATESETFORMALISM_H

#include "stateset.h"
#include "ssvconstant.h"

#include <vector>


class StateSetFormalism : public StateSetVariable
{
public:
    virtual bool check_statement_b1(std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right) const = 0;
    virtual bool check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<size_t> &action_indices) const = 0;
    virtual bool check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<size_t> &action_indices) const = 0;
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

#endif // STATESETFORMALISM_H
