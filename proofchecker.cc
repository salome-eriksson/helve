#include "proofchecker.h"

#include "global_funcs.h"
#include "statesetcompositions.h"
#include "ssvconstant.h"

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

// TODO: should all error messages here be printed in cerr?

ProofChecker::ProofChecker(std::string &task_file)
    : task(task_file), unsolvability_proven(false) {

    check_dead_knowlege = {
        { "ed", [&](auto cid, auto sid, auto pids)
          {return check_rule_ed(cid, sid, pids);} },
        { "ud", [&](auto cid, auto sid, auto pids)
          {return check_rule_ud(cid, sid, pids);} },
        { "sd", [&](auto cid, auto sid, auto pids)
          {return check_rule_sd(cid, sid, pids);} },
        { "pg", [&](auto cid, auto sid, auto pids)
          {return check_rule_pg(cid, sid, pids);} },
        { "pi", [&](auto cid, auto sid, auto pids)
          {return check_rule_pi(cid, sid, pids);} },
        { "rg", [&](auto cid, auto sid, auto pids)
          {return check_rule_rg(cid, sid, pids);} },
        { "ri", [&](auto cid, auto sid, auto pids)
          {return check_rule_ri(cid, sid, pids);} },
    };

    check_subset_knowledge = {
        { "ura", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_ur<ActionSet>(cid, lid, rid, pids);} },
        { "urs", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_ur<StateSet>(cid, lid, rid, pids);} },
        { "ula", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_ul<ActionSet>(cid, lid, rid, pids);} },
        { "uls", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_ul<StateSet>(cid, lid, rid, pids);} },
        { "ira", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_ir<ActionSet>(cid, lid, rid, pids);} },
        { "irs", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_ir<StateSet>(cid, lid, rid, pids);} },
        { "ila", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_il<ActionSet>(cid, lid, rid, pids);} },
        { "ils", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_il<StateSet>(cid, lid, rid, pids);} },
        { "dia", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_di<ActionSet>(cid, lid, rid, pids);} },
        { "dis", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_di<StateSet>(cid, lid, rid, pids);} },
        { "sua", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_su<ActionSet>(cid, lid, rid, pids);} },
        { "sus", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_su<StateSet>(cid, lid, rid, pids);} },
        { "sia", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_si<ActionSet>(cid, lid, rid, pids);} },
        { "sis", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_si<StateSet>(cid, lid, rid, pids);} },
        { "sta", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_st<ActionSet>(cid, lid, rid, pids);} },
        { "sts", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_st<StateSet>(cid, lid, rid, pids);} },

        { "at", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_at(cid, lid, rid, pids);} },
        { "au", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_au(cid, lid, rid, pids);} },
        { "pt", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_pt(cid, lid, rid, pids);} },
        { "pu", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_pu(cid, lid, rid, pids);} },
        { "pr", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_pr(cid, lid, rid, pids);} },
        { "rp", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_rule_rp(cid, lid, rid, pids);} },

        { "b1", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_statement_B1(cid, lid, rid, pids);} },
        { "b2", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_statement_B2(cid, lid, rid, pids);} },
        { "b3", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_statement_B3(cid, lid, rid, pids);} },
        { "b4", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_statement_B4(cid, lid, rid, pids);} },
        { "b5", [&](auto cid, auto lid, auto rid, auto pids)
          {return check_statement_B5(cid, lid, rid, pids);} },
    };

    manager = Cudd(task.get_number_of_facts()*2);
    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();
}

template<>
const ActionSet *ProofChecker::get_set_expression<ActionSet>(SetID set_id) const {
    return actionsets[set_id].get();
}

template<>
const StateSet *ProofChecker::get_set_expression<StateSet>(SetID set_id) const {
    return statesets[set_id].get();
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
        std::cerr << "unknown expression type " << state_set_type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
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

    } else if(action_set_type.compare("a") == 0) { // constant (denoting the set of all actions)
        action_set = std::unique_ptr<ActionSet>(new ActionSetConstantAll(task));

    } else {
        std::cerr << "unknown actionset type " << action_set_type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    if (set_id >= actionsets.size()) {
        actionsets.resize(set_id+1);
    }
    assert(!actionsets[set_id]);
    actionsets[set_id] = std::move(action_set);
}

// TODO: unify error messages

