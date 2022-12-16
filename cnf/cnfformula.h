#ifndef CNFFORMULA_H
#define CNFFORMULA_H

#include "../global_funcs.h"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace cnf {
// TODO: We might want to move this class somewhere else eventually
class CNFFormula
{
protected:
  unsigned varamount;
  /*
   * Clauses are split up according to length:
   * - If the formula has an empty clause (unsatisfiable), we set has_empty_clause.
   * - All clauses of length 1 are stored as a vector of Literals in unit clauses.
   * - The rest (clauses of length >= 2) are stored in clauses.
   * If has_empty_clause is set to true, unit_clauses and clauses must be empty.
   */
  bool has_empty_clause;
  std::vector<Literal> unit_clauses;
  std::vector<Clause> clauses;
  // for each variable "key", list the index of all clauses where "key" occurs
  std::unordered_map<unsigned, std::vector<size_t>> variable_occurrence;

  void simplify_with_unit_propagation();
public:
  CNFFormula() = delete;
  CNFFormula(std::stringstream &ss);
  CNFFormula(const std::vector<const CNFFormula *> &&conjuncts);
  CNFFormula(const PartialAssignment &partial_assignment);
  CNFFormula(std::vector<Clause > &&clauses);

  /*
   * Perform unit propagation given the partial assignment (without
   * simplifying the formulas). All existing and newly inferred unit clauses are
   * added to the given partial assignment. If unit propagation detects that
   * the formula is unsatisfiable it returns false.
   */
  static bool unit_propagation(const std::vector<const CNFFormula *> &formulas,
                               PartialAssignment &partial_assignment);

  static const CNFFormula &get_unsatisfiable_formula();

  unsigned get_varamount() const;
  size_t get_number_of_clauses() const;
  bool contains_empty_clause() const;
  bool has_no_clauses() const;

  void rename(const std::vector<std::pair<unsigned,unsigned>> &renames);
  std::string print(bool long_version = true) const;

  class ClauseIterator {
  private:
      size_t pos;
      const CNFFormula &formula;
      Clause clause; //for temporarily storing unit clauses and empty clause

      void set_clause();
  public:
      ClauseIterator(const CNFFormula &formula, size_t pos);
      const Clause &operator *() const;
      ClauseIterator &operator++();
      bool operator!=(const ClauseIterator &other) const;
      bool operator==(const ClauseIterator &other) const;
      void reset();
  };
  ClauseIterator begin() const;
  ClauseIterator end() const;

};
}

#endif // CNFFORMULA_H
