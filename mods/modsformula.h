#ifndef MODSFORMULA_H
#define MODSFORMULA_H

#include "../global_funcs.h"
#include "../cnf/cnfformula.h"

#include <sstream>
#include <unordered_set>

namespace mods {
class ModsFormula
{
private:
    VariableOrder variable_order;
    std::unordered_set<Model> models;

    static std::vector<bool> get_bool_vector_from_hex(std::string hex, size_t length);
public:
    /*
     * Creates a MODS formula over the given variable order. Expects all models
     * to have the same length as variable_order (checked with assertions).
     */
    ModsFormula(const VariableOrder &variable_order, std::unordered_set<Model> &&models);
    /*
     * Creates a conjunction or disjunction of a set of MODS formula.
     * Expects at least two formulas and that all formulas share the same
     * variable order and task information (all checked with assertions).
     */
    ModsFormula(std::vector<const ModsFormula *> elements, bool conjunction);
    ModsFormula(std::stringstream &input);

    /*
     * Merges multiple elements with the same variable order into a new
     * ModsStateSet (stored in new_sets), either as conjunction or disjunction,
     * and modifies the elements vector so that it points to the merged objects.
     * Trivial elements ("true" for conjunctions and "false" for disjunctions)
     * are ignored, and if the con/disjunction is trivial ("false"/"true"
     * occurs) all elements are removed and instead a single "false"/"true"
     * is stored in elements.
     */
    static void aggregate_by_varorder(std::vector<const ModsFormula *> &elements,
                                      std::vector<ModsFormula> &new_formulas,
                                      bool conjunction);

    cnf::CNFFormula transform_to_cnf() const;

    bool is_valid() const;
    bool is_unsatisfiable() const;
    bool contains(const Model &model) const;
    int get_model_count() const;
    const VariableOrder &get_variable_order() const;
    std::unordered_set<Model>::const_iterator get_cbegin() const;
    std::unordered_set<Model>::const_iterator get_cend() const;

    void dump() const;
};
}

#endif // MODSFORMULA_H