// line format: <id> <type> <description>
void ProofChecker::verify_knowledge(std::string &line) {
    KnowledgeID knowledge_id;
    std::stringstream ssline(line);
    std::string word, rule, knowledge_type;
    bool knowledge_is_correct = false;

    ssline >> word;
    knowledge_id = get_id_from_string(word);
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
            std::cerr << "Rule " << rule << " is not a subset rule." << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        knowledge_is_correct =
                check_subset_knowledge[rule](knowledge_id, left_id, right_id, premises);

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

        if (check_dead_knowlege.find(rule) == check_dead_knowlege.end()) {
            std::cerr << " Rule " << rule << " is not a dead rule." << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        knowledge_is_correct =
                check_dead_knowlege[rule](knowledge_id, dead_set_id, premises);

    } else if(knowledge_type == "u") {
        // Unsolvability knowledge is defined by "<rule> <premise_id>".
        KnowledgeID premise;

        ssline >> rule;
        ssline >> word;
        premise = get_id_from_string(word);

        if (rule.compare("ci") == 0) {
            knowledge_is_correct = check_rule_ci(knowledge_id, premise);
        } else if (rule.compare("cg") == 0) {
            knowledge_is_correct = check_rule_cg(knowledge_id, premise);
        } else {
            std::cerr << "Rule " << rule << " is not an unsolvability rule." << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }

    } else {
        std::cerr << "unknown knowledge type " << knowledge_type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    if(!knowledge_is_correct) {
        std::cerr << "check for knowledge #" << knowledge_id << " NOT successful!" << std::endl;
    }
}


/*
 * RULES ABOUT DEADNESS
 */

// Emptyset Dead: set=emptyset is dead
bool ProofChecker::check_rule_ed(KnowledgeID conclusion_id, SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    SSVConstant *f = dynamic_cast<SSVConstant *>(statesets[stateset_id].get());
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        std::cerr << "Error when applying rule ED: set expression #" << stateset_id
                  << " is not the constant empty set." << std::endl;
        return false;
    }
    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Union Dead: given (1) S is dead and (2) S' is dead, set=S \cup S' is dead
bool ProofChecker::check_rule_ud(KnowledgeID conclusion_id, SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    // set represents S \cup S'
    StateSetUnion *f = dynamic_cast<StateSetUnion *>(statesets[stateset_id].get());
    if (!f) {
        std::cerr << "Error when applying rule UD to conclude knowledge #" << conclusion_id
                  << ": set expression #" << stateset_id << "is not a union." << std::endl;
        return false;
    }
    int left_id = f->get_left_id();
    int right_id = f->get_right_id();

    // check if premise_ids[0] says that S is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[0]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != left_id)) {
        std::cerr << "Error when applying rule UD: Knowledge #" << premise_ids[0]
                  << "does not state that set expression #" << left_id
                  << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[1] says that S' is dead
    dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != right_id)) {
        std::cerr << "Error when applying rule UD: Knowledge #" << premise_ids[1]
                  << "does not state that set expression #" << right_id
                  << " is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Subset Dead: given (1) S \subseteq S' and (2) S' is dead, set=S is dead
bool ProofChecker::check_rule_sd(KnowledgeID conclusion_id, SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    // check if premise_ids[0] says that S is a subset of S' (S' can be anything)
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge || (subset_knowledge->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule SD: knowledge #" << premise_ids[0]
                  << " does not state that set expression #" << stateset_id
                  << " is a subset of another set." << std::endl;
        return false;
    }

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != subset_knowledge->get_right_id())) {
        std::cerr << "Error when applying rule SD: knowledge #" << premise_ids[0]
                  << " states that set expression #" << stateset_id
                  << " is a subset of set expression #" << subset_knowledge->get_right_id()
                  << ", but knowledge #" << premise_ids[1] << " does not state that "
                  << subset_knowledge->get_right_id() << " is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}

// Progression Goal: given (1) S[A] \subseteq S \cup S', (2) S' is dead and (3) S \cap S_G^\Pi is dead,
// then set=S is dead
bool ProofChecker::check_rule_pg(KnowledgeID conclusion_id, SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // check if premise_ids[0] says that S[A] \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[0]
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_prog) || (s_prog->get_stateset_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the progression of set expression #" << stateset_id << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule PG: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check if the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << stateset_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that S \cap S_G^\Pi is dead
    dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[2]].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[2]
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_and_goal =
            dynamic_cast<StateSetIntersection *>(statesets[dead_knowledge->get_set_id()].get());
    // check if left side of s_and_goal is S
    if ((!s_and_goal) || (s_and_goal->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with set expression #" << stateset_id
                  << " on the left side." << std::endl;
        return false;
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(statesets[s_and_goal->get_right_id()].get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule PG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Progression Initial: given (1) S[A] \subseteq S \cup S', (2) S' is dead and (3) {I} \subseteq S_not,
// then set=S_not is dead
bool ProofChecker::check_rule_pi(KnowledgeID conclusion_id, SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(statesets[stateset_id].get());
    if(!s_not) {
        std::cerr << "Error when applying rule PI: set expression #" << stateset_id
                  << " is not a negation." << std::endl;
        return false;
    }
    int s_id = s_not->get_child_id();

    // check if premise_ids[0] says that S[A] \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[0]
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_prog) || (s_prog->get_stateset_id() != s_id)) {
        std::cerr << "Error when applying rule PI: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the progression of set expression #" << s_id << "." << std::endl;
        return false;
    }
    if(!actionsets[s_prog->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule PI: "
                     "the progression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != s_id)) {
        std::cerr << "Error when applying rule PI: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << s_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that {I} \subseteq S
    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[2]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[2]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise_ids[2] is {I}
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(statesets[subset_knowledge->get_left_id()].get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule PI: the left side of subset knowledge #" << premise_ids[2]
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of pemise3 is S
    if(subset_knowledge->get_right_id() != s_id) {
        std::cerr << "Error when applying rule PI: the right side of subset knowledge #" << premise_ids[2]
                  << " is not set expression #" << s_id << "." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Regression Goal: given (1)[A]S \subseteq S \cup S', (2) S' is dead and (3) S_not \cap S_G^\Pi is dead,
// then set=S_not is dead
bool ProofChecker::check_rule_rg(KnowledgeID conclusion_id, SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(statesets[stateset_id].get());
    if(!s_not) {
        std::cerr << "Error when applying rule RG: set expression #" << stateset_id
                  << " is not a negation." << std::endl;
        return false;
    }
    int s_id = s_not->get_child_id();

    // check if premise_ids[0] says that [A]S \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if(!subset_knowledge) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[0]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_reg) || (s_reg->get_stateset_id() != s_id)) {
        std::cerr << "Error when applying rule RG: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the regression of set expression #" << s_id << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule RG: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != s_id)) {
        std::cerr << "Error when applying rule RG: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << s_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that S_not \cap S_G(\Pi) is dead
    dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[2]].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[2]
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_not_and_goal =
            dynamic_cast<StateSetIntersection *>(statesets[dead_knowledge->get_set_id()].get());
    // check if left side of s_not_and goal is S_not
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule RG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with set expression #" << stateset_id
                  << " on the left side." << std::endl;
        return false;
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(statesets[s_not_and_goal->get_right_id()].get());
    if((!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule RG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with the constant goal set on the right side."
                  << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Regression Initial: given (1) [A]S \subseteq S \cup S', (2) S' is dead and (3) {I} \subseteq S_not,
// then set=S is dead
bool ProofChecker::check_rule_ri(KnowledgeID conclusion_id, SetID stateset_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]] &&
           knowledgebase[premise_ids[2]]);

    // check if premise_ids[0] says that [A]S \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if(!subset_knowledge) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[0]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(statesets[subset_knowledge->get_left_id()].get());
    if ((!s_reg) || (s_reg->get_stateset_id() != stateset_id)) {
        std::cerr << "Error when applying rule RI: the left side of subset knowledge #" << premise_ids[0]
                  << " is not the regression of set expression #" << stateset_id << "." << std::endl;
        return false;
    }
    if(!actionsets[s_reg->get_actionset_id()].get()->is_constantall()) {
        std::cerr << "Error when applying rule RI: "
                     "the regression does not speak about all actions" << std::endl;
        return false;
    }
    // check f the right side of premise_ids[0] is S \cup S'
    StateSetUnion *s_cup_sp =
            dynamic_cast<StateSetUnion *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule RI: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << stateset_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if k2 says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that {I} \subseteq S_not
    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[2]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[2]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise_ids[2] is {I}
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(statesets[subset_knowledge->get_left_id()].get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule RI: the left side of subset knowledge #" << premise_ids[2]
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of premise_ids[2] is S_not
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(statesets[subset_knowledge->get_right_id()].get());
    if((!s_not) || s_not->get_child_id() != stateset_id) {
        std::cerr << "Error when applying rule RI: the right side of subset knowledge #" << premise_ids[2]
                  << " is not the negation of set expression #" << stateset_id << "." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}

/*
 * CONCLUSION RULES
 */

// Conclusion Initial: given (1) {I} is dead, the task is unsolvable
bool ProofChecker::check_rule_ci(KnowledgeID conclusion_id, KnowledgeID premise_id) {
    assert(knowledgebase[premise_id] != nullptr);

    // check that premise says that {I} is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_id].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule CI: knowledge #" << premise_id
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(statesets[dead_knowledge->get_set_id()].get());
    if ((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule CI: knowledge #" << premise_id
                  << " does not state that the constant initial set is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new UnsolvableKnowledge()), conclusion_id);
    unsolvability_proven = true;
    return true;
}

// Conclusion Goal: given (1) S_G^\Pi is dead, the task is unsolvable
bool ProofChecker::check_rule_cg(KnowledgeID conclusion_id, KnowledgeID premise_id) {
    assert(knowledgebase[premise_id] != nullptr);

    // check that premise says that S_G^\Pi is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(knowledgebase[premise_id].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule CG: knowledge #" << premise_id
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(statesets[dead_knowledge->get_set_id()].get());
    if ( (!goal) || (goal->get_constant_type() != ConstantType::GOAL)) {
        std::cerr << "Error when applying rule CG: knowledge #" << premise_id
                  << " does not state that the constant goal set is dead." << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new UnsolvableKnowledge()), conclusion_id);
    unsolvability_proven = true;
    return true;
}

/*
 * SET THEORY RULES
 */

// Union Right: without premises, E \subseteq E \cup E'
template<class T>
bool ProofChecker::check_rule_ur(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetUnion *runion = dynamic_cast<const SetUnion *>(get_set_expression<T>(right_id));
    if (!runion) {
        std::cerr << "Error when applying rule UR: right side is not a union" << std::endl;
        return false;
    }
    if (!runion->get_left_id() != left_id) {
        std::cerr << "Error when applying rule UR: right does not have the form (left cup E')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id, right_id)), conclusion_id);
    return true;
}

// Union Left: without premises, E \subseteq E' \cup E
template<class T>
bool ProofChecker::check_rule_ul(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetUnion *runion = dynamic_cast<const SetUnion *>(get_set_expression<T>(right_id));
    if (!runion) {
        std::cerr << "Error when applying rule UL: right side is not a union" << std::endl;
        return false;
    }
    if (!runion->get_right_id() != left_id) {
        std::cerr << "Error when applying rule UL: right does not have the form (E' cup left)" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id, right_id)), conclusion_id);
    return true;
}

// Intersection Right: without premises, E \cap E' \subseteq E
template<class T>
bool ProofChecker::check_rule_ir(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetIntersection *lintersection = dynamic_cast<const SetIntersection *>(get_set_expression<T>(left_id));
    if (!lintersection) {
        std::cerr << "Error when applying rule IR: left side is not an intersection" << std::endl;
        return false;
    }
    if (!lintersection->get_left_id() != right_id) {
        std::cerr << "Error when applying rule IR: left does not have the form (right cap E')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id)), conclusion_id);
    return true;
}

// Intersection Left: without premises, E' \cap E \subseteq E
template <class T>
bool ProofChecker::check_rule_il(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    const SetIntersection *lintersection = dynamic_cast<const SetIntersection *>(get_set_expression<T>(left_id));
    if (!lintersection) {
        std::cerr << "Error when applying rule IL: left side is not an intersection" << std::endl;
        return false;
    }
    if (!lintersection->get_right_id() != right_id) {
        std::cerr << "Error when applying rule IL: left does not have the form (E' cap right)" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id)), conclusion_id);
    return true;
}

// DIstributivity: without premises, ((E \cup E') \cap E'') \subseteq ((E \cap E'') \cup (E' \cap E''))
template<class T>
bool ProofChecker::check_rule_di(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.empty());

    int e0,e1,e2;

    // left side
    const SetIntersection *si = dynamic_cast<const SetIntersection *>(get_set_expression<T>(left_id));
    if (!si) {
        std::cerr << "Error when applying rule DI: left is not an intersection" << std::endl;
        return false;
    }
    const SetUnion *su = dynamic_cast<const SetUnion *>(get_set_expression<T>(si->get_left_id()));
    if (!su) {
        std::cerr << "Error when applying rule DI: left side of left is not a union" << std::endl;
        return false;
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = si->get_right_id();

    // right side
    su = dynamic_cast<const SetUnion *>(get_set_expression<T>(right_id));
    if (!su) {
        std::cerr << "Error when applying rule DI: right is not a union" << std::endl;
        return false;
    }
    si = dynamic_cast<const SetIntersection *>(get_set_expression<T>(su->get_left_id()));
    if (!si) {
        std::cerr << "Error when applying rule DI: left side of right is not an intersection" << std::endl;
        return false;
    }
    if (si->get_left_id() != e0 || si->get_right_id() != e2) {
        std::cerr << "Error when applying rule DI: left side of right does not match with left" << std::endl;
        return false;
    }
    si = dynamic_cast<const SetIntersection *>(get_set_expression<T>(su->get_right_id()));
    if (!si) {

        std::cerr << "Error when applying rule DI: right side of right is not an intersection" << std::endl;
        return false;
    }
    if (si->get_left_id() != e1 || si->get_right_id() != e2) {
        std::cerr << "Error when applying rule DI: right side of right does not match with left" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id)), conclusion_id);
    return true;
}

// Subset Union: given (1) E \subseteq E'' and (2) E' \subseteq E'',
// then (E \cup E') \subseteq E''
template<class T>
bool ProofChecker::check_rule_su(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int e0,e1,e2;
    const StateSetUnion *su = dynamic_cast<const StateSetUnion *>(get_set_expression<T>(left_id));
    if (!su) {
        std::cerr << "Error when applying rule SU: left is not a union" << std::endl;
        return false;
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = right_id;

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e0 || subset_knowledge->get_right_id() != e2) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[0] << " does not state (E subset E'')" << std::endl;
        return false;
    }
    subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[1] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e1 || subset_knowledge->get_right_id() != e2) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[1] << " does not state (E' subset E'')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id)), conclusion_id);
    return true;
}

// Subset Intersection: given (1) E \subseteq E' and (2) E \subseteq E'',
// then E \subseteq (E' \cap E'')
template<class T>
bool ProofChecker::check_rule_si(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int e0,e1,e2;
    const StateSetIntersection *si = dynamic_cast<const StateSetIntersection*>(get_set_expression<T>(right_id));
    if (!si) {
        std::cerr << "Error when applying rule SI: right is not an intersection" << std::endl;
        return false;
    }
    e0 = left_id;
    e1 = si->get_left_id();
    e2 = si->get_right_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[0]].get());

    if (!subset_knowledge) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e0 || subset_knowledge->get_right_id() != e1) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E')" << std::endl;
        return false;
    }
    subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[1] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e0 || subset_knowledge->get_right_id() != e2) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[1] << " does not state (E subset E'')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id)), conclusion_id);
    return true;
}

