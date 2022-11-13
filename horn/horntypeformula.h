#ifndef HORNTYPEFORMULA_H
#define HORNTYPEFORMULA_H

#include "cnfformula.h"

namespace horn {
/*
 * Horn-type formulas are CNF formulas with the restriction that each clause
 * can contain at most one negative (horn) / positive (dual-horn) literal.
 */
class HornTypeFormula : public CNFFormula
{
private:
    bool dual; // Whether the formula is horn or dual horn
    void verify_horn_type();
public:
    HornTypeFormula() = delete;
    HornTypeFormula(std::stringstream &ss, bool dual);
    HornTypeFormula(const std::vector<const HornTypeFormula *> &&conjuncts, bool dual);
    HornTypeFormula(const PartialAssignment &partial_assignment, bool dual);

    bool entails(const Clause &clause) const;

    static const HornTypeFormula &get_unsatisfiable_formula(bool dual);

    void dump() const;
};
}

#endif // HORNTYPEFORMULA_H
