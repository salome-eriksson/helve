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

using SetID = size_t;
using KnowledgeID = size_t;

class ProofChecker
{
private:
    using DeadKnowledgeFunction =
        std::function<std::unique_ptr<Knowledge>(SetID, std::vector<KnowledgeID> &)>;
    using SubsetKnowledgeFunction =
        std::function<std::unique_ptr<Knowledge>(SetID, SetID, std::vector<KnowledgeID> &)>;
    Task task;

    std::deque<std::unique_ptr<StateSet>> statesets;
    std::deque<std::unique_ptr<ActionSet>> actionsets;
    std::deque<std::unique_ptr<Knowledge>> knowledgebase;
    bool unsolvability_proven;

    std::unordered_map<std::string, DeadKnowledgeFunction> check_dead_knowlege;
    std::unordered_map<std::string, SubsetKnowledgeFunction> check_subset_knowledge;

    void add_knowledge(std::unique_ptr<Knowledge> entry, KnowledgeID id);

    // rules for checking if state sets are dead
    std::unique_ptr<Knowledge> check_rule_ed(
            SetID stateset_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_ud(
            SetID stateset_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_sd(
            SetID stateset_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_pg(
            SetID stateset_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_pi(
            SetID stateset_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_rg(
            SetID stateset_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_ri(
            SetID stateset_id, std::vector<KnowledgeID> &premise_ids);

    // rules for ending the proof
    std::unique_ptr<Knowledge> check_rule_ci(KnowledgeID premise_id);
    std::unique_ptr<Knowledge> check_rule_cg(KnowledgeID premise_id);

    // rules from basic set theory
    template<class T>
    std::unique_ptr<Knowledge> check_rule_ur(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    template<class T>
    std::unique_ptr<Knowledge> check_rule_ul(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    template<class T>
    std::unique_ptr<Knowledge> check_rule_ir(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    template<class T>
    std::unique_ptr<Knowledge> check_rule_il(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    template<class T>
    std::unique_ptr<Knowledge> check_rule_di(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    template<class T>
    std::unique_ptr<Knowledge> check_rule_su(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    template<class T>
    std::unique_ptr<Knowledge> check_rule_si(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    template<class T>
    std::unique_ptr<Knowledge> check_rule_st(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);

    // rules for progression and its relation to regression
    std::unique_ptr<Knowledge> check_rule_at(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_au(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_pt(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_pu(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_rp(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_rule_pr(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);

    // basic statements
    std::unique_ptr<Knowledge> check_statement_b1(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_statement_b2(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_statement_b3(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_statement_b4(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    std::unique_ptr<Knowledge> check_statement_b5(
            SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids);
    // helper function for finding formalism for b* statements
    const StateSetFormalism *get_reference_formula(std::vector<const StateSetVariable *> &vars) const;

public:
    ProofChecker(std::string &task_file);

    template<class T, typename std::enable_if<std::is_base_of<ActionSet, T>::value>::type * = nullptr>
    const T *get_set_expression(SetID set_id) const;
    template<class T, typename std::enable_if<std::is_base_of<StateSet, T>::value>::type * = nullptr>
    const T *get_set_expression(SetID set_id) const;
    template<class T, typename std::enable_if<std::is_base_of<Knowledge, T>::value>::type * = nullptr>
    const T *get_knowledge(KnowledgeID knowledge_id) const;
    void add_state_set(std::string &line);
    void add_action_set(std::string &line);
    void verify_knowledge(std::string &line);

    bool is_unsolvability_proven();
};

#endif // PROOFCHECKER_H