// Subset Transitivity: given (1) E \subseteq E' and (2) E' \subseteq E'',
// then E \subseteq E''
template<class T>
bool ProofChecker::check_rule_st(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int e0,e1,e2;
    e0 = left_id;
    e2 = right_id;

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule ST: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e0) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E')" << std::endl;
        return false;
    }
    e1 = subset_knowledge->get_right_id();
    subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule ST: knowledge #" << premise_ids[1] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e1 || subset_knowledge->get_right_id()!= e2) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E' subset E'')" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id)), conclusion_id);
    return true;
}


/*
 * RULES ABOUT PRO/REGRESSION
 */

// Action Transitivity: given (1) S[A] \subseteq S' and (2) A' \subseteq A,
// then S[A'] \subseteq S'
bool ProofChecker::check_rule_at(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0, s1, a0, a1;
    s1 = right_id;
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        std::cerr << "Error when applying rule AT: left side is not a progression" << std::endl;
        return false;
    }
    s0 = progression->get_stateset_id();
    a1 = progression->get_actionset_id();

    auto subset_knowledege0 = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledege0) {
        std::cerr << "Error when applying rule AT: knowledge #" << premise_ids[0] << " is not state subset knowledge" << std::endl;
        return false;
    }
    progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledege0->get_left_id()));
    if (!progression || progression->get_stateset_id() != s0 || subset_knowledege0->get_right_id() != s1) {
        std::cerr << "Error when applying rule AT: knowledge #" << premise_ids[0] << " does not state S[A] subseteq S'" << std::endl;
        return false;
    }
    a0 = progression->get_actionset_id();

    auto subset_knowledege1 = dynamic_cast<SubsetKnowledge<ActionSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledege1) {
        std::cerr << "Error when applying rule AT: knowledge #" << premise_ids[1] << " is not state subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledege1->get_left_id() != a1 || subset_knowledege1->get_right_id() != a0) {
        std::cerr << "Error when applying rule AT: knwoledge #" << premise_ids[1] << " does not state A' subseteq A" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id)), conclusion_id);
    return true;
}

