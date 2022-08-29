#include "ssfhorn.h"

#include "hornutil.h"

#include "../global_funcs.h"
#include "../ssvconstant.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <string>

HornUtil *SSFHorn::util = nullptr;

SSFHorn::SSFHorn(const std::vector<std::pair<std::vector<int>, int> > &clauses, int varamount)
    : variable_occurences(varamount), varamount(varamount) {

    for(auto clause : clauses) {
        if (clause.first.size() == 0) {
            assert(clause.second != -1);
            forced_true.push_back(clause.second);
            continue;
        } else if (clause.first.size() == 1 && clause.second == -1) {
            forced_false.push_back(clause.first[0]);
            continue;
        }
        left_vars.push_back(clause.first);
        right_side.push_back(clause.second);
        for(int var : clause.first) {
            variable_occurences[var].first.push_front(left_vars.size()-1);
        }
        if(clause.second != -1) {
            variable_occurences[clause.second].second.push_front(left_vars.size()-1);
        }
        left_sizes.push_back(clause.first.size());
    }
    simplify();
    varorder.reserve(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder.push_back(var);
    }
}

SSFHorn::SSFHorn(std::vector<SSFHorn *> &formulas) {
    std::vector<HornConjunctionElement> elements;
    elements.reserve(formulas.size());
    for (SSFHorn *f : formulas) {
        elements.emplace_back(HornConjunctionElement(f));
    }
    Cube partial_assignments;
    bool satisfiable = util->simplify_conjunction(elements, partial_assignments);
    varamount = partial_assignments.size();
    variable_occurences.resize(varamount);

    if (!satisfiable) {
        variable_occurences.clear();
        variable_occurences.resize(varamount);
        forced_true.clear();
        forced_true.push_back(0);
        forced_false.clear();
        forced_false.push_back(0);
    } else {
        // set forced true/false
        for (size_t i = 0; i < partial_assignments.size(); ++i) {
            if (partial_assignments[i] == 0) {
                forced_false.push_back(i);
            } else if (partial_assignments[i] == 1) {
                forced_true.push_back(i);
            }
        }
        int implication_amount = 0;
        for (HornConjunctionElement elem : elements) {
            for (bool val : elem.removed_implications) {
                if (!val) {
                    implication_amount++;
                }
            }
        }
        left_sizes.reserve(implication_amount);
        left_vars.reserve(implication_amount);
        right_side.reserve(implication_amount);

        // iterate over all formulas and insert each simplified clause one by one
        for (HornConjunctionElement elem : elements) {
            // iterate over all implications of the current formula
            for (int i = 0; i < elem.formula->get_size(); ++i) {
                // implication is removed - jump over it
                if (elem.removed_implications[i] ) {
                    continue;
                }
                std::vector<int> left;
                for (int var : elem.formula->get_left_vars(i)) {
                    // var is assigned - jump over it
                    if (partial_assignments[var] != 2) {
                        continue;
                    }
                    left.push_back(var);
                    variable_occurences[var].first.push_front(left_vars.size());
                }
                left_sizes.push_back(left.size());
                left_vars.push_back(std::move(left));

                int right_var = elem.formula->get_right(i);
                if (right_var == -1) {
                    right_side.push_back(-1);
                    continue;
                }
                // right var is assigned in partial assignment - no right var
                if(partial_assignments[right_var] != 2) {
                    right_side.push_back(-1);
                } else {
                    variable_occurences[right_var].second.push_front(right_side.size());
                    right_side.push_back(right_var);
                }
            } // end iterate over implications of the current formula
        } // end iterate over all formulas
    } // end else block of !satisfiable
    varorder.reserve(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder.push_back(var);
    }
}

