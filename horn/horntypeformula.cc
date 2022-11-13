#include "horntypeformula.h"

#include <cassert>

#include <iostream>

namespace horn {
void HornTypeFormula::verify_horn_type() {
    for (const Clause &clause : clauses) {
        int negative_literals = 0;
        int positive_literals = 0;
        for (Literal literal : clause) {
            if (literal.second) {
                positive_literals++;
            } else {
                negative_literals++;
            }
        }
        if (!dual && positive_literals > 1) {
            throw std::runtime_error(print(false) + " is not a horn formula.");
        } else if (dual && negative_literals > 1) {
            throw std::runtime_error(print(false) + " is not a dual horn formula.");
        }
    }
}

HornTypeFormula::HornTypeFormula(std::stringstream &ss, bool dual)
    : CNFFormula(ss), dual(dual) {
    verify_horn_type();
}


HornTypeFormula::HornTypeFormula(const std::vector<const HornTypeFormula *> &&conjuncts,
                                 bool dual)
    : CNFFormula(std::move(std::vector<const CNFFormula *>
                           (conjuncts.begin(), conjuncts.end()))),
      dual(dual) {
    for (const HornTypeFormula *conjunct : conjuncts) {
        // make sure that we don't mix horn with dual horn
        assert(conjunct->dual == dual);
    }
    /*
     * If all conjuncts are (dual) horn formulas, the conjunction is also
     * (dual) horn -> no need to call verify_horn_type().
     */
}

HornTypeFormula::HornTypeFormula(const PartialAssignment &partial_assignment,
                                 bool dual)
    : CNFFormula(partial_assignment), dual(dual) {
    /*
     * A conjunction of literals is always a (dual) horn formula
     * -> no need to call verify_horn_type().
     */
}

/*
 * Formula phi entails clause gamma if phi ^ ¬gamma is unsatisfiable.
 * For Horn type formulas unsatisfiability can be checked with unit propagation.
 */
bool HornTypeFormula::entails(const Clause &clause) const {
    PartialAssignment negated_clause;
    for (Literal literal : clause) {
        negated_clause.insert({literal.first, !literal.second});
    }
    return !unit_propagation({this}, negated_clause);
}

const HornTypeFormula &HornTypeFormula::get_unsatisfiable_formula(bool dual) {
    std::stringstream ss1("p cnf 1 2 1 0 -1 0 ;");
    std::stringstream ss2("p cnf 1 2 1 0 -1 0 ;");
    static HornTypeFormula horn_unsatisfiable_formula(ss1, false);
    static HornTypeFormula dual_horn_unsatisfiable_formula(ss2, true);
    if (dual) {
        return dual_horn_unsatisfiable_formula;
    } else {
        return horn_unsatisfiable_formula;
    }
}

void HornTypeFormula::dump() const {
    if (has_no_clauses()) {
        std::cout << "TRUE" << std::endl;
    } else if (has_empty_clause) {
        std::cout << "FALSE" << std::endl;
    } else {
        for (Literal lit : unit_clauses) {
            std::cout << lit.first << "->" << lit.second << std::endl;
        }
        for (const Clause &clause : clauses) {
            for (Literal lit : clause) {
                std::cout << lit.first << "->" << lit.second << " ";
            }
            std::cout << std::endl;
        }
    }
}

}