// Action Union: given (1) S[A] \subseteq S' and (2) S[A'] \subseteq S',
// then S[A \cup A'] \subseteq S'
bool ProofChecker::check_rule_au(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0,s1,a0,a1;
    s1 = right_id;
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        std::cerr << "Error when applying rule AU: left side is not a progression" << std::endl;
        return false;
    }
    s0 = progression->get_stateset_id();
    const ActionSetUnion *action_union = dynamic_cast<const ActionSetUnion *>(get_set_expression<ActionSet>(progression->get_actionset_id()));
    if (!action_union) {
        std::cerr << "Error when applying rule AU: left side is not a progression of A cup A'" << std::endl;
        return false;
    }
    a0 = action_union->get_left_id();
    a1 = action_union->get_right_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule AU: knowledge #" << premise_ids[0] << " is not state subset knowledge" << std::endl;
        return false;
    }
    progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
         progression->get_stateset_id() != s0 || subset_knowledge->get_right_id() != s1) {
        std::cerr << "Error when applying rule AU: knowledge #" << premise_ids[0] << " does not state S[A] subseteq S'" << std::endl;
        return false;
    }

    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule AU: knowledge #" << premise_ids[1] << " is not state subset knowledge" << std::endl;
        return false;
    }
    progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a1 ||
        progression->get_stateset_id() != s0 || subset_knowledge->get_right_id() != s1) {
        std::cerr << "Error when applying rule AU: knowledge #" << premise_ids[0] << " does not state S[A'] subseteq S'" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id)), conclusion_id);
    return true;
}

