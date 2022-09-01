#include "proofchecker.h"

#include "global_funcs.h"
#include "statesetcompositions.h"
#include "ssvconstant.h"

#include "rules/rules.h"

#include <cassert>
#include <cmath>
#include <iostream>

inline size_t get_id_from_string(std::string input) {
    long tmp  = std::stol(input);
    if (tmp < 0) {
       throw std::invalid_argument("ID " + input + " is negative.");
    }
    return (size_t) tmp;
}

ProofChecker::ProofChecker(std::string &task_file)
    : task(task_file), unsolvability_proven(false) {

    check_subset_knowledge = {
        { "ura", [&](auto lid, auto rid, auto pids)
          {return check_rule_ur<ActionSet>(lid, rid, pids);} },
        { "urs", [&](auto lid, auto rid, auto pids)
          {return check_rule_ur<StateSet>(lid, rid, pids);} },
        { "ula", [&](auto lid, auto rid, auto pids)
          {return check_rule_ul<ActionSet>(lid, rid, pids);} },
        { "uls", [&](auto lid, auto rid, auto pids)
          {return check_rule_ul<StateSet>(lid, rid, pids);} },
        { "ira", [&](auto lid, auto rid, auto pids)
          {return check_rule_ir<ActionSet>(lid, rid, pids);} },
        { "irs", [&](auto lid, auto rid, auto pids)
          {return check_rule_ir<StateSet>(lid, rid, pids);} },
        { "ila", [&](auto lid, auto rid, auto pids)
          {return check_rule_il<ActionSet>(lid, rid, pids);} },
        { "ils", [&](auto lid, auto rid, auto pids)
          {return check_rule_il<StateSet>(lid, rid, pids);} },
        { "dia", [&](auto lid, auto rid, auto pids)
          {return check_rule_di<ActionSet>(lid, rid, pids);} },
        { "dis", [&](auto lid, auto rid, auto pids)
          {return check_rule_di<StateSet>(lid, rid, pids);} },
        { "sua", [&](auto lid, auto rid, auto pids)
          {return check_rule_su<ActionSet>(lid, rid, pids);} },
        { "sus", [&](auto lid, auto rid, auto pids)
          {return check_rule_su<StateSet>(lid, rid, pids);} },
        { "sia", [&](auto lid, auto rid, auto pids)
          {return check_rule_si<ActionSet>(lid, rid, pids);} },
        { "sis", [&](auto lid, auto rid, auto pids)
          {return check_rule_si<StateSet>(lid, rid, pids);} },
        { "sta", [&](auto lid, auto rid, auto pids)
          {return check_rule_st<ActionSet>(lid, rid, pids);} },
        { "sts", [&](auto lid, auto rid, auto pids)
          {return check_rule_st<StateSet>(lid, rid, pids);} },

        { "at", [&](auto lid, auto rid, auto pids)
          {return check_rule_at(lid, rid, pids);} },
        { "au", [&](auto lid, auto rid, auto pids)
          {return check_rule_au(lid, rid, pids);} },
        { "pt", [&](auto lid, auto rid, auto pids)
          {return check_rule_pt(lid, rid, pids);} },
        { "pu", [&](auto lid, auto rid, auto pids)
          {return check_rule_pu(lid, rid, pids);} },
        { "pr", [&](auto lid, auto rid, auto pids)
          {return check_rule_pr(lid, rid, pids);} },
        { "rp", [&](auto lid, auto rid, auto pids)
          {return check_rule_rp(lid, rid, pids);} },

        { "b1", [&](auto lid, auto rid, auto pids)
          {return check_statement_b1(lid, rid, pids);} },
        { "b2", [&](auto lid, auto rid, auto pids)
          {return check_statement_b2(lid, rid, pids);} },
        { "b3", [&](auto lid, auto rid, auto pids)
          {return check_statement_b3(lid, rid, pids);} },
        { "b4", [&](auto lid, auto rid, auto pids)
          {return check_statement_b4(lid, rid, pids);} },
        { "b5", [&](auto lid, auto rid, auto pids)
          {return check_statement_b5(lid, rid, pids);} },
    };

    manager = Cudd(task.get_number_of_facts()*2);
    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();
}

template<class T, typename std::enable_if<std::is_base_of<ActionSet, T>::value>::type * = nullptr>
const T *ProofChecker::get_set_expression(SetID set_id) const {
    if (set_id < 0 || set_id >= actionsets.size() || !actionsets[set_id]) {
        throw std::runtime_error("Action set expression #" +
                                 std::to_string(set_id) +
                                 " does not exist.");
    }
    const T* ret= dynamic_cast<const T *>(actionsets[set_id].get());
    if (!ret) {
        throw std::runtime_error("Action set expression #" +
                                 std::to_string(set_id) +
                                 " is not of type" + typeid(T).name());
    }
    return ret;
}

template<class T, typename std::enable_if<std::is_base_of<StateSet, T>::value>::type * = nullptr>
const T *ProofChecker::get_set_expression(SetID set_id) const {
    if (set_id < 0 || set_id >= statesets.size() || !statesets[set_id]) {
        throw std::runtime_error("State set expression #" +
                                 std::to_string(set_id) +
                                 " does not exist.");
    }
    const T* ret= dynamic_cast<const T *>(statesets[set_id].get());
    if (!ret) {
        throw std::runtime_error("State set expression #" +
                                 std::to_string(set_id) +
                                 " is not of type" + typeid(T).name());
    }
    return ret;
}

template<class T, typename std::enable_if<std::is_base_of<Knowledge, T>::value>::type * = nullptr>
const T *ProofChecker::get_knowledge(KnowledgeID knowledge_id) const {
    if (knowledge_id < 0 || knowledge_id >= knowledgebase.size()
            || !knowledgebase[knowledge_id]) {
        throw std::runtime_error("Knowledge #" + std::to_string(knowledge_id) +
                                 " does not exist.");
    }
    const T* ret= dynamic_cast<const T *>(knowledgebase[knowledge_id].get());
    if (!ret) {
        throw std::runtime_error("Knowledge #" + std::to_string(knowledge_id) +
                                 " is not of type" + typeid(T).name());
    }
    return ret;
}

void ProofChecker::add_knowledge(std::unique_ptr<Knowledge> entry,
                                 KnowledgeID id) {
    if(id >= knowledgebase.size()) {
        knowledgebase.resize(id+1);
    }
    assert(!knowledgebase[id]);
    knowledgebase[id] = std::move(entry);
}

