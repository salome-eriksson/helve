#include "actionapplier.h"

#include <algorithm>
#include <unordered_map>

//TODO: remove
#include <iostream>

namespace mods {
ActionApplier::ActionApplier(const VariableOrder& varorder,
                             const Action &action, bool progress)
    : input_varorder(varorder), action(action), progress(progress) {

    std::unordered_map<unsigned,size_t> input_varorder_map;
    for (size_t i = 0; i < input_varorder.size(); ++i) {
        input_varorder_map[input_varorder[i]] = i;
    }

    std::unordered_map<unsigned,bool> preconditions;
    for (int var : action.pre) {
        preconditions[var] = true;
    }

    std::unordered_map<unsigned,bool> effects;
    // unless the precondition is altered by the effect (done below),
    // it needs to be true in the successor
    for (int var :  action.pre) {
        effects[var] = true;
    }
    for (size_t i = 0; i < action.change.size(); ++i) {
        if (action.change[i] == 1) {
            effects[i] = true;
        } else if (action.change[i] == -1) {
            effects[i] = false;
        }
    }

    std::unordered_map<unsigned,bool> &before = progress ? preconditions : effects;
    std::unordered_map<unsigned,bool> &after = progress ? effects : preconditions;

    // check the before values present in the input
    for (std::pair<unsigned,bool> entry : before) {
        if (input_varorder_map.count(entry.first)> 0) {
            to_check.push_back({input_varorder_map[entry.first], entry.second});
        }
    }

    /*
     * The ouput must contain:
     *  (i) all vars that are set by the action (=have an after value)
     *    - (a) vars in input
     *    - (b) vars not in input
     *  (ii) all vars in input that have no before value and no after value
     *  Specifically, this means that variables in input that have a before
     *  but no after value are deleted. Note that this can only happen in
     *  regression (i.e. variables that are in the effect but not in the
     *  precondition), since in progression, all effects in the precondition
     *  retain their value in the successor unless they are explicitly set
     *  in the effect.
     */
    for(size_t i = 0; i < input_varorder.size(); ++i) {
        unsigned var = input_varorder[i];
        if(before.count(var) > 0 && after.count(var) == 0) {
            continue;
        }
        output_varorder.push_back(var);
        if(after.count(var) > 0) { //(i)(a)
            applied_action_template.push_back(after[var]);
            after.erase(var);
        } else { //(ii), since vars with no after but a before value are caught above
            applied_action_template.push_back(false);
            vals_to_copy.push_back({i, output_varorder.size()-1});
        }
    }

    for (std::pair<unsigned,bool> entry : after) { // (i)(b)
        output_varorder.push_back(entry.first);
        applied_action_template.push_back(entry.second);
    }
}

const VariableOrder &ActionApplier::get_variable_order() {
    return output_varorder;
}

bool ActionApplier::is_applicable(const Model &model) {
    for (std::pair<size_t, bool> check : to_check) {
        if (model[check.first] != check.second) {
            return false;
        }
    }
    return true;
}

Model ActionApplier::apply(const Model &model) {
    Model ret = applied_action_template;
    for (std::pair<size_t,size_t> copy : vals_to_copy) {
        ret[copy.second] = model[copy.first];
    }
    return ret;
}
}