// Progression Transitivity: given (1) S[A] \subseteq S'' and (2) S' \subseteq S,
// then S'[A] \subseteq S''
bool ProofChecker::check_rule_pt(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0,s1,s2,a0;
    s2 = right_id;
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        std::cerr << "Error when applying rule PT: left side is not a progression" << std::endl;
        return false;
    }
    s1 = progression->get_stateset_id();
    s0 = progression->get_actionset_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PT: knowledge #" << premise_ids[0] << " is not state subset knowledge" << std::endl;
        return false;
    }
    progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
        progression->get_stateset_id() != s0 || subset_knowledge->get_right_id() != s2) {
        std::cerr << "Error when applying rule PT: knowledge #" << premise_ids[0] << " does not state S[A] subseteq S''" << std::endl;
        return false;
    }

    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PT: knowledge #" << premise_ids[1] << " is not state subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != s1 || subset_knowledge->get_right_id() != s0) {
        std::cerr << "Error when applying rule PT: knowledge #" << premise_ids[1] << " does not state S' subseteq S" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id)), conclusion_id);
    return true;
}

// Progression Union: given (1) S[A] \subseteq S'' and (2) S'[A] \subseteq S'',
// then (S \cup S')[A] \subseteq S''
bool ProofChecker::check_rule_pu(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           knowledgebase[premise_ids[0]] &&
           knowledgebase[premise_ids[1]]);

    int s0,s1,s2,a0;
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        std::cerr << "Error when applying rule PU: left side is not a progresion" << std::endl;
        return false;
    }
    const StateSetUnion *state_union = dynamic_cast<const StateSetUnion *>(get_set_expression<StateSet>(progression->get_stateset_id()));
    if (!state_union) {
        std::cerr << "Error when applying rule PU: left side is not a progression of a state set union" << std::endl;
        return false;
    }
    s0 = state_union->get_left_id();
    s1 = state_union->get_right_id();
    s2 = right_id;
    a0 = progression->get_actionset_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PU: knowledge #" << premise_ids[0] << " is not state subset knowledge" << std::endl;
        return false;
    }
    progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
        progression->get_stateset_id() != s0 || subset_knowledge->get_right_id() != s2) {
        std::cerr << "Error when applying rule PU: knowledge #" << premise_ids[0] << " does not state S[A] subseteq S''" << std::endl;
        return false;
    }

    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[1]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PU: knowledge #" << premise_ids[1] << " is not state subset knowledge" << std::endl;
        return false;
    }
    progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
        progression->get_stateset_id() != s1 || subset_knowledge->get_right_id() != s2) {
        std::cerr << "Error when applying rule PU: knowledge #" << premise_ids[1] << " does not state S'[A] subseteq S''" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id)), conclusion_id);
    return true;
}