SSFHorn::SSFHorn(const SSFHorn &other, const Action &action, bool progression)
    : left_vars(other.left_vars), left_sizes(other.left_sizes), right_side(other.right_side),
      forced_true(other.forced_true), forced_false(other.forced_false), varamount(2*action.change.size()) {
    variable_occurences.resize(action.change.size()*2);

    if (progression) {
        for (int var : action.pre) {
            forced_true.push_back(var);
        }
    } else {
        for (size_t var = 0; var < action.change.size(); ++var) {
            if (action.change[var] == 1) {
                forced_true.push_back(var);
            } else if (action.change[var] == -1) {
                forced_false.push_back(var);
            }
        }
    }

    //shift
    for (size_t i = 0; i < forced_true.size(); ++i) {
        if (action.change[forced_true[i]] != 0) {
            forced_true[i] += action.change.size();
        }
    }
    for (size_t i = 0; i < forced_false.size(); ++i) {
        if (action.change[forced_false[i]] != 0) {
            forced_false[i] += action.change.size();
        }
    }
    for (size_t var = 0; var < other.variable_occurences.size(); ++var) {
        if (action.change[var] == 0) {
            variable_occurences[var].first = other.variable_occurences[var].first;
            variable_occurences[var].second = other.variable_occurences[var].second;
        } else {
            //shift vars
            variable_occurences[var + action.change.size()].first = other.variable_occurences[var].first;
            variable_occurences[var + action.change.size()].second = other.variable_occurences[var].second;
            for (int impl : other.variable_occurences[var].first) {
                for (size_t i = 0; i < left_vars[impl].size(); ++i) {
                    if (left_vars[impl][i] == var) {
                        left_vars[impl][i] += action.change.size();
                        break;
                    }
                }
            }
            for (int impl : other.variable_occurences[var].second) {
                right_side[impl] += action.change.size();
            }
        }
    }

    // apply actions
    if (progression) {
        for (size_t var = 0; var < action.change.size(); ++var) {
            if (action.change[var] == 1) {
                forced_true.push_back(var);
            } else if (action.change[var] == -1) {
                forced_false.push_back(var);
            }
        }
    } else {
        for (int var : action.pre) {
            forced_true.push_back(var);
        }
    }

    simplify();
    varorder.clear();
    varorder.resize(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder[var] = var;
    }
}


SSFHorn::SSFHorn(Task &task) {
    if (util == nullptr) {
        util = new HornUtil(task);
    }
}

