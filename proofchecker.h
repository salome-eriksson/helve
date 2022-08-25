#ifndef PROOFCHECKER_H
#define PROOFCHECKER_H

#include "actionset.h"
#include "knowledge.h"
#include "stateset.h"
#include "task.h"

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class ProofChecker
{
private:
    Task task;

    std::deque<std::unique_ptr<StateSet>> formulas;
    std::deque<std::unique_ptr<ActionSet>> actionsets;
    std::deque<std::unique_ptr<Knowledge>> kbentries;
    bool unsolvability_proven;

    // TODO: unordered_map
    // using for dead_knowledge/subset knowledge types
    std::map<std::string, std::function<bool(int, int, std::vector<int> &)>> dead_knowledge_functions;
    std::map<std::string, std::function<bool(int, int, int, std::vector<int> &)>> subset_knowledge_functions;

    template<class T>
    const T *get_set_expression(int set_id) const;
    void add_knowledge(std::unique_ptr<Knowledge> entry, int id);

    // rules for checking if state sets are dead
    bool check_rule_ed(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_ud(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_sd(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_pg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_pi(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_rg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);
    bool check_rule_ri(int conclusion_id, int stateset_id, std::vector<int> &premise_ids);

    // rules for ending the proof
    bool check_rule_ci(int conclusion_id, int premise_id);
    bool check_rule_cg(int conclusion_id, int premise_id);


    // rules from basic set theory
    template<class T>
    bool check_rule_ur(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template<class T>
    bool check_rule_ul(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template<class T>
    bool check_rule_ir(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template<class T>
    bool check_rule_il(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template<class T>
    bool check_rule_di(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template<class T>
    bool check_rule_su(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template<class T>
    bool check_rule_si(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    template<class T>
    bool check_rule_st(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);

    // rules for progression and its relation to regression
    bool check_rule_at(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_au(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_pt(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_pu(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_rp(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_rule_pr(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);

    // basic statements
    bool check_statement_B1(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_statement_B2(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_statement_B3(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_statement_B4(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);
    bool check_statement_B5(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids);

    // helper function for finding formalism for b* statements
    const StateSetFormalism *get_reference_formula(std::vector<const StateSetVariable *> &vars) const;

public:
    ProofChecker(std::string &task_file);

    void add_state_set(std::string &line);
    void add_action_set(std::string &line);

    void verify_knowledge(std::string &line);

    bool is_unsolvability_proven();
};

#endif // PROOFCHECKER_H