// Progression to Regression: given (1) S[A] \subseteq S', then [A]S'_not \subseteq S_not
bool ProofChecker::check_rule_pr(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 1 &&
           knowledgebase[premise_ids[0]]);

    int s0,s1,a0;
    const StateSetRegression *regression = dynamic_cast<const StateSetRegression *>(get_set_expression<StateSet>(left_id));
    if (!regression) {
        std::cerr << "Error when applying rule PR: left side is not a regression" << std::endl;
        return false;
    }
    const StateSetNegation *negation = dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(regression->get_stateset_id()));
    if (!negation) {
        std::cerr << "Error when applying rule PR: left side is not a regression of a negation" << std::endl;
        return false;
    }
    s1 = negation->get_child_id();
    a0 = regression->get_actionset_id();
    negation = dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(right_id));
    if (!negation) {
        std::cerr << "Error when applying rule PR: right side is not a negation" << std::endl;
        return false;
    }
    s0 = negation->get_child_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PR: knowledge #" << premise_ids[0] << " is not state subset knowledge" << std::endl;
        return false;
    }
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!progression || progression->get_actionset_id() != a0 ||
            progression->get_stateset_id() != s0 || subset_knowledge->get_right_id() != s1) {
        std::cerr << "Error when applying rule PR: knowledge #" << premise_ids[0] << " does not state S[A] subseteq S'" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id)), conclusion_id);
    return true;
}


// Regression to Progression: given (1) [A]S \subseteq S', then S'_not[A] \subseteq S_not
bool ProofChecker::check_rule_rp(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &premise_ids) {
    assert(premise_ids.size() == 1 &&
           knowledgebase[premise_ids[0]]);

    int s0,s1,a0;
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        std::cerr << "Error when applying rule RP: left side is not a progression" << std::endl;
        return false;
    }
    const StateSetNegation *negation = dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(progression->get_stateset_id()));
    if (!negation) {
        std::cerr << "Error when applying rule RP: left side is not a progression of a negation" << std::endl;
        return false;
    }
    s1 = negation->get_child_id();
    a0 = progression->get_actionset_id();
    negation = dynamic_cast<const StateSetNegation *>(get_set_expression<StateSet>(right_id));
    if (!negation) {
        std::cerr << "Error when applying rule RP: right side is not a negation" << std::endl;
        return false;
    }
    s0 = negation->get_child_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(knowledgebase[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule RP: knowledge #" << premise_ids[0] << " is not state subset knowledge" << std::endl;
        return false;
    }
    const StateSetRegression *regression = dynamic_cast<const StateSetRegression *>(get_set_expression<StateSet>(subset_knowledge->get_left_id()));
    if (!regression || regression->get_actionset_id() != a0 ||
            regression->get_stateset_id() != s0 || subset_knowledge->get_right_id() != s1) {
        std::cerr << "Error when applying rule RP: knowledge #" << premise_ids[0] << " does not state [A]S subseteq S'" << std::endl;
        return false;
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id, right_id)), conclusion_id);
    return true;
}



/*
 * BASIC STATEMENTS
 */

// check if \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B1(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    bool ret = false;

    try {
        std::vector<const StateSetVariable *> left;
        std::vector<const StateSetVariable *> right;
        bool gather_variables_successfull = false;

        /*
         * Break up the statement into an intersection of variables on the left side
         * and a union of variables on the right side according to the following equivalency:
         * (1) \bigcap L_p \land \bigcap \lnot L_n \subseteq \bigcup L_p' \cup \bigcup \lnot L_n'
         * iff (2) \bigcap L_p \land \bigcap L_n' \subseteq \bigcup L_n \cup \bigcup L_p'
         */

        /*
         * variables in the left side of (1) belong to:
         *  - the left side of (2) if they occur positively
         *  - the right side of (2) if they occur negatively
         */
        gather_variables_successfull = get_set_expression<StateSet>(left_id)->gather_intersection_variables(statesets, left, right);
        if (!gather_variables_successfull) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(left_id)
                    + " is not a intersection of literals of the same type.";
            throw std::runtime_error(msg);
        }
        /*
         * variables in the right side of (1) belong to:
         *  - the right side of (2) if they occur positively
         *  - the left side of (2) if they occur negatively
         */        gather_variables_successfull =  get_set_expression<StateSet>(right_id)->gather_union_variables(statesets, right, left);
        if (!gather_variables_successfull) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(right_id)
                    + " is not a union of literals of the same type.";
            throw std::runtime_error(msg);
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
        const StateSetFormalism *reference_formula = get_reference_formula(allformulas);
        if (!reference_formula) {
            std::string msg = "Error when checking statement B1: no concrete subformula!";
            throw std::runtime_error(msg);
        }

        if (!reference_formula->check_statement_b1(left, right)) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(left_id) + " is not a subset of set expression #"
                    + std::to_string(right_id) + ".";
            throw std::runtime_error(msg);
        }

        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id)), conclusion_id);
        ret = true;
    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}


