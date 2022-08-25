#ifndef SSFMODS_H
#define SSFMODS_H

#include "../stateset.h"
#include "../task.h"

#include <sstream>
#include <unordered_set>
#include <vector>

class ModsUtil;
typedef std::vector<bool> Model;
typedef std::vector<Model> ModelExtensions;
typedef std::vector<const Model *>GlobalModel;
typedef std::pair<int,int> GlobalModelVarOcc;

class SSFMods : public StateSetFormalism
{
    friend class ModsUtil;
private:
    static std::unique_ptr<ModsUtil> util;
    std::vector<int> vars;
    std::unordered_set<Model> models;
    std::unordered_set<Model>::iterator model_it;
    SSFMods(std::vector<int> &&vars, std::unordered_set<Model> &&models);
    // all formulas need to have same varorder
    SSFMods(std::vector<int> &varorder, std::vector<SSFMods *>&disjuncts);
public:
    SSFMods();
    SSFMods(std::stringstream &input, Task &task);
    virtual ~SSFMods() {}

    virtual bool check_statement_b1(std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right) const;
    virtual bool check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) const;
    virtual bool check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                    std::vector<const StateSetVariable *> &left,
                                    std::vector<const StateSetVariable *> &right,
                                    std::unordered_set<int> &action_indices) const;
    virtual bool check_statement_b4(const StateSetFormalism *right,
                                    bool left_positive, bool right_positive) const;

    virtual bool supports_mo() const { return true; }
    virtual bool supports_ce() const { return true; }
    virtual bool supports_im() const { return true; }
    virtual bool supports_me() const { return true; }
    virtual bool supports_todnf() const { return true; }
    virtual bool supports_tocnf() const { return false; }
    virtual bool supports_ct() const { return true; }
    virtual bool is_nonsuccint() const { return true; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const ;
    virtual bool is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant) const;
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const;
    virtual bool get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) const;
    virtual int get_model_count() const;

    virtual const std::vector<int> &get_varorder() const;

    virtual const SSFMods *get_compatible(const StateSetVariable *stateset) const override;
    virtual const SSFMods *get_constant(ConstantType ctype) const override;

    /*
     * model is expected to have the same varorder (ie we need no transformation).
     * However, the values at position missing_vars can be discarded.
     * Instead we need to consider each combination of the missing vars.
     * Returns those combinations where the (filled out) model is contained.
     */
    std::vector<Model> get_missing_var_values(
            Model &model, const std::vector<int> &missing_vars_pos) const;
};

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

#endif // SSFMODS_H
