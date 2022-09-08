#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>

/* A Cube represents a state and has 3 valid values:
 *  - 0: false
 *  - 1: true
 *  - 2: don't care
 * When Cubes are passed as parameters, they always follow the default variable order
 */
typedef std::vector<int> Cube;

struct Action {
    std::string name;
    std::vector<int> pre;
    //change shows for each variable if it gets added (+1), deleted (-1), or does not change (0)
    std::vector<int> change;
    int cost;
};

class Task {
private:
  std::vector<std::string> fact_names;
  std::vector<Action> actions;
  Cube initial_state;
  Cube goal;

  // parsing utilities
  void split(const std::string &s, std::vector<std::string> &vec, char delim);
  void print_parsing_error_and_exit(std::string &line, std::string expected);
public:
  Task(std::string file);
  const std::vector<std::string>& get_fact_names() const;
  const std::string& get_fact(int n) const;
  const Action& get_action(size_t n) const;
  int get_number_of_actions() const;
  int get_number_of_facts() const;
  const Cube& get_initial_state() const;
  const Cube& get_goal() const;
  void dump_action(int n) const;
};

#endif /* TASK_H */
