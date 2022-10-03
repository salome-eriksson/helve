#ifndef HORNSTATESET_H
#define HORNSTATESET_H

#include "horntypeformula.h"

#include "../statesetformalism.h"

namespace horn {
class TaskInformation;

class HornStateSet : public StateSetFormalism
{
private:
    HornTypeFormula formula;
    const TaskInformation &task_information;

    static std::vector<const HornTypeFormula *> get_formulas(
            const std::vector<const HornStateSet *> &state_sets);

    // TODO: this should be part of the Action class (once other setformalisms are reworked)
    std::vector<Literal> get_precondition_literals(const Action &action) const;
    std::vector<Literal> get_effect_literals(const Action &action) const;

public:
    HornStateSet(std::stringstream &input, const Task &task);
    virtual bool check_statement_b1(
            std::vector<const StateSetVariable *> &left,
            std::vector<const StateSetVariable *> &right) const override;
    virtual bool check_statement_b2(
            std::vector<const StateSetVariable *> &progress,
            std::vector<const StateSetVariable *> &left,
            std::vector<const StateSetVariable *> &right,
            std::unordered_set<size_t> &action_indices) const override;
    virtual bool check_statement_b3(
            std::vector<const StateSetVariable *> &regress,
            std::vector<const StateSetVariable *> &left,
            std::vector<const StateSetVariable *> &right,
            std::unordered_set<size_t> &action_indices) const override;
    virtual bool check_statement_b4(
            const StateSetFormalism *right, bool left_positive,
            bool right_positive) const override;

    const HornTypeFormula *get_formula() const;


    // TODO: remodel this
    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &) const override; // TODO: this could be covered by is_entailed
    virtual bool is_implicant(const std::vector<int> &, const std::vector<bool> &) const override; // TODO: use global varorder
    virtual bool is_entailed(const std::vector<int> &, const std::vector<bool> &) const override; // TODO: use global varorder
    // returns false if no clause with index i exists
    virtual bool get_clause(int, std::vector<int> &, std::vector<bool> &) const override; // TODO: remove
    virtual int get_model_count() const override;
    virtual const std::vector<int> &get_varorder() const override; // TODO: remove


    virtual bool supports_mo() const override { return true; }
    virtual bool supports_ce() const override { return true; }
    virtual bool supports_im() const override { return true; }
    virtual bool supports_me() const override { return true; }
    virtual bool supports_todnf() const override { return false; }
    virtual bool supports_tocnf() const override { return true; }
    virtual bool supports_ct() const override { return false; }
    virtual bool is_nonsuccint() const override { return false; }

    virtual const HornStateSet *get_compatible(const StateSetVariable *stateset) const override;
    virtual const HornStateSet *get_constant(ConstantType ctype) const override;

    std::string print();

};
}

#endif // HORNSTATESET_H
