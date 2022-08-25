#ifndef MODSUTIL_H
#define MODSUTIL_H

#include "ssfmods.h"

#include <vector>

class SSFMods;

class ModsUtil {
    friend class SSFMods;
private:
    Task &task;
    SSFMods emptyformula;
    SSFMods trueformula;
    SSFMods initformula;
    SSFMods goalformula;
    std::vector<std::vector<bool>> hex;

    ModsUtil(Task &task);

    struct OtherVarorderFormula {
        SSFMods *formula;
        // tells for each var in formula->vars, where in the extended model this var is
        std::vector<GlobalModelVarOcc> var_occurences;
        // positions of vars that first occured in this formula
        std::vector<int> newvars_pos;

        OtherVarorderFormula(SSFMods *f) {
            formula = f;
            var_occurences.reserve(formula->vars.size());
        }
    };
    /*
     * The global model is over the union of vars occuring in any formula.
     * The first vector<bool> covers the vars of left[0], then each entry i until size-1
     * covers the variables newly introduced in other_left_formulas[i+1]
     * (if other_left_formulas[i] does not contain any new vars, the vector is empt<).
     * The last entry covers the variables occuring on the right side.
     * Note that all formulas on the right side must contain the same vars.
     * Also note that the global_model might point to deleted vectors, but those should never be accessed.
     */
    struct SubsetCheckHelper {
        std::vector<int> varorder;
        std::vector<SSFMods *> same_varorder_left;
        std::vector<SSFMods *> same_varorder_right;
        std::vector<OtherVarorderFormula> other_varorder_left;
        std::vector<OtherVarorderFormula> other_varorder_right;
        GlobalModel global_model;
        std::vector<ModelExtensions> model_extensions;
    };
    bool check_same_vars(std::vector<SSFMods *> &formulas);
    SubsetCheckHelper get_subset_checker_helper(std::vector<int> &varorder,
                                                std::vector<SSFMods *> &left_formulas,
                                                std::vector<SSFMods *> &right_formulas);
    bool is_model_contained(const Model &model, SubsetCheckHelper &helper);
};

#endif // MODSUTIL_H
