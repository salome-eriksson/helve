#include "hornutil.h"

#include "ssfhorn.h"

#include "../global_funcs.h"

#include <cassert>
#include <deque>

HornConjunctionElement::HornConjunctionElement (const SSFHorn *formula)
    : formula(formula), removed_implications(formula->get_size(), false) {
}

HornUtil::HornUtil(Task &task)
    : task(task) {
    int varamount = task.get_number_of_facts();
    std::vector<std::pair<std::vector<int>,int>> clauses;
    // this is the maximum amount of clauses the formulas need
    clauses.reserve(varamount*2);

    trueformula = new SSFHorn(clauses, varamount);
    clauses.push_back(std::make_pair(std::vector<int>(1,0),-1));
    clauses.push_back(std::make_pair(std::vector<int>(),0));
    emptyformula = new SSFHorn(clauses, varamount);
    clauses.clear();

    // insert goal
    const Cube &goal = task.get_goal();
    for(int i = 0; i < goal.size(); ++i) {
        if(goal.at(i) == 1) {
            clauses.push_back(std::make_pair(std::vector<int>(),i));
        }
    }
    goalformula = new SSFHorn(clauses, varamount);
    clauses.clear();

    // insert initial state
    clauses.clear();
    const Cube &init = task.get_initial_state();
    for(int i = 0; i < init.size(); ++i) {
        if(init.at(i) == 1) {
            clauses.push_back(std::make_pair(std::vector<int>(),i));
        } else {
            clauses.push_back(std::make_pair(std::vector<int>(1,i),-1));
        }
    }
    initformula = new SSFHorn(clauses, varamount);
}

bool HornUtil::simplify_conjunction(std::vector<HornConjunctionElement> &conjuncts, Cube &partial_assignment) {
    int total_varamount = partial_assignment.size();
    int amount_implications = 0;
    std::vector<int> implstart;

    std::deque<std::pair<int,bool>> unit_clauses;
    std::vector<int> leftcount;
    std::vector<int> rightvars;

    for(size_t var = 0; var < partial_assignment.size(); ++var) {
        int assignment = partial_assignment[var];
        if (assignment == 0) {
            unit_clauses.push_back(std::make_pair(var,false));
        } else if (assignment == 1) {
            unit_clauses.push_back(std::make_pair(var,true));
        }
    }

    for(const HornConjunctionElement &conjunct : conjuncts) {
        int local_varamount = conjunct.formula->get_varamount();
        total_varamount = std::max(total_varamount, local_varamount);
        implstart.push_back(amount_implications);
        amount_implications += conjunct.formula->get_size();
        partial_assignment.resize(total_varamount,2);
        leftcount.insert(leftcount.end(), conjunct.formula->get_left_sizes().begin(),
                         conjunct.formula->get_left_sizes().end());
        rightvars.insert(rightvars.end(), conjunct.formula->get_right_sides().begin(),
                        conjunct.formula->get_right_sides().end());

        for(int var : conjunct.formula->get_forced_false()) {
            if (partial_assignment[var] == 1) {
                return false;
            }
            unit_clauses.push_back(std::make_pair(var, false));
            partial_assignment[var] = 0;
        }
        for(int var : conjunct.formula->get_forced_true()) {
            if (partial_assignment[var] == 0) {
                return false;
            }
            unit_clauses.push_back(std::make_pair(var, true));
            partial_assignment[var] = 1;
        }
    }
    std::vector<bool> already_seen(total_varamount, false);

    // unit propagation
    while(!unit_clauses.empty()) {
        std::pair<int,bool> unit_clause = unit_clauses.front();
        unit_clauses.pop_front();

        int var = unit_clause.first;
        if (already_seen[var]) {
            continue;
        }
        int positive = unit_clause.second;
        already_seen[var] = true;
        // positive literal
        if(positive) {
            for (size_t i = 0; i < conjuncts.size(); ++i) {
                HornConjunctionElement &conjunct = conjuncts[i];
                if (var >= conjunct.formula->get_varamount()) {
                    continue;
                }
                const std::forward_list<int> &left_occurences = conjunct.formula->get_variable_occurence_left(var);
                const std::forward_list<int> &right_occurences = conjunct.formula->get_variable_occurence_right(var);
                for (int left_occ : left_occurences) {
                    int global_impl = implstart[i]+left_occ;
                    leftcount[global_impl]--;
                    // deleted last negative literal --> must have positive literal
                    if (leftcount[global_impl] == 0) {
                        int newvar = conjunct.formula->get_right(left_occ);
                        assert(newvar != -1);
                        if (partial_assignment[newvar] == 0) {
                            return false;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, true));
                        partial_assignment[newvar] = 1;
                        conjunct.removed_implications[left_occ] = true;
                    // deleted second last negative literal and no positive literal
                    } else if (leftcount[global_impl] == 1
                               && rightvars[global_impl] == -1) {
                        // find corresponding variable in left
                        int newvar = -1;
                        for (int leftvar : conjunct.formula->get_left_vars(left_occ)) {
                            if (partial_assignment[leftvar] != 1) {
                                newvar = leftvar;
                                break;
                            }
                        }
                        if (newvar == -1) {
                            return false;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                        partial_assignment[newvar] = 0;
                        conjunct.removed_implications[left_occ] = true;
                    }
                }
                for(int right_occ: right_occurences) {
                    conjunct.removed_implications[right_occ] = true;
                }
            } // end iterate over formulas
        // negative literal
        } else {
            for (size_t i = 0; i < conjuncts.size(); ++i) {
                HornConjunctionElement &conjunct = conjuncts[i];
                if (var >= conjunct.formula->get_varamount()) {
                    continue;
                }
                const std::forward_list<int> &left_occurences = conjunct.formula->get_variable_occurence_left(var);
                const std::forward_list<int> &right_occurences = conjunct.formula->get_variable_occurence_right(var);
                for (int left_occ : left_occurences) {
                    conjunct.removed_implications[left_occ] = true;
                }
                for(int right_occ: right_occurences) {
                    rightvars[implstart[i]+right_occ] = -1;
                    if(leftcount[implstart[i]+right_occ] == 1) {
                        // find corresponding variable in left
                        int newvar = -1;
                        for (int leftvar : conjunct.formula->get_left_vars(right_occ)) {
                            if (partial_assignment[leftvar] != 1) {
                                newvar = leftvar;
                                break;
                            }
                        }
                        if (newvar == -1) {
                            return false;
                        }
                        unit_clauses.push_back(std::make_pair(newvar, false));
                        partial_assignment[newvar] = 0;
                        conjunct.removed_implications[right_occ] = true;
                    }
                }
            } // end iterate over formulas
        } // end if else (positive or negative unit clause)
    } // end while unit clauses not empty
    return true;
}