// check if (\bigcap_{X \in \mathcal X} X)[A] \land \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B2(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    bool ret = false;

    try {
        std::vector<const StateSetVariable *> prog;
        std::vector<const StateSetVariable *> left;
        std::vector<const StateSetVariable *> right;
        std::unordered_set<int> actions;
        const StateSet *prog_formula = nullptr;
        const StateSet *left_formula = nullptr;
        const StateSet *right_formula = get_set_expression<StateSet>(right_id);

        /*
         * We expect the left side to either be a progression or an intersection with
         * a progression on the left side
         */
        const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
        if (!progression) { // left side is not a progression, check if it is an intersection
            auto intersection = dynamic_cast<const StateSetIntersection *>(get_set_expression<StateSet>(left_id));
            if (!intersection) {
                std::string msg = "Error when checking statement B2: set expression #"
                        + std::to_string(left_id)
                        + " is not a progression or intersection.";
                throw std::runtime_error(msg);
            }
            progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(intersection->get_left_id()));
            if (!progression) { // the intersection does not have a progression on the left
                std::string msg = "Error when checking statement B2: set expression #"
                        + std::to_string(left_id)
                        + " is an intersection but not with a progression on the left side.";
                throw std::runtime_error(msg);
            }
            left_formula = get_set_expression<StateSet>(intersection->get_right_id());
        }
        const ActionSet *action_set = get_set_expression<ActionSet>(progression->get_actionset_id());
        action_set->get_actions(actionsets, actions);
        prog_formula = get_set_expression<StateSet>(progression->get_stateset_id());


        /*
         * In the progression, we only allow set variables, not set literals.
         * If right is not empty, then prog contianed set literals.
         */
        bool gather_variables_successfull = prog_formula->gather_intersection_variables(statesets, prog, right);
        if (!right.empty() || !gather_variables_successfull) {
            std::string msg = "Error when checking statement B2: "
                              "the progression in set expression #"
                    + std::to_string(left_id)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // The left_formula is empty if the left side contains only a progression.
        if(left_formula) {
            gather_variables_successfull = left_formula->gather_intersection_variables(statesets, left, right);
            if(!gather_variables_successfull) {
                std::string msg = "Error when checking statement B2: "
                                  "the non-progression part in set expression #"
                        + std::to_string(left_id)
                        + " is not an intersection of set literals.";
                throw std::runtime_error(msg);
            }
        }
        gather_variables_successfull = right_formula->gather_union_variables(statesets, right, left);
        if(!gather_variables_successfull) {
            std::string msg = "Error when checking statement B2: set expression #"
                    + std::to_string(right_id)
                    + " is not a union of literals.";
            throw std::runtime_error(msg);
        }
        assert(prog.size() > 0);

        std::vector<const StateSetVariable *> allformulas;
        allformulas.reserve(prog.size() + left.size() + right.size());
        allformulas.insert(allformulas.end(), prog.begin(), prog.end());
        allformulas.insert(allformulas.end(), left.begin(), left.end());
        allformulas.insert(allformulas.end(), right.begin(), right.end());
        const StateSetFormalism *reference_formula = get_reference_formula(allformulas);
        if (!reference_formula) {
            std::string msg = "Error when checking statement B2: no concrete subformula!";
            throw std::runtime_error(msg);
        }

        if(!reference_formula->check_statement_b2(prog, left, right, actions)) {
            std::string msg = "Error when checking statement B2: set expression #"
                    + std::to_string(left_id) + " is not a subset of set expression #"
                    + std::to_string(right_id) + ".";
            throw std::runtime_error(msg);
        }
        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id)), conclusion_id);
        ret = true;

    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}

// check if [A](\bigcap_{X \in \mathcal X} X) \land \bigcap_{L \in \mathcal L} L \subseteq \bigcup_{L' \in \mathcal L'} L'
bool ProofChecker::check_statement_B3(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    bool ret = false;

    try {
        std::vector<const StateSetVariable *> reg;
        std::vector<const StateSetVariable *> left;
        std::vector<const StateSetVariable *> right;
        std::unordered_set<int> actions;
        const StateSet *reg_formula = nullptr;
        const StateSet *left_formula = nullptr;
        const StateSet *right_formula = get_set_expression<StateSet>(right_id);

        /*
         * We expect the left side to either be a regression or an intersection with
         * a regression on the left side
         */
        const StateSetRegression *regression = dynamic_cast<const StateSetRegression *>(get_set_expression<StateSet>(left_id));
        if (!regression) { // left side is not a regression, check if it is an intersection
            const StateSetIntersection *intersection = dynamic_cast<const StateSetIntersection *>(get_set_expression<StateSet>(left_id));
            if (!intersection) {
                std::string msg = "Error when checking statement B3: set expression #"
                        + std::to_string(left_id)
                        + " is not a regression or intersection.";
                throw std::runtime_error(msg);
            }
            regression = dynamic_cast<const StateSetRegression *>(get_set_expression<StateSet>(intersection->get_left_id()));
            if (!regression) { // the intersection does not have a regression on the left
                std::string msg = "Error when checking statement B3: set expression #"
                        + std::to_string(left_id)
                        + " is an intersection but not with a regression on the left side.";
                throw std::runtime_error(msg);
            }
            left_formula = get_set_expression<StateSet>(intersection->get_right_id());
        }
        const ActionSet *action_set = get_set_expression<ActionSet>(regression->get_actionset_id());
        action_set->get_actions(actionsets, actions);
        reg_formula = get_set_expression<StateSet>(regression->get_stateset_id());


        /*
         * In the regression, we only allow set variables, not set literals.
         * If right is not empty, then reg contianed set literals.
         */
        bool gather_variables_successfull = reg_formula->gather_intersection_variables(statesets, reg, right);
        if (!right.empty() || !gather_variables_successfull) {
            std::string msg = "Error when checking statement B3: "
                              "the regression in set expression #"
                    + std::to_string(left_id)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // left_formula is empty if the left side contains only a regression
        if(left_formula) {
            gather_variables_successfull = left_formula->gather_intersection_variables(statesets, left, right);
            if(!gather_variables_successfull) {
                std::string msg = "Error when checking statement B3: "
                                  "the non-regression part in set expression #"
                        + std::to_string(left_id)
                        + " is not an intersection of set literals.";
                throw std::runtime_error(msg);
            }
        }
        gather_variables_successfull = right_formula->gather_union_variables(statesets, right, left);
        if(!gather_variables_successfull) {
            std::string msg = "Error when checking statement B3: set expression #"
                    + std::to_string(right_id)
                    + " is not a union of literals.";
            throw std::runtime_error(msg);
        }
        assert(reg.size() > 0);

        std::vector<const StateSetVariable *> allformulas;
        allformulas.reserve(reg.size() + left.size() + right.size());
        allformulas.insert(allformulas.end(), reg.begin(), reg.end());
        allformulas.insert(allformulas.end(), left.begin(), left.end());
        allformulas.insert(allformulas.end(), right.begin(), right.end());
        const StateSetFormalism *reference_formula = get_reference_formula(allformulas);
        if (!reference_formula) {
            std::string msg = "Error when checking statement B1: no concrete subformula!";
            throw std::runtime_error(msg);
        }

        if(!reference_formula->check_statement_b3(reg, left, right, actions)) {
            std::string msg = "Error when checking statement B3: set expression #"
                    + std::to_string(left_id) + " is not a subset of set expression #"
                    + std::to_string(right_id) + ".";
            throw std::runtime_error(msg);
        }
        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id)), conclusion_id);
        ret = true;

    } catch(std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}


