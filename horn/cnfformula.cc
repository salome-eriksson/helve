#include "cnfformula.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <unordered_set>

namespace horn {
CNFFormula::CNFFormula(std::stringstream &input)
    : has_empty_clause(false) {
    std::string word;
    size_t clauseamount;

    // A DIMACS formula starts with "p cnf".
    word = read_word(input);
    if (word.compare("p") != 0) {
        throw std::runtime_error("Invalid DIMACS format");
    }
    word = read_word(input);
    if (word.compare("cnf") != 0) {
        throw std::runtime_error("Invalid DIMACS format");
    }
    varamount = read_uint(input);
    clauseamount = read_uint(input);
    clauses.reserve(clauseamount);
    variable_occurrence.reserve(varamount);
    // First store unit clauses in a map for detecting inconsistency and redundancy.
    PartialAssignment unit_clauses_map;

    for(size_t i = 0; i < clauseamount; ++i) {
        Clause clause;
        bool is_valid = false;
        int var = read_int(input);
        // Clauses are separated by 0.
        while (var != 0) {
            // A variable is represented by index {1..}, its negation by {-1,..}.
            bool val = true;
            if(var < 0) {
                var = -var;
                val = false;
            }
            // Shift by 1 since we start at 0 but DIMACS starts at 1.
            var -= 1;
            auto result = clause.insert({(unsigned)var,val});
            // If the negation of the literal is already in the clause, it is valid.
            if (!result.second && result.first->second != val) {
                is_valid = true;
            }
            var = read_int(input);
        }

        // Valid clauses can be ignored.
        if(is_valid) {
            continue;
            // We don't break so we can check the syntax of the rest of the formula.
        }
        if(clause.size() == 0) {
            has_empty_clause = true;
        } else if(clause.size() == 1) {
            auto result = unit_clauses_map.insert({clause.begin()->first,
                                                   clause.begin()->second});
            /*
             * If the negation of the literal is already a unit clause
             * the formula is unsatisfiable, which we express with an empty clause.
             */
            if (!result.second && result.first->second != clause.begin()->second) {
                has_empty_clause = true;
            }
        } else {
            size_t clause_index = clauses.size();
            for (const Literal &literal : clause) {
                variable_occurrence[literal.first].push_back(clause_index);
            }
            clauses.push_back(clause);
        }
    }
    word = read_word(input);
    if(word.compare(";") != 0) {
        throw std::runtime_error("Invalid HornStateSet syntax.");
    }
    // An empty clause implies unsatisfiablility -> remove all other information.
    if (has_empty_clause) {
        clauses.clear();
        variable_occurrence.clear();
    } else {
        // Move unit clauses from temporary map to vector.
        unit_clauses.reserve(unit_clauses_map.size());
        for (Literal literal : unit_clauses_map) {
            unit_clauses.push_back(literal);
        }
        simplify_with_unit_propagation();
    }
}

CNFFormula::CNFFormula(const std::vector<const CNFFormula *> &&conjuncts)
    : varamount(0) {
    size_t clauseamount = 0;
    for(const CNFFormula *conjunct : conjuncts) {
        varamount = std::max(varamount, conjunct->varamount);
        clauseamount += conjunct->clauses.size();
    }

    PartialAssignment unit_clauses_map;
    has_empty_clause = !(CNFFormula::unit_propagation(conjuncts, unit_clauses_map));
    if(has_empty_clause) {
        return;
    }

    unit_clauses.reserve(unit_clauses_map.size());
    for (Literal literal : unit_clauses_map) {
        unit_clauses.push_back(literal);
    }

    clauses.reserve(clauseamount);
    for(const CNFFormula *conjunct : conjuncts) {
        for (const Clause &clause : conjunct->clauses) {
            Clause new_clause;
            new_clause.reserve(clause.size());
            bool valid = false;
            for (Literal literal : clause) {
                Clause::iterator it = unit_clauses_map.find(literal.first);
                /*
                 * See if the variable of the literal is already used in some
                 * unit clause gamma. We distinguish three possibilities:
                 *  - literal occurs in gamma: remove clause
                 *  - negated literal occurs in gamma: remove literal from clause
                 *  - variable does not occur in any unit clause: do nothing
                 */
                if (it == unit_clauses_map.end()) {
                    new_clause.insert(literal);
                } else if (it->second == literal.second) {
                    valid = true;
                    break;
                }
            }
            if (!valid) {
                for (Literal literal : new_clause) {
                    variable_occurrence[literal.first].push_back(clauses.size());
                }
                clauses.push_back(new_clause);
            }
        }
    }
    clauses.shrink_to_fit();
}

CNFFormula::CNFFormula(const PartialAssignment &partial_assignment)
    : has_empty_clause(false), varamount(0) {
    for (Literal literal : partial_assignment) {
        varamount = std::max(varamount, (literal.first+1));
        unit_clauses.push_back(literal);
    }
}

/*
 * Helper function for simplify_with_unit_propatation().
 * Assumes that indices is sorted.
 */
inline void removeClausesByIndex(std::vector<size_t>& indices,
                          std::vector<Clause>& clauses) {
  size_t indices_index = 0;
  clauses.erase(
    std::remove_if(std::begin(clauses), std::end(clauses), [&](Clause& elem) {
        if ( (indices_index != indices.size())
                  && (&elem - &clauses[0] == indices[indices_index]) )
        {
           indices_index++;
           return true;
        }
        return false;
    }),
    std::end(clauses)
  );
}

void CNFFormula::simplify_with_unit_propagation() {
    PartialAssignment new_unit_clauses;
    bool unsatisfiability_shown = !unit_propagation({this}, new_unit_clauses);

    // If the formula is unsatisfiable, we can remove all other information.
    if (unsatisfiability_shown) {
        has_empty_clause = true;
        unit_clauses.clear();
        clauses.clear();
        variable_occurrence.clear();
        return;
    }

    // Remove all unit clauses (old ones are contained in new_unit_clauses).
    unit_clauses.clear();
    unit_clauses.reserve(new_unit_clauses.size());

    // This needs to be a set, we might insert clause inidices multiple times.
    std::unordered_set<size_t> clauses_to_remove;
    // Add unit clauses and clean up existing clauses.
    for (const Literal &literal : new_unit_clauses) {
        unsigned var = literal.first;
        bool val = literal.second;

        unit_clauses.push_back(literal);

        // Remove references  of the unit clause variable in clauses.
        if(variable_occurrence.count(var) > 0) {
            for (size_t clause_index : variable_occurrence[var]) {
                Clause &clause = clauses[clause_index];
                assert(clause.count(var) > 0);

                // unit clause satisfies clause: mark clause for deletion
                if(clause.at(var) == val) {
                    clauses_to_remove.insert(clause_index);
                // unit clause does not satisfy clause: remove var from clause
                } else {
                    clause.erase(var);
                }
            }
        }
    }

    // Remove clauses that are satisfied through new unit clauses.
    std::vector<size_t> vec(clauses_to_remove.begin(), clauses_to_remove.end());
    std::sort(vec.begin(), vec.end());
    removeClausesByIndex(vec, clauses);
    clauses.shrink_to_fit();

    /*
     * We need to rebuild variable occurrences, since variable occurences
     * within clauses have changed, as well as indicies of clauses
     * (due to erasing some clauses in the clause vector).
     */
    variable_occurrence.clear();
    for(size_t index = 0; index < clauses.size(); ++index) {
        const Clause &clause = clauses[index];
        for (Literal literal: clause) {
            variable_occurrence[literal.first].push_back(index);
        }
        //Check that all remaining clauses have at least size 2.
        assert(clause.size() >= 2);
    }
}

// Helper function for unit_propagation().
inline bool insert_to_queue(Literal lit, std::deque<Literal> &queue,
                            PartialAssignment &assignment) {
    if (assignment.count(lit.first) > 0) {
        if(assignment[lit.first] != lit.second) {
            return false;
        } else {
            return true;
        }
    }
    queue.push_back(lit);
    assignment[lit.first] = lit.second;
    return true;
}

/*
 * Unit propagation removes all occurrences of unit clause variables from other
 * clauses. Given unit clause "x/¬x", we know that x must be assigned true/false.
 * We can thus remove ¬x/x from all clauses and ignore all clauses containing x/¬x.
 * Since clauses can shrink during unit proagation, new unit clauses can be found.
 */
bool CNFFormula::unit_propagation(const std::vector<const CNFFormula *> &formulas,
                                  PartialAssignment &partial_assignment) {

    // If any of the formulas has an empty clause, the conjunction is unsatisfiable.
    for (const CNFFormula *formula : formulas) {
        if (formula->has_empty_clause) {
            return false;
        }
    }

    /*
     * We only store the lenght of each clause, since variable_occurence can
     * tell us where each variable occurs.
     */
    std::vector<std::vector<int>> clauses_sizes;
    clauses_sizes.resize(formulas.size(), std::vector<int>());
    for(size_t formula_index = 0; formula_index < formulas.size(); ++formula_index) {
        const std::vector<Clause> &formula_clauses = formulas[formula_index]->clauses;
        clauses_sizes[formula_index].reserve(formula_clauses.size());
        for(const Clause &clause : formula_clauses) {
            clauses_sizes[formula_index].push_back(clause.size());
        }
    }

    std::deque<Literal> queue;
    // Used for finding out which var in a clause with size 1 has not been seen.
    std::unordered_set<unsigned> processed_variables;

    /*
     * Initialize queue with all unit clauses from the formulas and the given
     * partial assignment, and extend the partial assignment with all unit clauses
     * Note: In general insert_to_queue() should be used since this also inserts
     * the unit clause into partial_assignment. For unit clauses originating
     * from partial_assignment we don't need to do this and thus directly
     * push them into the queue.
     */
    for (Literal literal : partial_assignment) {
        queue.push_back(literal);
    }
    for (const CNFFormula *formula : formulas) {
        for (Literal literal : formula->unit_clauses) {
            if(!insert_to_queue(literal, queue, partial_assignment)) {
                return false;
            }
        }
    }

    while(!queue.empty()) {
        Literal assigned_literal = queue.front();
        queue.pop_front();
        unsigned var = assigned_literal.first;
        bool val = assigned_literal.second;
        processed_variables.insert(var);

        // (Implicitly) simplify clauses, potentially finding new unit clauses.
        for (size_t f_index = 0; f_index < formulas.size(); ++f_index) {
            const CNFFormula *formula = formulas[f_index];
            if(formula->variable_occurrence.count(var) == 0) {
                continue;
            }
            for(size_t clause_index : formula->variable_occurrence.at(var)) {
                const Clause &clause = formula->clauses[clause_index];
                int &clause_size = clauses_sizes[f_index][clause_index];
                assert(clause.count(var) > 0);

                if(clause.at(var) == val) { // Assigned literal satisfies the clause.
                    // We represent a satisfied clause with negative size.
                    clause_size = -1;
                } else { // Assigned literal does not satisfy the clause.
                    clause_size--;
                    // If only one literal is left, we found a new unit clause.
                    if (clause_size == 1) {
                        bool found = false;
                        for (Literal literal : clause) {
                            if (processed_variables.count(literal.first) == 0) {
                                if (!insert_to_queue(literal, queue,
                                                     partial_assignment)) {
                                    return false;
                                }
                                found = true;
                                break;
                            }
                       }
                        assert(found);
                    }
                }
            } // end variable occurences loop
        } // end formulas loop
    } // end while queue is not empty
    return true;
}

inline void print_literal(std::stringstream &ss, const Literal &literal) {
    if(!literal.second) {
        ss << "¬";
    }
    ss << literal.first;
}

std::string CNFFormula::print(bool long_version) const {
    if (has_empty_clause) {
        return "false\n";
    } else if (unit_clauses.empty() && clauses.empty()) {
        return "true\n";
    }
    bool first_clause = true;
    std::stringstream ret;
    for (const Literal &literal : unit_clauses) {
        if(!first_clause) {
            ret << " ^ ";
        }
        first_clause = false;
        ret << "(";
        print_literal(ret, literal);
        ret << ")";
    }
    for (const Clause &clause : clauses) {
        if(!first_clause) {
            ret << " ^ ";
        }
        first_clause = false;
        ret << "(";
        bool first_literal = true;
        for (const Literal &literal : clause) {
            if(!first_literal) {
                ret << " v ";
            }
            first_literal = false;
            print_literal(ret, literal);
        }
        ret << ")";
    }

    ret << std::endl;
    if (long_version) {
        ret << "Variable occurences:" << std::endl;
        for (auto &entry : variable_occurrence) {
            ret << entry.first << ": ";
            for (size_t occ : entry.second) {
                ret << occ << " ";
            }
            ret << std::endl;
        }
        }
    return ret.str();
}

unsigned CNFFormula::get_varamount() const {
    return varamount;
}

size_t CNFFormula::get_number_of_clauses() const {
    return (has_empty_clause + unit_clauses.size() + clauses.size());
}

bool CNFFormula::contains_empty_clause() const {
    return has_empty_clause;
}

bool CNFFormula::has_no_clauses() const {
    return (get_number_of_clauses() == 0);
}


void CNFFormula::rename(const std::vector<std::pair<unsigned,unsigned>> &renames) {
    for (std::pair<unsigned, unsigned> rename : renames) {
        unsigned old_var = rename.first;
        unsigned new_var = rename.second;

        for (size_t i = 0; i < unit_clauses.size(); ++i) {
            if (unit_clauses[i].first == old_var) {
                unit_clauses[i].first = new_var;
            }
        }

        auto it = variable_occurrence.find(old_var);
        if (it != variable_occurrence.end()) {
            // First, change the variable in all clauses.
            for (size_t clause_index : it->second) {
                Clause &clause = clauses[clause_index];
                assert(clause.find(old_var) != clause.end());
                clause[new_var] = clause[old_var];
                clause.erase(old_var);
            }
            // Second, change the key in variable_occurrence.
            std::swap(variable_occurrence[new_var], it->second);
            variable_occurrence.erase(old_var);
        }
    }
}


CNFFormula::ClauseIterator::ClauseIterator(const CNFFormula &formula, size_t pos)
    : pos(pos), formula(formula) {
    set_clause();
}

void CNFFormula::ClauseIterator::set_clause() {
    if (pos < formula.unit_clauses.size() + formula.has_empty_clause) {
        clause.clear();
        if (!formula.has_empty_clause) {
            Literal literal = formula.unit_clauses[pos];
            clause[literal.first] = literal.second;
        }
    }
}

const Clause &CNFFormula::ClauseIterator::operator *() const {
    assert(pos <= formula.get_number_of_clauses());
    if (pos < formula.unit_clauses.size()+ formula.has_empty_clause) {
        return clause;
    } else {
        return formula.clauses[pos-formula.unit_clauses.size()];
    }
}

CNFFormula::ClauseIterator &CNFFormula::ClauseIterator::operator ++() {
    ++pos;
    set_clause();
    return *this;
}

bool CNFFormula::ClauseIterator::operator !=(const ClauseIterator &other) const {
    return (pos != other.pos || &formula != &other.formula);
}

bool CNFFormula::ClauseIterator::operator ==(const ClauseIterator &other) const {
    return (pos == other.pos && &formula == &other.formula);
}

void CNFFormula::ClauseIterator::reset() {
    pos = 0;
    set_clause();
}

CNFFormula::ClauseIterator CNFFormula::begin() const {
    return ClauseIterator(*this, 0);
}

CNFFormula::ClauseIterator CNFFormula::end() const {
    return ClauseIterator(*this, get_number_of_clauses());
}
}
