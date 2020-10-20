#ifndef SSFHORN_H
#define SSFAHORN_H

#include <forward_list>
#include <unordered_set>

#include "stateset.h"
#include "ssvconstant.h"
#include "task.h"

class SSFHorn;

struct HornConjunctionElement {
    const SSFHorn *formula;
    std::vector<bool> removed_implications;

    HornConjunctionElement(const SSFHorn *formula);
};

typedef std::vector<std::pair<const SSFHorn *,bool>> HornFormulaList;
typedef std::vector<std::pair<std::forward_list<int>,std::forward_list<int>>> VariableOccurences;

class HornUtil {
    friend class SSFHorn;
private:
    Task &task;
    SSFHorn *emptyformula;
    SSFHorn *initformula;
    SSFHorn *goalformula;
    SSFHorn *trueformula;
    HornUtil(Task &task);

    void build_actionformulas();

    /*
     * Perform unit propagation through the conjunction of the formulas, returning whether it is satisfiable or not.
     * Partial assignment serves both as input of a partial assignment, and as output indicating which variables
     * are forced true/false by the conjunction.
     */
    bool simplify_conjunction(std::vector<HornConjunctionElement> &conjuncts, Cube &partial_assignment);
    bool is_restricted_satisfiable(const SSFHorn *formula, Cube &restriction);

    bool conjunction_implies_disjunction(std::vector<SSFHorn *> &conjuncts,
                                         std::vector<SSFHorn *> &disjuncts);
};


/*
 * A Horn Formula consist of the following parts:
 *  - forced true: represents all unit clauses with one positive literal
 *  - forced false: represents all unit clauses with one negative literal
 *  - All nonunit clauses are represented by left_vars and right_side:
 *    - left_vars[i] is the negative literals of clause [i]
 *    - right_side is the positive literal (or -1 if none exists) of clause [i]
 * IMPORTANT: the clauses in left_vars/right_side are expected to contain at least 2 literals!
 *
 * left_sizes and variable_occurences are helper infos for checking satisfiability
 *   - left_sizes stores for each clause how many negative literals it contains
 *   - variable occurences stores for each variable, in which clause it appears
 *     (the first unordered_set is for negative occurence, the second for positive)
 */
class SSFHorn : public StateSetFormalism
{
    friend class HornUtil;
private:
    std::vector<std::vector<int>> left_vars;
    std::vector<int> left_sizes;
    std::vector<int> right_side;
    VariableOccurences variable_occurences;
    std::vector<int> forced_true;
    std::vector<int> forced_false;
    int varamount;
    std::vector<int> varorder;

    static HornUtil *util;

    /*
     * Private constructors can only be called by SSFHorn or HornUtil,
     * meaning the static member SSFHorn::util is already initialized
     * and we thus do not need to worry about initializing it.
     */
    // this constructor is used for setting up the formulas in util
    SSFHorn(const std::vector<std::pair<std::vector<int>,int>> &clauses, int varamount);
    // used for getting a simplified conjunction of several (possibly primed) formulas
    SSFHorn(std::vector<SSFHorn *> &formulas);
    SSFHorn(const SSFHorn &other, const Action &action, bool progression);
    void simplify();
public:
    // TODO: this is currently only used for a dummy initialization
    SSFHorn(Task &task);
    SSFHorn(std::stringstream &input, Task &task);
    virtual ~SSFHorn() {}

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
    virtual bool supports_todnf() const { return false; }
    virtual bool supports_tocnf() const { return false; }
    virtual bool supports_ct() const { return false; }
    virtual bool is_nonsuccint() const { return false; }

    // expects the model in the varorder of the formula;
    virtual bool is_contained(const std::vector<bool> &model) const ;
    virtual bool is_implicant(const std::vector<int> &vars, const std::vector<bool> &implicant) const;
    virtual bool is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const;
    virtual bool get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) const;
    virtual int get_model_count() const;

    virtual const std::vector<int> &get_varorder() const;

    int get_size() const;
    int get_varamount() const;
    int get_left(int index) const;
    const std::vector<int> &get_left_sizes() const;
    const std::vector<int> &get_left_vars(int index) const;
    int get_right(int index) const;
    const std::vector<int> &get_right_sides() const;
    const std::forward_list<int> &get_variable_occurence_left(int var) const;
    const std::forward_list<int> &get_variable_occurence_right(int var) const;
    const std::vector<int> & get_forced_true() const;
    const std::vector<int> & get_forced_false() const;
    const bool is_satisfiable() const;
    void dump() const;

    virtual const SSFHorn *get_compatible(const StateSetVariable *stateset) const override;
    virtual const SSFHorn *get_constant(ConstantType ctype) const override;
};

#endif // SSFHORN_H