bool HornUtil::is_restricted_satisfiable(const SSFHorn *formula, Cube &restriction) {
    std::vector<HornConjunctionElement> vec(1,HornConjunctionElement(formula));
    return simplify_conjunction(vec, restriction);
}

inline bool update_current_clauses(std::vector<int> &current_clauses, std::vector<int> &clause_amount) {
    int pos_to_change = 0;
    while (current_clauses[pos_to_change] == clause_amount[pos_to_change]-1) {
        current_clauses[pos_to_change] = 0;
        pos_to_change++;
        if (pos_to_change == current_clauses.size()) {
            return false;
        }
    }
    current_clauses[pos_to_change]++;
    return true;
}


bool HornUtil::conjunction_implies_disjunction(std::vector<SSFHorn *> &conjuncts,
                                               std::vector<SSFHorn *> &disjuncts) {
    if (conjuncts.size() == 0) {
        conjuncts.push_back(trueformula);
    }
    std::vector<int> clause_amounts;
    std::vector<int> current_clauses;
    int disj_varamount = 0;
    // iterator for removing while iterating
    std::vector<SSFHorn *>::iterator disjunct_it = disjuncts.begin();
    while (disjunct_it != disjuncts.end()) {
        SSFHorn *formula = *disjunct_it;
        /*
         * An empty formula is equivalent to \top
         * -> return true since everything is a subset of a union containing \top
         */
        if (formula->get_size() + formula->get_forced_false().size()
                + formula->get_forced_true().size() == 0) {
            return true;
        }
        // formula not satisfiable -> ignore it
        Cube dummy;
        if (!is_restricted_satisfiable(formula, dummy)) {
            disjunct_it = disjuncts.erase(disjunct_it);
            continue;
        }
        clause_amounts.push_back(formula->get_forced_true().size()
                                 +formula->get_forced_false().size()
                                 +formula->get_size());
        current_clauses.push_back(0);
        int local_varamount = formula->get_varamount();
        disj_varamount = std::max(disj_varamount, local_varamount);
        disjunct_it++;
    }

    SSFHorn conjunction_dummy(task);
    const SSFHorn *conjunction = conjuncts[0];
    if(conjuncts.size() > 1) {
        conjunction_dummy = SSFHorn(conjuncts);
        conjunction = &conjunction_dummy;
    }

    Cube conjunction_pa;
    if ( !is_restricted_satisfiable(conjunction,conjunction_pa) ) {
        return true;
    }
    conjunction_pa.resize(disj_varamount, 2);

    // try for each clause combination if its negation together with conjuncts is unsat
    do {
        bool unsatisfiable = false;
        Cube partial_assignment(disj_varamount,2);
        for (size_t i = 0; i < current_clauses.size(); ++i) {
            const SSFHorn *formula = disjuncts[i];
            int clausenum = current_clauses[i];
            if (clausenum < formula->get_forced_true().size()) {
                int forced_true = formula->get_forced_true().at(clausenum);
                if (partial_assignment[forced_true] == 1
                        || conjunction_pa[forced_true] == 1) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_true] = 0;
            }

            clausenum -= formula->get_forced_true().size();
            if (clausenum >= 0 && clausenum < formula->get_forced_false().size()) {
                int forced_false = formula->get_forced_false().at(clausenum);
                if (partial_assignment[forced_false] == 0
                        || conjunction_pa[forced_false] == 0) {
                    unsatisfiable = true;
                    break;
                }
                partial_assignment[forced_false] = 1;
            }

            clausenum -= formula->get_forced_false().size();
            if (clausenum >= 0) {
                for (int left_var : formula->get_left_vars(clausenum)) {
                    if (partial_assignment[left_var] == 0
                            || conjunction_pa[left_var] == 0) {
                        unsatisfiable = true;
                        break;
                    }
                    partial_assignment[left_var] = 1;
                }
                int right_var = formula->get_right(clausenum);
                if (right_var != -1) {
                    if (partial_assignment[right_var] == 1
                            || conjunction_pa[right_var] == 1) {
                        unsatisfiable = true;
                        break;
                    }
                    partial_assignment[right_var] = 0;
                }
            }

            if (unsatisfiable) {
                break;
            }
        } // end loop over disjuncts

        if (unsatisfiable) {
            continue;
        }
        if (is_restricted_satisfiable(conjunction, partial_assignment)) {
            return false;
        }
    } while (update_current_clauses(current_clauses, clause_amounts));

    return true;
}