// line format: <id> <type> <description>
void ProofChecker::add_state_set(std::string &line) {
    std::stringstream ssline(line);
    std::string word, state_set_type;
    ssline >> word;
    SetID set_id = get_id_from_string(word);
    ssline >> state_set_type;
    auto stateset_constructors = StateSet::get_stateset_constructors();
    if (stateset_constructors->find(state_set_type)
            == stateset_constructors->end()) {
        throw std::runtime_error("Stateset expression type " + state_set_type
                                 + " does not exist.");
    }
    std::unique_ptr<StateSet> expression =
            StateSet::get_stateset_constructors()->at(state_set_type)(ssline, task);
    if (set_id >= statesets.size()) {
        statesets.resize(set_id+1);
    }
    assert(!statesets[set_id]);
    statesets[set_id] = std::move(expression);
}

// line format: <id> <type> <description>
void ProofChecker::add_action_set(std::string &line) {
    std::stringstream ssline(line);
    std::string word, action_set_type;
    SetID set_id;
    std::unique_ptr<ActionSet> action_set;
    ssline >> word;
    set_id = get_id_from_string(word);
    ssline >> action_set_type;

    if(action_set_type.compare("b") == 0) { // basic enumeration of actions
        // the first number denotes the amount of actions being enumerated
        int amount;
        std::unordered_set<int> actions;
        ssline >> amount;
        int a;
        for(int i = 0; i < amount; ++i) {
            ssline >> a;
            actions.insert(a);
        }
        action_set = std::unique_ptr<ActionSet>(new ActionSetBasic(actions));

    } else if(action_set_type.compare("u") == 0) { // union of action sets
        SetID left_id, right_id;
        ssline >> word;
        left_id = get_id_from_string(word);
        ssline >> word;
        right_id = get_id_from_string(word);
        action_set = std::unique_ptr<ActionSet>(new ActionSetUnion(left_id, right_id));

    } else if(action_set_type.compare("a") == 0) { // constant (set of all actions)
        action_set = std::unique_ptr<ActionSet>(new ActionSetConstantAll(task));

    } else {
        throw std::runtime_error("Actionset expression type " + action_set_type
                                 + " does not exist.");
    }

    if (set_id >= actionsets.size()) {
        actionsets.resize(set_id+1);
    }
    assert(!actionsets[set_id]);
    actionsets[set_id] = std::move(action_set);
}

// line format: <id> <type> <description>
void ProofChecker::verify_knowledge(std::string &line) {
    KnowledgeID conclusion_id;
    std::unique_ptr<Knowledge> conclusion;
    std::stringstream ssline(line);
    std::string word, rule, knowledge_type;

    ssline >> word;
    conclusion_id = get_id_from_string(word);
    ssline >> knowledge_type;

    if(knowledge_type == "s") {
        // Subset knowledge is defined by "<left_id> <right_id> <rule> {premise_ids}".
        SetID left_id, right_id;
        std::vector<KnowledgeID> premises;
        // reserve max amount of premises (currently 2)
        premises.reserve(2);

        ssline >> word;
        left_id = get_id_from_string(word);
        ssline >> word;
        right_id = get_id_from_string(word);
        ssline >> rule;
        while (ssline >> word) {
            premises.push_back(get_id_from_string(word));
        }

        if (check_subset_knowledge.find(rule) == check_subset_knowledge.end()) {
            throw std::runtime_error("Rule " + rule + " is not a subset rule.");
        }
        conclusion = check_subset_knowledge[rule](left_id, right_id, premises);

    } else if(knowledge_type == "d") {
        // Dead knowledge is defined by "<dead_id> <rule> {premise_ids}".
        SetID dead_set_id;
        std::vector<KnowledgeID> premises;
        // reserve max amount of premises (currently 3)
        premises.reserve(3);

        ssline >> word;
        dead_set_id = get_id_from_string(word);
        ssline >> rule;
        while (ssline >> word) {
            premises.push_back(get_id_from_string(word));
        }

        if (rules::deadness_rules.find(rule) == rules::deadness_rules.end()) {
            throw std::runtime_error(" Rule " + rule + " is not a dead rule.");
        }
        conclusion = rules::deadness_rules[rule](dead_set_id, premises, *this);
    } else if(knowledge_type == "u") {
        // Unsolvability knowledge is defined by "<rule> <premise_id>".
        KnowledgeID premise;

        ssline >> rule;
        ssline >> word;
        premise = get_id_from_string(word);

        if (rule.compare("ci") == 0) {
            conclusion = check_rule_ci(premise);
        } else if (rule.compare("cg") == 0) {
            conclusion = check_rule_cg(premise);
        } else {
            throw std::runtime_error("Rule " + rule
                                     + " is not an unsolvability rule.");
        }
        unsolvability_proven = true;

    } else {
        throw std::runtime_error("Knowledge type " + knowledge_type
                                 + " does not exist.");
    }
    add_knowledge(std::move(conclusion), conclusion_id);
}


/*
 * RULES ABOUT DEADNESS
 */