// check if L \subseteq L', where L and L' might be represented by different formalisms
bool ProofChecker::check_statement_B4(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    bool ret = true;

    try {
        bool left_positive = true;
        bool right_positive = true;
        const StateSet *left = get_set_expression<StateSet>(left_id);
        const StateSet *right = get_set_expression<StateSet>(right_id);
        const StateSetFormalism *lformula = dynamic_cast<const StateSetFormalism *>(left);
        const StateSetFormalism *rformula = dynamic_cast<const StateSetFormalism *>(right);
        // TODO: document that we do not allow constants in B4 (we can use B1, every formalism should support CL...)

        if (!lformula) {
            const StateSetNegation *lvar_neg = dynamic_cast<const StateSetNegation *>(left);
            if (!lvar_neg) {
                std::string msg = "Error when checking statement B4: set expression #"
                        + std::to_string(left_id) + " is not a set literal.";
                throw std::runtime_error(msg);
            }
            lformula = dynamic_cast<const StateSetFormalism *>(get_set_expression<StateSet>(lvar_neg->get_child_id()));
            left_positive = false;
            if (!lformula) {
                std::string msg = "Error when checking statement B4: set expression #"
                        + std::to_string(left_id) + " is not a set literal.";
                throw std::runtime_error(msg);
            }
        }
        if (!rformula) {
            const StateSetNegation *rvar_neg = dynamic_cast<const StateSetNegation *>(left);
            if (!rvar_neg) {
                std::string msg = "Error when checking statement B4: set expression #"
                        + std::to_string(right_id) + " is not a set literal.";
                throw std::runtime_error(msg);
            }
            rformula = dynamic_cast<const StateSetFormalism *>(get_set_expression<StateSet>(rvar_neg->get_child_id()));
            right_positive = false;
            if (!rformula) {
                std::string msg = "Error when checking statement B4: set expression #"
                        + std::to_string(right_id) + " is not a set literal.";
                throw std::runtime_error(msg);
            }
        }

        if(!lformula->check_statement_b4(rformula, left_positive, right_positive)) {
            std::string msg = "Error when checking statement B4: set expression #"
                    + std::to_string(left_id) + " is not a subset of set expression #"
                    + std::to_string(right_id) + ".";
            throw std::runtime_error(msg);
        }

        add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<StateSet>(left_id,right_id)), conclusion_id);
    } catch (std::runtime_error e) {
        std::cerr << e.what();
        ret = false;
    }
    return ret;
}


// check if A \subseteq A'
bool ProofChecker::check_statement_B5(KnowledgeID conclusion_id, SetID left_id, SetID right_id, std::vector<KnowledgeID> &) {
    std::unordered_set<int> left_indices, right_indices;
    const ActionSet *left_set = get_set_expression<ActionSet>(right_id);
    const ActionSet *right_set = get_set_expression<ActionSet>(right_id);
    left_set->get_actions(actionsets, left_indices);
    right_set->get_actions(actionsets, right_indices);

    for (int index: left_indices) {
        if (right_indices.find(index) == right_indices.end()) {
            std::cerr << "Error when checking statement B5: action set #"
                      << left_id << " is not a subset of action set #" << right_id << "." << std::endl;
            return false;
        }
    }

    add_knowledge(std::unique_ptr<Knowledge>(new SubsetKnowledge<ActionSet>(left_id,right_id)), conclusion_id);
    return true;
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
