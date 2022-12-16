#ifndef ACTIONAPPLIER_H
#define ACTIONAPPLIER_H

#include "../global_funcs.h"
#include "../task.h"

#include <vector>

namespace mods {
class ActionApplier
{
private:
    VariableOrder input_varorder;
    VariableOrder output_varorder;
    // TODO; this is not really needed but maybe good to store anyways
    const Action &action;
    bool progress;

    // for each entry, check that input_model[first] = second
    std::vector<std::pair<size_t,bool>> to_check;
    // in output_varorder, sets effects and leaves all other vars on false
    Model applied_action_template;
    // for each entry copy input_model[first] to output_model[second]
    std::vector<std::pair<size_t,size_t>> vals_to_copy;
public:
    ActionApplier(const VariableOrder &varorder, const Action &action,
                  bool progress);

    const VariableOrder &get_variable_order();
    bool is_applicable(const Model &model);
    /*
     * Only call this after calling is_applicable. It will apply the action
     * changes without checking if the application conditions are satisfied.
     */
    Model apply(const Model &model);

};
}

#endif // ACTIONAPPLIER_H