/*
// Emptyset Dead: Without premises, \emptyset is dead.
std::unique_ptr<Knowledge> ProofChecker::check_rule_ed(
        SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    SSVConstant *f =
        dynamic_cast<SSVConstant *>(statesets[stateset_id].get());
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        throw std::runtime_error("Cannot apply rule ED: set expression #" +
                                 std::to_string(stateset_id) +
                                 " is not the constant empty set.");
    }
    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}


// Union Dead: Given (1) S is dead and (2) S' is dead, then S \cup S' is dead.
std::unique_ptr<Knowledge> ProofChecker::check_rule_ud(
        SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    // The stateset represents S \cup S'.
    StateSetUnion *f =
        dynamic_cast<StateSetUnion *>(statesets[stateset_id].get());
    if (!f) {
        throw std::runtime_error("Cannot apply rule UD: set expression #"
                                 + std::to_string(stateset_id)
                                 + " is not a union.");
    }
    int left_id = f->get_left_id();
    int right_id = f->get_right_id();

    // Check if premise_ids[0] says that S is dead.
    DeadKnowledge *dead_knowledge =
        dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[0]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != left_id)) {
        throw std::runtime_error("Cannot apply rule UD: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state that set expression #" +
                                 std::to_string(left_id) + " is dead.");
    }

    // Check if premise_ids[1] says that S' is dead.
    dead_knowledge =
        dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != right_id)) {
        throw std::runtime_error("Cannot apply rule UD: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state that set expression #" +
                                 std::to_string(right_id) + " is dead.");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}


// Subset Dead: Given (1) S \subseteq S' and (2) S' is dead, set=S is dead.
std::unique_ptr<Knowledge> ProofChecker::check_rule_sd(
        SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    // Check if premise_ids[0] says that S is a subset of some S'.
    SubsetKnowledge<StateSet> *subset_knowledge =
        dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge || (subset_knowledge->get_left_id() != stateset_id)) {
        throw std::runtime_error("Cannot apply rule SD: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state that set expression #" +
                                 std::to_string(stateset_id) +
                                 " is a subset of another set.");
    }

    // Check if premise_ids[1] says that S' is dead.
    DeadKnowledge *dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge ||
        (dead_knowledge->get_set_id() != subset_knowledge->get_right_id()) ) {
        throw std::runtime_error("Cannot apply rule SD: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state that set expression #" +
                                 std::to_string(subset_knowledge->get_right_id()) +
                                 " is dead.");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}

// Progression Goal: Given (1) S[A] \subseteq S \cup S', (2) S' is dead and
// (3) S \cap S_G^\Pi is dead, then set=S is dead.
std::unique_ptr<Knowledge> ProofChecker::check_rule_pg(
        SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // Check if premise_ids[0] says that S[A] \subseteq S \cup S'.
    SubsetKnowledge<StateSet> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PG: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a subset knowledge.");
    }
    // Check if the left side of premise_ids[0] is S[A].
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_prog) || (s_prog->get_stateset_id() != stateset_id)) {
        throw std::runtime_error("Cannot apply rule PG: the left side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not the progression of set expression #" +
                                 std::to_string(stateset_id) + ".");
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        throw std::runtime_error("cannot apply rule PG: "
                                 "the progression is not over all actions.");
    }
    // Check if the right side of premise_ids[0] is S \cup S'.
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        throw std::runtime_error("Cannot apply rule PG: the right side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a union containing set expression #" +
                                 std::to_string(stateset_id) + ".");
    }

    int sp_id = s_cup_sp->get_right_id();

    // Check if premise_ids[1] says that S' is dead.
    DeadKnowledge *dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        throw std::runtime_error("Cannot apply rule PG: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state that set expression #" +
                                 std::to_string(sp_id) + " is dead.");
    }

    // Check if premise_ids[2] says that S \cap S_G^\Pi is dead.
    dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[2]].get());
    if (!dead_knowledge) {
        throw std::runtime_error("Cannot apply rule PG: knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not a dead set knowledge.");
    }
    StateSetIntersection *s_and_goal =
            dynamic_cast<StateSetIntersection *>(statesets[dead_knowledge->get_set_id()].get());
    // Check if left side of s_and_goal is S.
    if ((!s_and_goal) || (s_and_goal->get_left_id() != stateset_id)) {
        throw std::runtime_error("Cannot apply rule PG: the set expression "
                                 "declared dead in knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not an intersection with set expression #" +
                                 std::to_string(stateset_id) +
                                 " on the left side.");
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(statesets[s_and_goal->get_right_id()].get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        throw std::runtime_error("Cannot apply rule PG: the set expression "
                                 "declared dead in knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not an intersection with "
                                 "the constant goal set on the right side.");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}


// Progression Initial: Given (1) S[A] \subseteq S \cup S', (2) S' is dead and
// (3) {I} \subseteq S_not, then set=S_not is dead.
std::unique_ptr<Knowledge> ProofChecker::check_rule_pi(
        SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // Check if stateset corresponds to S_not.
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(statesets[stateset_id].get());
    if(!s_not) {
        throw std::runtime_error("Cannot apply rule PI: set expression #" +
                                 std::to_string(stateset_id) +
                                 " is not a negation.");
    }
    int s_id = s_not->get_child_id();

    // Check if premise_ids[0] says that S[A] \subseteq S \cup S'.
    auto subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PI: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not of a subset knowledge.");
    }
    // Check if the left side of premise_ids[0] is S[A].
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_prog) || (s_prog->get_stateset_id() != s_id)) {
        throw std::runtime_error("Cannot apply rule PI: the left side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not the progression of set expression #" +
                                 std::to_string(s_id) + ".");
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        throw std::runtime_error("cannot apply rule PI: "
                                 "the progression is not over all actions.");
    }
    // Check if the right side of premise_ids[0] is S \cup S'.
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != s_id)) {
        throw std::runtime_error("Cannot apply rule PI: the right side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a union containing set expression #" +
                                 std::to_string(s_id) + ".");
    }

    int sp_id = s_cup_sp->get_right_id();

    // Check if premise_ids[1] says that S' is dead.
    DeadKnowledge *dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        throw std::runtime_error("Cannot apply rule PI: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state that set expression #" +
                                 std::to_string(sp_id) + " is dead.");
    }

    // Check if premise_ids[2] says that {I} \subseteq S.
    subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[2]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PI: knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not a subset knowledge.");
    }
    // Check that the left side of premise_ids[2] is {I}.
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(statesets[subset_knowledge->get_left_id()].get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        throw std::runtime_error("Cannot apply rule PI: the left side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not the constant initial set.");
    }
    // Check that the right side of pemise_ids[2] is S.
    if(subset_knowledge->get_right_id() != s_id) {
        throw std::runtime_error("Cannot apply rule PI: the right side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not set expression" +
                                 std::to_string(s_id) + ".");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}


// Regression Goal: Given (1)[A]S \subseteq S \cup S', (2) S' is dead and
// (3) S_not \cap S_G^\Pi is dead, then S_not is dead.
std::unique_ptr<Knowledge> ProofChecker::check_rule_rg(
        SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // check if set corresponds to s_not
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(statesets[stateset_id].get());
    if(!s_not) {
        throw std::runtime_error("Cannot apply rule RG: set expression #" +
                                 std::to_string(stateset_id) +
                                 " is not a negation.");
    }
    int s_id = s_not->get_child_id();

    // Check if premise_ids[0] says that [A]S \subseteq S \cup S'.
    auto subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if(!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule RG: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a subset knowledge.");
    }
    // Check if the left side of premise_ids[0] is [A]S.
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_reg) || (s_reg->get_stateset_id() != s_id)) {
        throw std::runtime_error("Cannot apply rule RG: the left side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not the regeression of set expression #" +
                                 std::to_string(s_id) + ".");
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        throw std::runtime_error("Cannot apply rule RG: "
                                 "the regression is not over all actions.");
    }
    // Check if the right side of premise_ids[0] is S \cup S'.
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != s_id)) {
        throw std::runtime_error("Cannot apply rule RG: the right side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a union containing set expression #" +
                                 std::to_string(s_id) + ".");
    }

    int sp_id = s_cup_sp->get_right_id();

    // Check if premise_ids[1] says that S' is dead.
    DeadKnowledge *dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        throw std::runtime_error("Cannot apply rule RG: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state that set expression #" +
                                 std::to_string(sp_id) + " is dead.");
    }

    // Check if premise_ids[2] says that S_not \cap S_G(\Pi) is dead.
    dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[2]].get());
    if (!dead_knowledge) {
        throw std::runtime_error("Cannot apply rule RG: knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not a dead knowledge.");
    }
    StateSetIntersection *s_not_and_goal =
            dynamic_cast<StateSetIntersection *>(statesets[dead_knowledge->get_set_id()].get());
    // Check if the left side of s_not_and goal is S_not.
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_id() != stateset_id)) {
        throw std::runtime_error("Cannot apply rule RG: the set expression "
                                 "declared dead in knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not an intersection with set expression #" +
                                 std::to_string(stateset_id) +
                                 " on the left side.");
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(statesets[s_not_and_goal->get_right_id()].get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        throw std::runtime_error("Cannot apply rule RG: the set expression "
                                 "declared dead in knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not an intersection "
                                 "with the constant goal set on the right side.");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}


// Regression Initial: Given (1) [A]S \subseteq S \cup S', (2) S' is dead and
// (3) {I} \subseteq S_not, then set=S is dead.
std::unique_ptr<Knowledge> ProofChecker::check_rule_ri(
        SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // Check if premise_ids[0] says that [A]S \subseteq S \cup S'.
    auto subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if(!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule RI: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a subset knowledge.");
    }
    // Check if the left side of premise_ids[0] is [A]S.
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_reg) || (s_reg->get_stateset_id() != stateset_id)) {
        throw std::runtime_error("Cannot apply rule RI: the left side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not the regression of set expression #" +
                                 std::to_string(stateset_id) + ".");
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        throw std::runtime_error("Cannot apply rule RI: "
                                 "the regression is not over all actions.");
    }
    // Check f the right side of premise_ids[0] is S \cup S'.
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        throw std::runtime_error("Cannot apply rule RI: the right side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a union containing set expression #" +
                                 std::to_string(stateset_id) + ".");
    }

    int sp_id = s_cup_sp->get_right_id();

    // Check if premise_ids[1] says that S' is dead.
    DeadKnowledge *dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        throw std::runtime_error("Cannot apply rule RI: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state that set expression #" +
                                 std::to_string(sp_id) + " is dead");
    }

    // Check if premise_ids[2] says that {I} \subseteq S_not.
    subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[2]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule RI: knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not a subset knowledge.");
    }
    // Check that the left side of premise_ids[2] is {I}.
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(statesets[subset_knowledge->get_left_id()].get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        throw std::runtime_error("Cannot apply rule RI: the left side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not the constant initial set.");
    }
    // Check that right side of premise_ids[2] is S_not.
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_not) || s_not->get_child_id() != stateset_id) {
        throw std::runtime_error("Cannot apply rule RI: the right side "
                                 "of subset knowledge #" +
                                 std::to_string(premise_ids[2]) +
                                 " is not the negation of set expression #" +
                                 std::to_string(stateset_id) + ".");
    }

    return std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id));
}
*/

