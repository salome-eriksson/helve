#ifndef MODSSTATESET_H
#define MODSSTATESET_H

#include "modsformula.h"

#include "../global_funcs.h"
#include "../statesetformalism.h"
#include "../cnf/disjunction.h"

#include <unordered_set>
#include <vector>

namespace mods {
class TaskInformation;

class ModsStateSet : public StateSetFormalism
{
private:
    ModsFormula formula;
    const TaskInformation &task_information;

    cnf::Disjunction get_cnf_disjunction(
            const std::vector<const ModsFormula *> &formulas,
            std::vector<cnf::CNFFormula> &cnf_formula_storage) const;
    bool cnf_disjunction_contains_model(
            const Model &model, const VariableOrder &variable_order,
            const cnf::Disjunction &disjunction) const;
public:
    ModsStateSet(std::stringstream &input, const Task &task);
    ModsStateSet(const VariableOrder &variable_order,
                 std::unordered_set<Model> &&models,
                 const TaskInformation &task_information);

    const ModsFormula *get_formula() const;

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

    // TODO: remodel this
    // expects the model in the varorder of the formula;
    virtual bool is_contained(const Model &model) const override; // TODO: this could be covered by is_entailed
    virtual bool is_implicant(const VariableOrder &varorder, const std::vector<bool> &implicant) const override; // TODO: use global varorder
    virtual bool is_entailed(const VariableOrder &varorder, const std::vector<bool> &clause) const override; // TODO: use global varorder
    // returns false if no clause with index i exists
    virtual bool get_clause(int, VariableOrder &, std::vector<bool> &) const override; // TODO: remove
    virtual int get_model_count() const override;
    virtual const std::vector<unsigned> &get_varorder() const override; // TODO: remove

    virtual bool supports_mo() const override { return true; }
    virtual bool supports_ce() const override { return true; }
    virtual bool supports_im() const override { return true; }
    virtual bool supports_me() const override { return true; }
    virtual bool supports_todnf() const override { return true; }
    virtual bool supports_tocnf() const override { return false; } // TODO: MODS can support this, implement it
    virtual bool supports_ct() const override { return true; }
    virtual bool is_nonsuccint() const override { return true; }

    virtual const ModsStateSet *get_compatible(const StateSetVariable *stateset) const override;
    virtual const ModsStateSet *get_constant(ConstantType ctype) const override;
};
}

#endif // MODSSTATESET_H
