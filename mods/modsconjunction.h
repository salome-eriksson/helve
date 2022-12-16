#ifndef MODSCONJUNCTION_H
#define MODSCONJUNCTION_H

#include "modsformula.h"

#include "../global_funcs.h"

#include <unordered_set>
#include <vector>

namespace mods {
/*
 * TODO: this could be implemented more efficiently. Currently, we always
 * iterate over *all* formulas that have a varorder we haven't seen yet,
 * even if the variables occuring have all been seen already. In these
 * cases it would make more sense to just build the model in that varorder
 * and see if it's contained in the formula.
 * Likewise, if only one or two variables are new, it might be more efficient
 * to just try out those 2-4 combinations. This could be done by inserting
 * a new element into the conjunction that is only over the new variables.
 * A good threshold on whether or not to do that would be whether the new
 * conjunction element would contain less models than the one we would
 * otherwise need to iterate over.
 */
class ModsConjunction
{
private:
    // Denotes (1) the conjunct and (2) the position within its variable order
    using VarPosition = std::pair<size_t,size_t>;

    VariableOrder variable_order;
    // used to store explicit conjunctions created within this construct
    std::vector<ModsFormula> local_formulas;
    std::vector<const ModsFormula *> conjuncts;

    VariableOrder restriction_variable_order;
    ModsFormula restriction_formula;

    /*
     * The following two variables are used to create a model in the variable
     * order of the conjunction.
     * If variable_order_index >= 0, then the variable order is equal to
     * the variable order of conjuncts[variable_order_index], meaning we can
     * directly use the model of this conjunct.
     * Otherwise, variable_order_positions stores for each variable in
     * variable_order, in which conjunct and at which position within it
     * the variable occurs.
     *
     * Example:
     *   variable_order = <4,5,0,2>
     *   variable_order_positions = <<2,5>,<1,8>,<0,3>,<1,0>>
     *   means that
     *   - var 4 occurs in conjunct 2 at position 5
     *   - var 5 occurs in conjunct 1 at position 8
     *   - var 0 occurs in conjunct 0 at position 3
     *   - var 2 occurs in conjunct 1 at position 0
     */
    int variable_order_index;
    std::vector<VarPosition> variable_order_positions;
    // 2^|vars in variable_order not occuring in the conjunction|
    int assignment_amount_of_new_variables;

    /*
     * Stores for each conjunct which vars already occured in previous conjuncts.
     * This is needed for checking whether a combination of models is a
     * consistent overall model. For verifying this, we need to check for each
     * conjunct, if the value of the variables that occured in previous
     * conjunctions agree with the model of the previous conjunct.
     * (Previous here means earlier in the conjuncts vector.)

     * Example:
     *   < <>
     *     <<3,<0,6>>
     *     <<2,<1,4>,<9,<0,5>> >
     *   means that
     *   - var in conjunct 1 at pos 3 already occured in conjunct 0 at pos 6
     *   - var in conjunct 2 at pos 2 already occured in conjunct 1 at pos 4
     *   - var in conjunct 2 at pos 9 already occured in conjunct 0 at pos 5
     *   Note that the first vector must be empty since nothing can be previous
     *   to the first.
     */
    std::vector<std::vector<std::pair<size_t,VarPosition>>> previous_var_positions;

    void build_variable_order_from_conjunction();

    class ModelIterator {
    private:
        const ModsConjunction &conjunction;
        std::vector<std::unordered_set<Model>::const_iterator> iterators;
        int new_variables_assignment; //encoded in binary
        Model model;

        /*
         * Starting from the current iterator positions, find a combination of
         * models that is consistent. We can assume that the combination of the
         * models of all conjuncts up to index-1 are already consistent.
         */
        bool find_consistent_model_from_here(size_t start_index);
        bool check_consistency(size_t index);
        void set_model();
    public:
        ModelIterator(const ModsConjunction &conjunction, bool end);
        const Model &operator *() const;
        ModelIterator &operator++();
        bool operator!=(const ModelIterator &other) const;
        bool operator==(const ModelIterator &other) const;
    };

public:
    ModsConjunction(std::vector<const ModsFormula *> &elements,
                    const VariableOrder &variable_order = {},
                    const VariableOrder &restriction_variable_order = {});
    ModelIterator begin();
    ModelIterator end();

    const VariableOrder &get_variable_order() const;
    void set_restriction(const Model &restriction);
};
}

#endif // MODSCONJUNCTION_H