/*
 * CONCLUSION RULES
 */

// Conclusion Initial: Given (1) {I} is dead, then the task is unsolvable.
std::unique_ptr<Knowledge> ProofChecker::check_rule_ci(KnowledgeID premise_id) {
    assert(knowledgebase[premise_id] != nullptr);

    // Check that premise says that {I} is dead.
    DeadKnowledge *dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_id].get());
    if (!dead_knowledge) {
        throw std::runtime_error("Cannot apply rule CI: knowledge #" +
                                 std::to_string(premise_id) +
                                 " is not a dead knowledge.");
    }
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(statesets[dead_knowledge->get_set_id()].get());
    if ((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        throw std::runtime_error("Cannot apply rule CI: knowledge #" +
                                 std::to_string(premise_id) + " does not state " +
                                 "that the constant initial set is dead.");
    }

    return std::unique_ptr<Knowledge>(new UnsolvableKnowledge());
}

// Conclusion Goal: Given (1) S_G^\Pi is dead, then the task is unsolvable.
std::unique_ptr<Knowledge> ProofChecker::check_rule_cg(KnowledgeID premise_id) {
    assert(knowledgebase[premise_id] != nullptr);

    // Check that premise says that S_G^\Pi is dead.
    DeadKnowledge *dead_knowledge =
            dynamic_cast<DeadKnowledge *>(knowledgebase[premise_id].get());
    if (!dead_knowledge) {
        throw std::runtime_error("Cannot apply rule CG: knowledge #"
                                 + std::to_string(premise_id)
                                 + " is not a dead knowledge.");
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(statesets[dead_knowledge->get_set_id()].get());
    if ( (!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        throw std::runtime_error("Cannot apply rule CG: knowledge #" +
                                 std::to_string(premise_id) + " does not state" +
                                 " that the constant goal set is dead.");
    }

    return std::unique_ptr<Knowledge>(new UnsolvableKnowledge());
}

/*
 * SET THEORY RULES
 */

// Union Right: without premises, E \subseteq E \cup E'.
template<class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_ur(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetUnion *runion =
            dynamic_cast<const SetUnion *>(get_set_expression<T>(right_id));
    if (!runion) {
        throw std::runtime_error("Cannot apply rule UR: set expression #"
                                 + std::to_string(right_id)
                                 + " is not a union.");
    }
    if (runion->get_left_id() != left_id) {
        throw std::runtime_error("Cannot apply rule UR: set expression #"
                                 + std::to_string(right_id)
                                 + " is not a union with set expression #"
                                 + std::to_string(left_id)
                                 + " on its left side.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id, right_id));
}

// Union Left: Without premises, E \subseteq E' \cup E.
template<class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_ul(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetUnion *runion =
            dynamic_cast<const SetUnion *>(get_set_expression<T>(right_id));
    if (!runion) {
        throw std::runtime_error("Cannot apply rule UL: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a union.");
    }
    if (runion->get_right_id() != left_id) {
        throw std::runtime_error("Cannot apply rule UL: set expression #" +
                                 std::to_string(right_id) +
                                 " ist no a union with set expression #" +
                                 std::to_string(left_id) +
                                 " on its right side.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id, right_id));
}

// Intersection Right: Without premises, E \cap E' \subseteq E.
template<class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_ir(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetIntersection *lintersection =
            dynamic_cast<const SetIntersection *>(get_set_expression<T>(left_id));
    if (!lintersection) {
        throw std::runtime_error("Cannot apply rule IR: set expression #" +
                                 std::to_string(left_id) +
                                 " is not an intersection.");

    }
    if (lintersection->get_left_id() != right_id) {
        throw std::runtime_error("Cannot apply rule IR: set expression #" +
                                 std::to_string(left_id) +
                                 " is not an intersection with set expression" +
                                 std::to_string(right_id) +
                                 " on its left side.");

    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

// Intersection Left: Without premises, E' \cap E \subseteq E.
template <class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_il(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetIntersection *lintersection =
            dynamic_cast<const SetIntersection *>(get_set_expression<T>(left_id));
    if (!lintersection) {
        throw std::runtime_error("Cannot apply rule IL: set expression #" +
                                 std::to_string(left_id) +
                                 " is not an intersection.");

    }
    if (lintersection->get_right_id() != right_id) {
        throw std::runtime_error("Cannot apply rule IL: set expression #" +
                                 std::to_string(left_id) +
                                 " is not an intersection with set expression #" +
                                 std::to_string(right_id) +
                                 " on its right side.");

    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

// DIstributivity: Without premises,
// ((E \cup E') \cap E'') \subseteq ((E \cap E'') \cup (E' \cap E''))
template<class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_di(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    int e0,e1,e2;

    // Get sets E, E' and E'' from the left side.
    const SetIntersection *si =
            dynamic_cast<const SetIntersection *>(get_set_expression<T>(left_id));
    if (!si) {
        throw std::runtime_error("Cannot apply rule DI: set expression #" +
                                 std::to_string(left_id) +
                                 " is not an intersection.");

    }
    const SetUnion *su =
            dynamic_cast<const SetUnion *>(get_set_expression<T>(si->get_left_id()));
    if (!su) {
        throw std::runtime_error("Cannot apply rule DI: set expression #" +
                                 std::to_string(left_id) +
                                 " is not an intersection with a"
                                 " union on the left side.");
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = si->get_right_id();

    // Check if the right side matches the left.
    su = dynamic_cast<const SetUnion *>(get_set_expression<T>(right_id));
    if (!su) {
        throw std::runtime_error("Cannot apply rule DI: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a union.");
    }
    si = dynamic_cast<const SetIntersection *>(get_set_expression<T>(su->get_left_id()));
    if (!si) {
        throw std::runtime_error("Cannot apply rule DI: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a union with an"
                                 " intersection on the left side");
    }
    if (si->get_left_id() != e0 || si->get_right_id() != e2) {
        throw std::runtime_error("Cannot apply rule DI: left side of set expression #" +
                                 std::to_string(right_id) +
                                 " does not match the sets in set expression #" +
                                 std::to_string(left_id) + ".");
    }
    si = dynamic_cast<const SetIntersection *>(get_set_expression<T>(su->get_right_id()));
    if (!si) {
        throw std::runtime_error("Cannot apply rule DI: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a union with an"
                                 " intersection on the right side.");
    }
    if (si->get_left_id() != e1 || si->get_right_id() != e2) {
        throw std::runtime_error("Cannot apply rule DI: right side of set expression #" +
                                 std::to_string(right_id) +
                                 " does not match the sets in set expression #" +
                                 std::to_string(left_id) + ".");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

// Subset Union: Given (1) E \subseteq E'' and (2) E' \subseteq E'',
// then (E \cup E') \subseteq E''.
template<class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_su(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int e0,e1,e2;
    const StateSetUnion *su =
            dynamic_cast<const StateSetUnion *>(get_set_expression<T>(left_id));
    if (!su) {
        throw std::runtime_error("Cannot apply rule SU: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a union.");
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = right_id;

    SubsetKnowledge<T> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule SU: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a subset knowledge.");
    }
    if (subset_knowledge->get_left_id() != e0 || subset_knowledge->get_right_id() != e2) {
        throw std::runtime_error("Cannot apply rule SU: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state (E \\subseteq E'').");
    }
    subset_knowledge =
            dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule SU: knowledge #" +
                                 std::to_string(premise_ids[1])
                                 + " is not a subset knowledge.");
    }
    if (subset_knowledge->get_left_id() != e1 ||
            subset_knowledge->get_right_id() != e2) {
        throw std::runtime_error("Cannot apply rule SU: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state (E' \\subseteq E'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

// Subset Intersection: Given (1) E \subseteq E' and (2) E \subseteq E'',
// then E \subseteq (E' \cap E'').
template<class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_si(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int e0,e1,e2;
    const StateSetIntersection *si =
            dynamic_cast<const StateSetIntersection*>(get_set_expression<T>(right_id));
    if (!si) {
        throw std::runtime_error("Cannot apply rule SI: set expression #" +
                                 std::to_string(right_id) +
                                 " is not an intersection.");
    }
    e0 = left_id;
    e1 = si->get_left_id();
    e2 = si->get_right_id();

    SubsetKnowledge<T> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[0]].get());

    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule SI: knnowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a subset knowledge.");
    }
    if (subset_knowledge->get_left_id() != e0 ||
            subset_knowledge->get_right_id() != e1) {
        throw std::runtime_error("Cannot apply rule SI: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state (E \\subseteq E').");
    }
    subset_knowledge =
            dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule SI: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " is not a subset knowledge.");
    }
    if (subset_knowledge->get_left_id() != e0 ||
            subset_knowledge->get_right_id() != e2) {
        throw std::runtime_error("Cannot apply rule SI: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state (E \\subseteq E'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

// Subset Transitivity: Given (1) E \subseteq E' and (2) E' \subseteq E'',
// then E \subseteq E''.
template<class T>
std::unique_ptr<Knowledge> ProofChecker::check_rule_st(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int e0,e1,e2;
    e0 = left_id;
    e2 = right_id;

    SubsetKnowledge<T> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule ST: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a subset knowledge.");
    }
    if (subset_knowledge->get_left_id() != e0) {
        throw std::runtime_error("Cannot apply rule ST: knwoledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state (E \\subseteq E').");
    }
    e1 = subset_knowledge->get_right_id();
    subset_knowledge =
            dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule ST: knwoledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " is not a subset knowledge.");
    }
    if (subset_knowledge->get_left_id() != e1 ||
            subset_knowledge->get_right_id()!= e2) {
        throw std::runtime_error("Cannot apply rule ST: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state (E' \\subset E'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}


/*
 * RULES ABOUT PRO/REGRESSION
 */

// Action Transitivity: Given (1) S[A] \subseteq S' and (2) A' \subseteq A,
// then S[A'] \subseteq S'.
std::unique_ptr<Knowledge> ProofChecker::check_rule_at(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0, s1, a0, a1;
    s1 = right_id;
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        throw std::runtime_error("Cannot apply rule AT: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a progression.");
    }
    s0 = progression->get_stateset_id();
    a1 = progression->get_actionset_id();

    SubsetKnowledge<StateSet> *subset_knowledege0 =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledege0) {
        throw std::runtime_error("Cannot apply rule AT: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a state subset knowledge.");
    }
    progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledege0->get_left_id()));
    if (!progression || progression->get_stateset_id() != s0 ||
            subset_knowledege0->get_right_id() != s1) {
        throw std::runtime_error("Cannot apply rule AT: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state (S[A] \\subseteq S').");
    }
    a0 = progression->get_actionset_id();

    SubsetKnowledge<ActionSet> *subset_knowledege1 =
            dynamic_cast<SubsetKnowledge<ActionSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledege1) {
        throw std::runtime_error("Cannot apply rule AT: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " is not a state subset knowledge.");
    }
    if (subset_knowledege1->get_left_id() != a1 ||
            subset_knowledege1->get_right_id() != a0) {
        throw std::runtime_error("Cannot apply rule AT: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state (A' \\subseteq A");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

// Action Union: Given (1) S[A] \subseteq S' and (2) S[A'] \subseteq S',
// then S[A \cup A'] \subseteq S'.
std::unique_ptr<Knowledge> ProofChecker::check_rule_au(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0,s1,a0,a1;
    s1 = right_id;
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        throw std::runtime_error("Cannot apply rule AU: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a progression.");
    }
    s0 = progression->get_stateset_id();
    const ActionSetUnion *action_union =
            dynamic_cast<const ActionSetUnion *>(get_set_expression<ActionSet>(progression->get_actionset_id()));
    if (!action_union) {
        throw std::runtime_error("Cannot apply rule AU: set expression #" +
                                 std::to_string(left_id) +
                                 " is not progressing with (A \\cup A').");
    }
    a0 = action_union->get_left_id();
    a1 = action_union->get_right_id();

    SubsetKnowledge<StateSet> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule AU: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a state subset knowledge.");
    }
    progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
            progression->get_stateset_id() != s0 ||
            subset_knowledge->get_right_id() != s1) {
        throw std::runtime_error("Cannot apply rule AU: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state (S[A] \\subseteq S').");
    }

    subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule AU: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " is not a state subset knowledge.");
    }
    progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a1 ||
            progression->get_stateset_id() != s0 ||
            subset_knowledge->get_right_id() != s1) {
        throw std::runtime_error("Cannot apply rule AU: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state (S[A'] \\subseteq S').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

// Progression Transitivity: Given (1) S[A] \subseteq S'' and (2) S' \subseteq S,
// then S'[A] \subseteq S''.
std::unique_ptr<Knowledge> ProofChecker::check_rule_pt(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0,s1,s2,a0;
    s2 = right_id;
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        throw std::runtime_error("Cannot apply rule PT: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a progression.");
    }
    s1 = progression->get_stateset_id();
    s0 = progression->get_actionset_id();

    SubsetKnowledge<StateSet> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PT: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a state subset knowledge.");
    }
    progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
            progression->get_stateset_id() != s0 ||
            subset_knowledge->get_right_id() != s2) {
        throw std::runtime_error("Cannot apply rule PT: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state (S[A] \\subseteq S'').");
    }

    subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PT: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " is not a state subset knowledge.");
    }
    if (subset_knowledge->get_left_id() != s1 ||
            subset_knowledge->get_right_id() != s0) {
        throw std::runtime_error("Cannot apply rule PT: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state (S' \\subseteq S).");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}

// Progression Union: Given (1) S[A] \subseteq S'' and (2) S'[A] \subseteq S'',
// then (S \cup S')[A] \subseteq S''.
std::unique_ptr<Knowledge> ProofChecker::check_rule_pu(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0,s1,s2,a0;
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        throw std::runtime_error("Cannot apply rule PU: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a progression.");
    }
    const StateSetUnion *state_union =
            dynamic_cast<const StateSetUnion *>(get_set_expression<StateSet>(progression->get_stateset_id()));
    if (!state_union) {
        throw std::runtime_error("Cannot apply rule PU: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a progression of a state set union.");
    }
    s0 = state_union->get_left_id();
    s1 = state_union->get_right_id();
    s2 = right_id;
    a0 = progression->get_actionset_id();

    SubsetKnowledge<StateSet> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PU: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a state subset knowledge.");
    }
    progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
            progression->get_stateset_id() != s0 ||
            subset_knowledge->get_right_id() != s2) {
        throw std::runtime_error("Cannot apply rule PU: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state (S[A] \\subseteq S'').");
    }

    subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PU: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " is not a state subset knowledge.");
    }
    progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
            progression->get_stateset_id() != s1 ||
            subset_knowledge->get_right_id() != s2) {
        throw std::runtime_error("Cannot apply rule PU: knowledge #" +
                                 std::to_string(premise_ids[1]) +
                                 " does not state (S'[A] \\subseteq S'').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}


// Progression to Regression: Given (1) S[A] \subseteq S',
// then [A]S'_not \subseteq S_not.
std::unique_ptr<Knowledge> ProofChecker::check_rule_pr(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 1 &&
           knowledgebase[premise_ids[0]]);

    int s0,s1,a0;
    const StateSetRegression *regression =
            dynamic_cast<const StateSetRegression *>(get_set_expression<StateSet>(left_id));
    if (!regression) {
        throw std::runtime_error("Cannot apply rule PR: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a regression.");
    }
    const StateSetNegation *negation =
            dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(regression->get_stateset_id()));
    if (!negation) {
        throw std::runtime_error("Cannot apply rule PR: set expression #" +
                                 std::to_string(left_id) +
                                 " is not the regression of a negation.");
    }
    s1 = negation->get_child_id();
    a0 = regression->get_actionset_id();
    negation =
            dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(right_id));
    if (!negation) {
        throw std::runtime_error("Cannot apply rule PR: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a negation.");
    }
    s0 = negation->get_child_id();

    SubsetKnowledge<StateSet> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule PR: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a state subset knowledge.");
    }
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
            progression->get_stateset_id() != s0 ||
            subset_knowledge->get_right_id() != s1) {
        throw std::runtime_error("Cannot apply rule PR: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state S[A] \\subseteq S'.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}


// Regression to Progression: Given (1) [A]S \subseteq S',
// then S'_not[A] \subseteq S_not.
std::unique_ptr<Knowledge> ProofChecker::check_rule_rp(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 1 &&
           knowledgebase[premise_ids[0]]);

    int s0,s1,a0;
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        throw std::runtime_error("Cannot apply rule RP: set expression #" +
                                 std::to_string(left_id) +
                                 " is not a progression.");
    }
    const StateSetNegation *negation =
            dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(progression->get_stateset_id()));
    if (!negation) {
        throw std::runtime_error("Cannot apply rule RP: set expression #" +
                                 std::to_string(left_id) +
                                 " is not the progression of a negation.");
    }
    s1 = negation->get_child_id();
    a0 = progression->get_actionset_id();
    negation =
            dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(right_id));
    if (!negation) {
        throw std::runtime_error("Cannot apply rule RP: set expression #"
                                 + std::to_string(right_id) +
                                 " is not a negation.");
    }
    s0 = negation->get_child_id();

    SubsetKnowledge<StateSet> *subset_knowledge =
            dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        throw std::runtime_error("Cannot apply rule RP: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " is not a state subset knowledge.");
    }
    const StateSetRegression *regression =
            dynamic_cast<const StateSetRegression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!regression || regression->get_actionset_id() != a0 ||
            regression->get_stateset_id() != s0 ||
            subset_knowledge->get_right_id() != s1) {
        throw std::runtime_error("Cannot apply rule RP: knowledge #" +
                                 std::to_string(premise_ids[0]) +
                                 " does not state ([A]S \\subseteq S').");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id));
}