SSFHorn::SSFHorn(std::stringstream &input, Task &task) {
    // parsing
    std::string word;
    int clausenum;
    input >> word;
    if (word.compare("p") != 0) {
        std::cerr << "Invalid DIMACS format" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    input >> word;
    if (word.compare("cnf") != 0) {
        std::cerr << "DIMACS format" << word << "not recognized" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    input >> varamount;
    input >> clausenum;
    variable_occurences.resize(varamount);

    int count = 0;
    for(int i = 0; i < clausenum; ++i) {
        int var;
        input >> var;
        std::vector<int> left;
        int right = -1;
        while(var != 0) {
            if (var < 0) {
                left.push_back(-1-var);
            } else {
                if (right != -1) {
                    std::cerr << "Invalid Horn formula" << std::endl;
                    exit_with(ExitCode::CRITICAL_ERROR);
                }
                right = var-1;
            }
            input >> var;
        }
        if (left.size() == 0) {
            if (right == -1) {
                std::cerr << "Invalid Horn formula" << std::endl;
                exit_with(ExitCode::CRITICAL_ERROR);
            }
            forced_true.push_back(right);
        } else if (left.size() == 1 && right == -1) {
            forced_false.push_back(left[0]);
        } else {
            for (int var : left) {
                variable_occurences[var].first.push_front(count);
            }
            if (right != -1) {
                variable_occurences[var].second.push_front(count);
            }
            left_sizes.push_back(left.size());
            left_vars.push_back(std::move(left));
            right_side.push_back(right);
            count++;
        }
    }
    input >> word;
    if(word.compare(";") != 0) {
        std::cerr << "HornFormula syntax wrong" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    // create util if this is the first horn formula
    if (util == nullptr) {
        util = new HornUtil(task);
    }
    simplify();

    varorder.reserve(varamount);
    for (size_t var = 0; var < varamount; ++var) {
        varorder.push_back(var);
    }
}

void SSFHorn::simplify() {
    // call simplify_conjunction to get a partial assignment and implications to remove
    std::vector<HornConjunctionElement> tmpvec(1,HornConjunctionElement(this));
    Cube assignments(varamount, 2);
    bool satisfiable = util->simplify_conjunction(tmpvec, assignments);

    if(!satisfiable) {
        left_vars.clear();
        right_side.clear();
        variable_occurences.clear();
        variable_occurences.resize(varamount);
        forced_true.clear();
        forced_true.push_back(0);
        forced_false.clear();
        forced_false.push_back(0);
    } else {
        // set forced true/false and clear up variable occurences
        forced_true.clear();
        forced_false.clear();
        assert (assignments.size() == varamount);
        for (size_t var = 0; var < assignments.size(); ++var) {
            // not assigned
            if (assignments[var] == 2) {
                continue;
            }
            // assigned - remove from implications
            for(int impl : variable_occurences[var].first) {
                left_sizes[impl]--;
                left_vars[impl].erase(std::remove(left_vars[impl].begin(),
                                                  left_vars[impl].end(), var),
                                      left_vars[impl].end());
            }
            for(int impl : variable_occurences[var].second) {
                right_side[impl] = -1;
            }
            if (assignments[var] == 0) {
                forced_false.push_back(var);
            } else if (assignments[var] == 1) {
                forced_true.push_back(var);
            }
        }

        // remove unneeded implications
        std::vector<bool> &implications_to_remove = tmpvec[0].removed_implications;
        int front = 0;
        int back = implications_to_remove.size()-1;

        while (front < back) {
            // back points to the last implication that should not be removed
            while (front < back && implications_to_remove[back]) {
                back--;
            }
            // front points to the next implication to remove
            while (front < back &&!implications_to_remove[front]) {
                front++;
            }
            if (front >= back) {
                break;
            }
            // move the implication at back to front
            right_side[front] = right_side[back];
            left_vars[front] = std::move(left_vars[back]);
            left_sizes[front] = left_sizes[back];
            front++;
            back--;
        }

        int newsize = 0;
        for (bool val : implications_to_remove) {
            if (!val) {
                newsize++;
            }
        }
        right_side.resize(newsize);
        left_vars.resize(newsize);
        left_sizes.resize(newsize);
        right_side.shrink_to_fit();
        left_vars.shrink_to_fit();
        left_sizes.shrink_to_fit();

    }
    variable_occurences.clear();
    variable_occurences.resize(varamount);
    for (size_t i = 0; i < left_vars.size(); ++i) {
        for (int var : left_vars[i]) {
            variable_occurences[var].first.push_front(i);
        }
        int var = right_side[i];
        if (var != -1) {
            variable_occurences[var].second.push_front(i);
        }
    }
}

const std::forward_list<int> &SSFHorn::get_variable_occurence_left(int var) const {
    return variable_occurences[var].first;
}

const std::forward_list<int> &SSFHorn::get_variable_occurence_right(int var) const {
    return variable_occurences[var].second;
}

int SSFHorn::get_size() const {
    return left_vars.size();
}

int SSFHorn::get_varamount() const {
    return varamount;
}

int SSFHorn::get_left(int index) const {
    return left_sizes[index];
}

const std::vector<int> &SSFHorn::get_left_sizes() const {
    return left_sizes;
}

int SSFHorn::get_right(int index) const {
    return right_side[index];
}

const std::vector<int> &SSFHorn::get_right_sides() const {
    return right_side;
}

const std::vector<int> &SSFHorn::get_forced_true() const {
    return forced_true;
}

const std::vector<int> &SSFHorn::get_forced_false() const {
    return forced_false;
}

const std::vector<int> &SSFHorn::get_left_vars(int index) const {
    return left_vars[index];
}

void SSFHorn::dump() const {
    std::cout << "forced true: ";
    for(int i = 0; i < forced_true.size(); ++i) {
        std::cout << forced_true[i] << " ";
    }
    std::cout << ", forced false: ";
    for(int i = 0; i < forced_false.size(); ++i) {
        std::cout << forced_false[i] << " ";
    }
    std::cout << std::endl;
    for(int i = 0; i < left_vars.size(); ++i) {
        for(int j = 0; j < left_vars[i].size(); ++j) {
            std::cout << left_vars[i][j] << " ";
        }
        std::cout << "," << right_side[i] << "(" << left_sizes[i] << ")"<< "|";
    }
    std::cout << std::endl;
    std::cout << "occurences: ";
    for(int i = 0; i < variable_occurences.size(); ++i) {
        std::cout << "[" << i << ":";
        for(auto elem : variable_occurences[i].first) {
            std::cout << elem << " ";
        }
        std::cout << "|";
        for(auto elem : variable_occurences[i].second) {
            std::cout << elem << " ";
        }
        std::cout << "] ";
    }
    std::cout << std::endl;
}

bool SSFHorn::check_statement_b1(std::vector<const StateSetVariable *> &left,
                                 std::vector<const StateSetVariable *> &right) const {

    std::vector<SSFHorn *> horn_formulas_left = convert_to_formalism<SSFHorn>(left, this);
    std::vector<SSFHorn *> horn_formulas_right = convert_to_formalism<SSFHorn>(right, this);

    return util->conjunction_implies_disjunction(horn_formulas_left, horn_formulas_right);
}

bool SSFHorn::check_statement_b2(std::vector<const StateSetVariable *> &progress,
                                 std::vector<const StateSetVariable *> &left,
                                 std::vector<const StateSetVariable *> &right,
                                 std::unordered_set<int> &action_indices) const {
    std::vector<SSFHorn *> horn_formulas_left = convert_to_formalism<SSFHorn>(left, this);
    std::vector<SSFHorn *> horn_formulas_right = convert_to_formalism<SSFHorn>(right, this);
    std::vector<SSFHorn *> horn_formulas_prog = convert_to_formalism<SSFHorn>(progress, this);

    SSFHorn *prog_singular;
    SSFHorn prog_dummy(util->task);
    if (horn_formulas_prog.size() > 1) {
        prog_dummy = SSFHorn(horn_formulas_prog);
        prog_singular = &prog_dummy;
    } else {
        prog_singular = horn_formulas_prog[0];
    }
    SSFHorn *left_singular = nullptr;
    SSFHorn left_dummy(util->task);
    if (!horn_formulas_left.empty()) {
        if (horn_formulas_left.size() > 1) {
            left_dummy = SSFHorn(horn_formulas_left);
            left_singular = &left_dummy;
        } else {
            left_singular = horn_formulas_left[0];
        }
    }

    std::vector<SSFHorn *> vec;
    if (left_singular) {
        vec.push_back(left_singular);
    }
    for (int a : action_indices) {
        SSFHorn prog_applied(*prog_singular, util->task.get_action(a), true);
        vec.push_back(&prog_applied);
        if (!util->conjunction_implies_disjunction(vec, horn_formulas_right)) {
            return false;
        }
        vec.pop_back();
    }
    return true;
}

bool SSFHorn::check_statement_b3(std::vector<const StateSetVariable *> &regress,
                                 std::vector<const StateSetVariable *> &left,
                                 std::vector<const StateSetVariable *> &right,
                                 std::unordered_set<int> &action_indices) const {
    std::vector<SSFHorn *> horn_formulas_left = convert_to_formalism<SSFHorn>(left, this);
    std::vector<SSFHorn *> horn_formulas_right = convert_to_formalism<SSFHorn>(right,this);
    std::vector<SSFHorn *> horn_formulas_reg  = convert_to_formalism<SSFHorn>(regress, this);

    SSFHorn *reg_singular;
    SSFHorn reg_dummy(util->task);
    if (horn_formulas_reg.size() > 1) {
        reg_dummy = SSFHorn(horn_formulas_reg);
        reg_singular = &reg_dummy;
    } else {
        reg_singular = horn_formulas_reg[0];
    }
    SSFHorn *left_singular = nullptr;
    SSFHorn left_dummy(util->task);
    if (!horn_formulas_left.empty()) {
        if (horn_formulas_left.size() > 1) {
            left_dummy = SSFHorn(horn_formulas_left);
            left_singular = &left_dummy;
        } else {
            left_singular = horn_formulas_left[0];
        }
    }

    std::vector<SSFHorn *> vec;
    if (left_singular) {
        vec.push_back(left_singular);
    }
    for (int a : action_indices) {
        SSFHorn reg_applied(*reg_singular, util->task.get_action(a), false);
        vec.push_back(&reg_applied);
        if (!util->conjunction_implies_disjunction(vec, horn_formulas_right)) {
            return false;
        }
        vec.pop_back();
    }
    return true;
}

bool SSFHorn::check_statement_b4(const StateSetFormalism *right_const, bool left_positive, bool right_positive) const {
    // TODO: HACK - remove!
    StateSetFormalism *right = const_cast<StateSetFormalism *>(right_const);
    if (left_positive && right_positive) {
        if (right->supports_tocnf()) {
            int count = 0;
            std::vector<int> varorder;
            std::vector<bool> clause;
            while (right->get_clause(count, varorder, clause)) {
                if (!is_entailed(varorder, clause)) {
                    return false;
                }
                count++;
            }
            return true;
        } else if (right->is_nonsuccint()) {
            const std::vector<int> sup_varorder = right->get_varorder();
            std::vector<bool> model(sup_varorder.size());
            std::vector<int> var_transform(varorder.size(), -1);
            std::vector<int> vars_to_fill;
            for (size_t i = 0; i < varorder.size(); ++i) {
                auto pos_it = std::find(sup_varorder.begin(), sup_varorder.end(), varorder[i]);
                if (pos_it == sup_varorder.end()) {
                    std::cerr << "mixed representation subset check not possible" << std::endl;
                    return false;
                }
                var_transform[i] = std::distance(sup_varorder.begin(), pos_it);
            }
            for (size_t i = 0; i < sup_varorder.size(); ++i) {
                if(sup_varorder[i] >= varorder.size()) {
                    vars_to_fill.push_back(i);
                }
            }

            std::vector<bool> mark(varorder.size(),false);
            std::vector<int> old_solution(varorder.size(),2);

            bool solution_found = util->is_restricted_satisfiable(this, old_solution);
            while(solution_found) {
                for(size_t i = 0; i < old_solution.size(); ++i) {
                    if(old_solution[i] == 2) {
                        old_solution[i] = 0;
                    }
                }

                for (size_t i = 0; i < varorder.size(); ++i) {
                    if (old_solution[i] == 1) {
                        model[var_transform[i]] = true;
                    } else {
                        model[var_transform[i]] = false;
                    }
                }

                for (int count = 0; count < (1 << vars_to_fill.size()); ++count) {
                    for (size_t i = 0; i < vars_to_fill.size(); ++i) {
                        model[vars_to_fill[i]] = ((count >> i) % 2 == 1);
                    }
                    if (!right->is_contained(model)) {
                        return false;
                    }
                }

                // get next solution
                solution_found = false;
                for(size_t i = varorder.size()-1; i >= 0; --i) {
                    if (!mark[i]) {
                        old_solution[i] = 1 - old_solution[i];
                        if (util->is_restricted_satisfiable(this, old_solution)) {
                            solution_found = true;
                            mark[i] = true;
                            for (int j = i+1; j < mark.size(); ++j) {
                                mark[j] = false;
                            }
                            break;
                        }
                    } else {
                        old_solution[i] = 2;
                    }
                }
            }
            return true;
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }
    } else if (left_positive && !right_positive) {
        if (right->supports_todnf()) {
            return right->check_statement_b4(this, true, false);
        } else if (right->is_nonsuccint()) {
            return right->check_statement_b4(this, true, false);
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }
    } else if (!left_positive && right_positive) {
        if (right->supports_im()) {
            std::vector<int> vars(1,-1);
            std::vector<bool> implicant;
            implicant.push_back(true);
            for (int var : forced_false) {
                vars[0] = var;
                if (!right->is_implicant(vars, implicant)) {
                    return false;
                }
            }
            implicant[0] = false;
            for (int var : forced_true) {
                vars[0] = var;
                if (!right->is_implicant(vars, implicant)) {
                    return false;
                }
            }
            for (size_t i = 0; i < left_vars.size(); ++i) {
                vars.clear();
                implicant.clear();
                for (int var : left_vars[i]) {
                    vars.push_back(var);
                    implicant.push_back(true);
                }
                if (right_side[i] != -1) {
                    vars.push_back(right_side[i]);
                    implicant.push_back(false);
                }
                if (!right->is_implicant(vars, implicant)) {
                    return false;
                }
            }
            return true;
        } else if (right->supports_tocnf()) {
            return right->check_statement_b4(this, false, true);
        } else {
            std::cerr << "mixed representation subset check not possible" << std::endl;
            return false;
        }
    } else { // both negative
        return right->check_statement_b4(this, true, true);
    }
}

const SSFHorn *SSFHorn::get_compatible(const StateSetVariable *stateset) const {
    const SSFHorn *ret = dynamic_cast<const SSFHorn *>(stateset);
    if (ret) {
        return ret;
    }
    const SSVConstant *cformula = dynamic_cast<const SSVConstant *>(stateset);
    if (cformula) {
        return get_constant(cformula->get_constant_type());
    }
    return nullptr;
}

const SSFHorn *SSFHorn::get_constant(ConstantType ctype) const {
    switch (ctype) {
    case ConstantType::EMPTY:
        return util->emptyformula;
        break;
    case ConstantType::INIT:
        return util->initformula;
        break;
    case ConstantType::GOAL:
        return util->goalformula;
        break;
    default:
        std::cerr << "Unknown Constant type: " << std::endl;
        return nullptr;
        break;
    }
}


const std::vector<int> &SSFHorn::get_varorder() const {
    return varorder;
}

bool SSFHorn::is_contained(const std::vector<bool> &model) const {
    Cube cube(varamount, 2);
    for (size_t i = 0; i < model.size(); ++i) {
        if(model[i]) {
            cube[i] = 1;
        } else {
            cube[i] = 0;
        }
    }
    return util->is_restricted_satisfiable(this, cube);
}

bool SSFHorn::is_implicant(const std::vector<int> &varorder, const std::vector<bool> &implicant) const {
    std::vector<std::pair<std::vector<int>,int>> clauses(varorder.size());
    for (size_t i = 0; i < varorder.size(); ++i) {
        if(implicant[i]) {
            clauses.push_back(std::make_pair(std::vector<int>(),varorder[i]));
        } else {
            clauses.push_back(std::make_pair(std::vector<int>(1,varorder[i]),-1));
        }
    }
    SSFHorn implicant_horn(clauses, util->task.get_number_of_facts());
    std::vector<SSFHorn *>left,right;
    left.push_back(&implicant_horn);
    // TODO: HACK - remove!
    right.push_back(const_cast<SSFHorn *>(this));
    return util->conjunction_implies_disjunction(left, right);
}

bool SSFHorn::is_entailed(const std::vector<int> &varorder, const std::vector<bool> &clause) const {
    Cube cube(varamount, 2);
    for (size_t i = 0; i < clause.size(); ++i) {
        if(clause[i]) {
            cube[varorder[i]] = 0;
        } else {
            cube[varorder[i]] = 1;
        }
    }
    return util->is_restricted_satisfiable(this, cube);
}

bool SSFHorn::get_clause(int i, std::vector<int> &vars, std::vector<bool> &clause) const {
    if(i < forced_true.size()) {
        vars.clear();
        clause.clear();
        vars.push_back(forced_true[i]);
        clause.push_back(true);
        return true;
    }

    i -= forced_true.size();
    if (i < forced_false.size()) {
        vars.clear();
        clause.clear();
        vars.push_back(forced_false[i]);
        clause.push_back(false);
        return true;
    }

    i -= forced_false.size();
    if (i < left_vars.size()) {
        vars.clear();
        clause.clear();
        for (int var : left_vars[i]) {
            vars.push_back(var);
            clause.push_back(false);
        }
        if (right_side[i] != -1) {
            vars.push_back(right_side[i]);
            clause.push_back(true);
        }
        return true;

    }
    return false;
}

int SSFHorn::get_model_count() const {
    std::cerr << "Horn Formula does not support model count";
    exit_with(ExitCode::CRITICAL_ERROR);
    return -1;
}

StateSetBuilder<SSFHorn> horn_builder("h");
