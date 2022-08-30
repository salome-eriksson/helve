#ifndef PROOFCHECKER_H
#define PROOFCHECKER_H

#include "actionset.h"
#include "knowledge.h"
#include "stateset.h"
#include "task.h"

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ProofChecker
{
private:
    using ActionID = size_t;
    using SetID = size_t;
    using KnowledgeID = size_t;
    using DeadKnowledgeFunction =
        std::function<void(KnowledgeID, SetID, std::vector<KnowledgeID> &)>;
    using SubsetKnowledgeFunction =
        std::function<void(KnowledgeID, SetID, SetID, std::vector<KnowledgeID> &)>;
    Task task;

    std::deque<std::unique_ptr<StateSet>> statesets;
    std::deque<std::unique_ptr<ActionSet>> actionsets;
    std::deque<std::unique_ptr<Knowledge>> knowledgebase;
    bool unsolvability_proven;

    std::unordered_map<std::string, DeadKnowledgeFunction> check_dead_knowlege;
    std::unordered_map<std::string, SubsetKnowledgeFunction> check_subset_knowledge;

    template<class T>
    const T *get_set_expression(SetID set_id) const;
    void add_knowledge(std::unique_ptr<Knowledge> entry, KnowledgeID id);

    // rules for checking if state sets are dead
    void check_rule_ed(KnowledgeID conclusion_id, SetID stateset_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_ud(KnowledgeID conclusion_id, SetID stateset_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_sd(KnowledgeID conclusion_id, SetID stateset_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_pg(KnowledgeID conclusion_id, SetID stateset_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_pi(KnowledgeID conclusion_id, SetID stateset_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_rg(KnowledgeID conclusion_id, SetID stateset_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_ri(KnowledgeID conclusion_id, SetID stateset_id,
                       std::vector<KnowledgeID> &premise_ids);

    // rules for ending the proof
    void check_rule_ci(KnowledgeID conclusion_id, KnowledgeID premise_id);
    void check_rule_cg(KnowledgeID conclusion_id, KnowledgeID premise_id);

    // rules from basic set theory
    template<class T>
    void check_rule_ur(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    template<class T>
    void check_rule_ul(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    template<class T>
    void check_rule_ir(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    template<class T>
    void check_rule_il(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    template<class T>
    void check_rule_di(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    template<class T>
    void check_rule_su(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    template<class T>
    void check_rule_si(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    template<class T>
    void check_rule_st(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);

    // rules for progression and its relation to regression
    void check_rule_at(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_au(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_pt(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_pu(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_rp(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);
    void check_rule_pr(KnowledgeID conclusion_id, SetID left_id, SetID right_id,
                       std::vector<KnowledgeID> &premise_ids);

    // basic statements
    void check_statement_b1(KnowledgeID conclusion_id,
                            SetID left_id, SetID right_id,
                            std::vector<KnowledgeID> &premise_ids);
    void check_statement_b2(KnowledgeID conclusion_id,
                            SetID left_id, SetID right_id,
                            std::vector<KnowledgeID> &premise_ids);
    void check_statement_b3(KnowledgeID conclusion_id,
                            SetID left_id, SetID right_id,
                            std::vector<KnowledgeID> &premise_ids);
    void check_statement_b4(KnowledgeID conclusion_id,
                            SetID left_id, SetID right_id,
                            std::vector<KnowledgeID> &premise_ids);
    void check_statement_b5(KnowledgeID conclusion_id,
                            SetID left_id, SetID right_id,
                            std::vector<KnowledgeID> &premise_ids);
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
