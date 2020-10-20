#ifndef SSFEXPLICIT_H
#define SSFEXPLICIT_H

#include "stateset.h"
#include "ssvconstant.h"
#include "task.h"

#include "cuddObj.hh"
#include <memory>
#include <unordered_set>

class ExplicitUtil;
typedef std::vector<bool> Model;
typedef std::vector<Model> ModelExtensions;
typedef std::vector<const Model *>GlobalModel;
typedef std::pair<int,int> GlobalModelVarOcc;

class SSFExplicit : public StateSetFormalism
{
    friend class ExplicitUtil;
private:
    static std::unique_ptr<ExplicitUtil> util;
    std::vector<int> vars;
    std::unordered_set<Model> models;
    std::unordered_set<Model>::iterator model_it;
    SSFExplicit(std::vector<int> &&vars, std::unordered_set<Model> &&models);
    // all formulas need to have same varorder
    SSFExplicit(std::vector<int> &varorder, std::vector<SSFExplicit *>&disjuncts);
public:
    SSFExplicit();
    SSFExplicit(std::stringstream &input, Task &task);
    virtual ~SSFExplicit() {}

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

    virtual const SSFExplicit *get_compatible(const StateSetVariable *stateset) const override;
    virtual const SSFExplicit *get_constant(ConstantType ctype) const override;

    /*
     * model is expected to have the same varorder (ie we need no transformation).
     * However, the values at position missing_vars can be discarded.
     * Instead we need to consider each combination of the missing vars.
     * Returns those combinations where the (filled out) model is contained.
     */
    std::vector<Model> get_missing_var_values(
            Model &model, const std::vector<int> &missing_vars_pos) const;
};

class ExplicitUtil {
    friend class SSFExplicit;
private:
    Task &task;
    SSFExplicit emptyformula;
    SSFExplicit trueformula;
    SSFExplicit initformula;
    SSFExplicit goalformula;
    std::vector<std::vector<bool>> hex;

    ExplicitUtil(Task &task);

    struct OtherVarorderFormula {
        SSFExplicit *formula;
        // tells for each var in formula->vars, where in the extended model this var is
        std::vector<GlobalModelVarOcc> var_occurences;
        // positions of vars that first occured in this formula
        std::vector<int> newvars_pos;

        OtherVarorderFormula(SSFExplicit *f) {
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
        std::vector<SSFExplicit *> same_varorder_left;
        std::vector<SSFExplicit *> same_varorder_right;
        std::vector<OtherVarorderFormula> other_varorder_left;
        std::vector<OtherVarorderFormula> other_varorder_right;
        GlobalModel global_model;
        std::vector<ModelExtensions> model_extensions;
    };
    bool check_same_vars(std::vector<SSFExplicit *> &formulas);
    SubsetCheckHelper get_subset_checker_helper(std::vector<int> &varorder,
                                                std::vector<SSFExplicit *> &left_formulas,
                                                std::vector<SSFExplicit *> &right_formulas);
    bool is_model_contained(const Model &model, SubsetCheckHelper &helper);
};

#endif // SSFEXPLICIT_H