/*
 * BASIC STATEMENTS
 */

/*
 * Basic statements containing possibly negative set literals
 * are transformed into an equivalent statement that only contains set
 * variables without negation according to the following rule:
 * \bigcap L_p \land \bigcap \lnot L_n
 *   \subseteq \bigcup L_p' \cup \bigcup \lnot L_n'
 * iff
 * \bigcap L_p \land \bigcap L_n'
 *   \subseteq \bigcup L_n \cup \bigcup L_p'
 */

// B1: \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
std::unique_ptr<Knowledge> ProofChecker::check_statement_b1(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {

    std::vector<const StateSetVariable *> left;
    std::vector<const StateSetVariable *> right;
    bool valid_syntax = false;


    const StateSet *left_set = get_set_expression<StateSet>(left_id);
    valid_syntax = left_set->gather_intersection_variables(statesets, left, right);
    if (!valid_syntax) {
        throw std::runtime_error("Cannot apply statement B1: set expression #" +
                                 std::to_string(left_id) +
                                 " is not an interseciton of literals "
                                 "of the same type.");
    }
    const StateSet *right_set = get_set_expression<StateSet>(right_id);
    valid_syntax =  right_set->gather_union_variables(statesets, right, left);
    if (!valid_syntax) {
        throw std::runtime_error("Cannot check statement B1: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a union of literals "
                                 "of the same type.");
    }
    assert(left.size() + right.size() > 0);

    /*
     * Figure out in which formalism the statement is given.
     * Since the statement might contian any number of constants,
     * we need to find the first non-constant variable.
     */
    std::vector<const StateSetVariable *> allformulas;
    allformulas.reserve(left.size() + right.size());
    allformulas.insert(allformulas.end(), left.begin(), left.end());
    allformulas.insert(allformulas.end(), right.begin(), right.end());
    const StateSetFormalism *reference = get_reference_formula(allformulas);
    if (!reference) {
        throw std::runtime_error("Cannot apply statement B1: "
                                 "It consists of only constant sets.");
    }

    if (!reference->check_statement_b1(left, right)) {
        throw std::runtime_error("Statement B1 is false.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}


// B2: (\bigcap_{X \in \mathcal X} X)[A] \land \bigcap_{L \in \mathcal L} L
//     \subseteq \bigcup_{L' \in \mathcal L'} L'
std::unique_ptr<Knowledge> ProofChecker::check_statement_b2(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {

    std::vector<const StateSetVariable *> prog;
    std::vector<const StateSetVariable *> left;
    std::vector<const StateSetVariable *> right;
    std::unordered_set<int> actions;
    const StateSet *progressed_set = nullptr;
    const ActionSet *action_set = nullptr;
    const StateSet *left_set = nullptr;
    const StateSet *right_set = get_set_expression<StateSet>(right_id);

    /*
     * We expect the left side to either be a progression
     * or an intersection with a progression on the left side
     */
    const StateSetProgression *progression =
            dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) { // Left side is not a progression, is it an intersection?
        const StateSetIntersection *intersection =
                dynamic_cast<const StateSetIntersection *>(get_set_expression<StateSet>(left_id));
        if (!intersection) {
            throw std::runtime_error("Cannot apply statement B2: set expression #" +
                                     std::to_string(left_id) +
                                     " is not a progression or intersection.");
        }
        const StateSet *intersection_left =
                get_set_expression<StateSet>(intersection->get_left_id());
        progression =
                dynamic_cast<const StateSetProgression *>(intersection_left);
        if (!progression) {
            throw std::runtime_error("Cannot apply statement B2: set expression #" +
                                     std::to_string(left_id) +
                                     " is an interesction but not with a"
                                     " progression on the left side");
        }
        left_set = get_set_expression<StateSet>(intersection->get_right_id());
    }
    action_set = get_set_expression<ActionSet>(progression->get_actionset_id());
    action_set->get_actions(actionsets, actions);
    progressed_set = get_set_expression<StateSet>(progression->get_stateset_id());

    bool valid_syntax =
            progressed_set->gather_intersection_variables(statesets, prog, right);
    /*
     * In the progression, we only allow set variables, not set literals.
     * If right is not empty, then the progression contained set literals.
     */
    if (!valid_syntax || !right.empty()) {
        throw std::runtime_error("Cannot apply statement B2: the progression"
                                 " is not an intersection of set variables");
    }

    /*
     * left_set is the optional part of the left side of the subset relation
     * which is not progressed.
     */
    if(left_set) {
        valid_syntax = left_set->gather_intersection_variables(statesets, left, right);
        if(!valid_syntax) {
            throw std::runtime_error("Cannot apply statement B2: the non-"
                                     "progressed part in set expression #" +
                                     std::to_string(left_id) +
                                     " is not an intersection of set literals.");
        }
    }
    valid_syntax = right_set->gather_union_variables(statesets, right, left);
    if(!valid_syntax) {
        throw std::runtime_error("Cannot apply statement B2: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a union of set literals.");
    }
    assert(prog.size() > 0);

    std::vector<const StateSetVariable *> allformulas;
    allformulas.reserve(prog.size() + left.size() + right.size());
    allformulas.insert(allformulas.end(), prog.begin(), prog.end());
    allformulas.insert(allformulas.end(), left.begin(), left.end());
    allformulas.insert(allformulas.end(), right.begin(), right.end());
    const StateSetFormalism *reference = get_reference_formula(allformulas);
    if (!reference) {
        throw std::runtime_error("Cannot apply statement B2: "
                                 "It consists of only constant sets.");
    }

    if(!reference->check_statement_b2(prog, left, right, actions)) {
        throw std::runtime_error("Statement B2 is false.");
    }
    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}

// B3: [A](\bigcap_{X \in \mathcal X} X) \land \bigcap_{L \in \mathcal L} L
//     \subseteq \bigcup_{L' \in \mathcal L'} L'
std::unique_ptr<Knowledge> ProofChecker::check_statement_b3(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    std::vector<const StateSetVariable *> reg;
    std::vector<const StateSetVariable *> left;
    std::vector<const StateSetVariable *> right;
    std::unordered_set<int> actions;
    const StateSet *regressed_set = nullptr;
    const ActionSet *action_set = nullptr;
    const StateSet *left_set = nullptr;
    const StateSet *right_set = get_set_expression<StateSet>(right_id);

    /*
     * We expect the left side to either be a regression
     * or an intersection with a regression on the left side.
     */
    const StateSetRegression *regression =
            dynamic_cast<const StateSetRegression *>(get_set_expression<StateSet>(left_id));
    if (!regression) { // Left side is not a regression, is it an intersection?
        const StateSetIntersection *intersection =
                dynamic_cast<const StateSetIntersection *>(get_set_expression<StateSet>(left_id));
        if (!intersection) {
            throw std::runtime_error("Cannot apply statement B3: set expression #" +
                                     std::to_string(left_id) +
                                     " is not a regression or intersection.");
        }
        const StateSet *intersection_left =
                get_set_expression<StateSet>(intersection->get_left_id());
        regression =
                dynamic_cast<const StateSetRegression *>(intersection_left);
        if (!regression) {
            throw std::runtime_error("Cannot apply statement B3: set expression #" +
                                     std::to_string(left_id) +
                                     " is an intersection but not with a"
                                     " regression on the left side.");
        }
        left_set = get_set_expression<StateSet>(intersection->get_right_id());
    }
    action_set = get_set_expression<ActionSet>(regression->get_actionset_id());
    action_set->get_actions(actionsets, actions);
    regressed_set = get_set_expression<StateSet>(regression->get_stateset_id());


    bool valid_syntax =
            regressed_set->gather_intersection_variables(statesets, reg, right);
    /*
     * In the regression, we only allow set variables, not set literals.
     * If right is not empty, then reg contianed set literals.
     */
    if (!valid_syntax || !right.empty()) {
        throw std::runtime_error("Cannot apply statement B3: the regression"
                                 " is not an interesction of set variables.");
    }

    /*
     * left_set is the optional part of the left side of the subset relation
     * which is not regressed.
     */
    if(left_set) {
        valid_syntax = left_set->gather_intersection_variables(statesets, left, right);
        if(!valid_syntax) {
            throw std::runtime_error("Cannot apply statement B3: the non-"
                                     "regressed part in set expression #" +
                                     std::to_string(left_id) +
                                     " is ont an intersection of set literals.");
        }
    }
    valid_syntax = right_set->gather_union_variables(statesets, right, left);
    if(!valid_syntax) {
        throw std::runtime_error("Cannot apply statement B3: set expression #" +
                                 std::to_string(right_id) +
                                 " is not a union of set literals.");
    }
    assert(reg.size() > 0);

    std::vector<const StateSetVariable *> allformulas;
    allformulas.reserve(reg.size() + left.size() + right.size());
    allformulas.insert(allformulas.end(), reg.begin(), reg.end());
    allformulas.insert(allformulas.end(), left.begin(), left.end());
    allformulas.insert(allformulas.end(), right.begin(), right.end());
    const StateSetFormalism *reference = get_reference_formula(allformulas);
    if (!reference) {
        throw std::runtime_error("Cannot apply statement B3: "
                                 "It consists of only constant sets.");
    }

    if(!reference->check_statement_b3(reg, left, right, actions)) {
        throw std::runtime_error("Statement B3 is false.");
    }
    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}


// B4: L \subseteq L', where L and L' are represented by arbitrary formalisms
std::unique_ptr<Knowledge> ProofChecker::check_statement_b4(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    bool left_positive = true;
    bool right_positive = true;

    const StateSetFormalism *left =
            dynamic_cast<const StateSetFormalism *>(get_set_expression<StateSet>(left_id));
    if (!left) {
        const StateSetNegation *tmp =
            dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(left_id));
        if (tmp) {
            left = dynamic_cast<const StateSetFormalism *>(get_set_expression<StateSet>(tmp->get_child_id()));
        }
        if (!left) {
            throw std::runtime_error("Cannot apply statement B4; set expression #" +
                                     std::to_string(left_id) +
                                     " is not a non-constant set literal.");
        }
        left_positive = false;
    }

    const StateSetFormalism *right =
            dynamic_cast<const StateSetFormalism *>(get_set_expression<StateSet>(right_id));
    if (!right) {
        const StateSetNegation *tmp =
                dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(right_id));
        if (tmp) {
            right = dynamic_cast<const StateSetFormalism *>(get_set_expression<StateSet>(tmp->get_child_id()));
        }
        if (!right) {
            throw std::runtime_error("Cannot apply statement B4: set expression #" +
                                     std::to_string(right_id) +
                                     " is not a non-constant set literal.");
        }
        right_positive = false;
    }

    if(!left->check_statement_b4(right, left_positive, right_positive)) {
        throw std::runtime_error("Statement B4 is false.");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id));
}


// check if A \subseteq A'
std::unique_ptr<Knowledge> ProofChecker::check_statement_b5(
        SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    std::unordered_set<int> left_indices, right_indices;
    const ActionSet *left_set = get_set_expression<ActionSet>(right_id);
    const ActionSet *right_set = get_set_expression<ActionSet>(right_id);
    left_set->get_actions(actionsets, left_indices);
    right_set->get_actions(actionsets, right_indices);

    for (int index: left_indices) {
        if (right_indices.find(index) == right_indices.end()) {
            throw std::runtime_error("Statement B5 is false.");
        }
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<ActionSet>(left_id,right_id));
}

// TODO: would it be better to make a vector of vectors?
const StateSetFormalism *ProofChecker::get_reference_formula(std::vector<const StateSetVariable *> &vars) const {
    const StateSetFormalism *ret = nullptr;
    for (const StateSetVariable *var : vars) {
        ret = dynamic_cast<const StateSetFormalism *>(var);
        if(ret) {
            return ret;
        }
    }
    return nullptr;
}


bool ProofChecker::is_unsolvability_proven() {
    return unsolvability_proven;
}
