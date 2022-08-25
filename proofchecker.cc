#include "proofchecker.h"

#include "global_funcs.h"
#include "statesetcompositions.h"
#include "ssvconstant.h"

#include <cassert>
#include <cmath>
#include <iostream>

// TODO: should all error messages here be printed in cerr?

ProofChecker::ProofChecker(std::string &task_file)
    : task(task_file), unsolvability_proven(false) {

    using namespace std::placeholders;
    dead_knowledge_functions = {
        { "ed", std::bind(&ProofChecker::check_rule_ed, this, _1, _2, _3) },
        { "ud", std::bind(&ProofChecker::check_rule_ud, this, _1, _2, _3) },
        { "sd", std::bind(&ProofChecker::check_rule_sd, this, _1, _2, _3) },
        { "pg", std::bind(&ProofChecker::check_rule_pg, this, _1, _2, _3) },
        { "pi", std::bind(&ProofChecker::check_rule_pi, this, _1, _2, _3) },
        { "rg", std::bind(&ProofChecker::check_rule_rg, this, _1, _2, _3) },
        { "ri", std::bind(&ProofChecker::check_rule_ri, this, _1, _2, _3) }
    };

    subset_knowledge_functions = {
        { "ura", std::bind(&ProofChecker::check_rule_ur<ActionSet>, this, _1, _2, _3, _4) },
        { "urs", std::bind(&ProofChecker::check_rule_ur<StateSet>, this, _1, _2, _3, _4) },
        { "ula", std::bind(&ProofChecker::check_rule_ul<ActionSet>, this, _1, _2, _3, _4) },
        { "uls", std::bind(&ProofChecker::check_rule_ul<StateSet>, this, _1, _2, _3, _4) },
        { "ira", std::bind(&ProofChecker::check_rule_ir<ActionSet>, this, _1, _2, _3, _4) },
        { "irs", std::bind(&ProofChecker::check_rule_ir<StateSet>, this, _1, _2, _3, _4) },
        { "ila", std::bind(&ProofChecker::check_rule_il<ActionSet>, this, _1, _2, _3, _4) },
        { "ils", std::bind(&ProofChecker::check_rule_il<StateSet>, this, _1, _2, _3, _4) },
        { "dia", std::bind(&ProofChecker::check_rule_di<ActionSet>, this, _1, _2, _3, _4) },
        { "dis", std::bind(&ProofChecker::check_rule_di<StateSet>, this, _1, _2, _3, _4) },
        { "sua", std::bind(&ProofChecker::check_rule_su<ActionSet>, this, _1, _2, _3, _4) },
        { "sus", std::bind(&ProofChecker::check_rule_su<StateSet>, this, _1, _2, _3, _4) },
        { "sia", std::bind(&ProofChecker::check_rule_si<ActionSet>, this, _1, _2, _3, _4) },
        { "sis", std::bind(&ProofChecker::check_rule_si<StateSet>, this, _1, _2, _3, _4) },
        { "sta", std::bind(&ProofChecker::check_rule_st<ActionSet>, this, _1, _2, _3, _4) },
        { "sts", std::bind(&ProofChecker::check_rule_st<StateSet>, this, _1, _2, _3, _4) },

        { "at", std::bind(&ProofChecker::check_rule_at, this, _1, _2, _3, _4) },
        { "au", std::bind(&ProofChecker::check_rule_au, this, _1, _2, _3, _4) },
        { "pt", std::bind(&ProofChecker::check_rule_pt, this, _1, _2, _3, _4) },
        { "pu", std::bind(&ProofChecker::check_rule_pu, this, _1, _2, _3, _4) },
        { "pr", std::bind(&ProofChecker::check_rule_pr, this, _1, _2, _3, _4) },
        { "rp", std::bind(&ProofChecker::check_rule_rp, this, _1, _2, _3, _4) },

        { "b1", std::bind(&ProofChecker::check_statement_B1, this, _1, _2, _3, _4) },
        { "b2", std::bind(&ProofChecker::check_statement_B2, this, _1, _2, _3, _4) },
        { "b3", std::bind(&ProofChecker::check_statement_B3, this, _1, _2, _3, _4) },
        { "b4", std::bind(&ProofChecker::check_statement_B4, this, _1, _2, _3, _4) },
        { "b5", std::bind(&ProofChecker::check_statement_B5, this, _1, _2, _3, _4) },
    };

    manager = Cudd(task.get_number_of_facts()*2);
    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();
    std::cout << "Amount of Actions: " << task.get_number_of_actions() << std::endl;
}

template<>
const ActionSet *ProofChecker::get_set_expression<ActionSet>(int set_id) const {
    return actionsets[set_id].get();
}

template<>
const StateSet *ProofChecker::get_set_expression<StateSet>(int set_id) const {
    return formulas[set_id].get();
}

void ProofChecker::add_knowledge(std::unique_ptr<Knowledge> entry, int id) {
    assert(id >= kbentries.size());
    if(id > kbentries.size()) {
        kbentries.resize(id);
    }
    kbentries.push_back(std::move(entry));
}

void ProofChecker::add_state_set(std::string &line) {
    std::stringstream ssline(line);
    int expression_index;
    ssline >> expression_index;
    std::string word;
    ssline >> word;
    auto stateset_constructors = StateSet::get_stateset_constructors();
    if (stateset_constructors->find(word) == stateset_constructors->end()) {
        std::cerr << "unknown expression type " << word << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    std::unique_ptr<StateSet> expression = StateSet::get_stateset_constructors()->at(word)(ssline, task);
    if (expression_index >= formulas.size()) {
        formulas.resize(expression_index+1);
    }
    assert(!formulas[expression_index]);
    formulas[expression_index] = std::move(expression);
}


void ProofChecker::add_action_set(std::string &line) {
    std::stringstream ssline(line);
    int action_index;
    std::unique_ptr<ActionSet> action_set;
    ssline >> action_index;
    std::string type;
    // read in action type
    ssline >> type;

    if(type.compare("b") == 0) { // basic enumeration of actions
        int amount;
        // the first number denotes the amount of actions being enumerated
        std::unordered_set<int> actions;
        ssline >> amount;
        int a;
        for(int i = 0; i < amount; ++i) {
            ssline >> a;
            actions.insert(a);
        }
        action_set = std::unique_ptr<ActionSet>(new ActionSetBasic(actions));

    } else if(type.compare("u") == 0) { // union of action sets
        int left_id, right_id;
        ssline >> left_id;
        ssline >> right_id;
        action_set = std::unique_ptr<ActionSet>(new ActionSetUnion(left_id, right_id));

    } else if(type.compare("a") == 0) { // constant (denoting the set of all actions)
        action_set = std::unique_ptr<ActionSet>(new ActionSetConstantAll(task));

    } else {
        std::cerr << "unknown actionset type " << type << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }

    if (action_index >= actionsets.size()) {
        actionsets.resize(action_index+1);
    }
    assert(!actionsets[action_index]);
    actionsets[action_index] = std::move(action_set);
}

// TODO: unify error messages

void ProofChecker::verify_knowledge(std::string &line) {
    int knowledge_id;
    std::stringstream ssline(line);
    ssline >> knowledge_id;
    bool knowledge_is_correct = false;

    std::string word;
    // read in knowledge type
    ssline >> word;

    if(word == "s") { // subset knowledge
        int left_id, right_id, tmp;
        std::vector<int> premises;
        // reserve max amount of premises (currently 2)
        premises.reserve(2);

        ssline >> left_id;
        ssline >> right_id;
        // read in with which basic statement or derivation rule this knowledge should be checked
        ssline >> word;
        // read in premises
        while (ssline >> tmp) {
            premises.push_back(tmp);
        }

        if (subset_knowledge_functions.find(word) == subset_knowledge_functions.end()) {
            std::cerr << "unknown justification for subset knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        knowledge_is_correct = subset_knowledge_functions[word](knowledge_id, left_id, right_id, premises);

    } else if(word == "d") { // dead knowledge
        int dead_set_id, tmp;
        std::vector<int> premises;
        // reserve max amount of premises (currently 3)
        premises.reserve(2);

        ssline >> dead_set_id;
        // read in with which derivation rule this knowledge should be checked
        ssline >> word;
        // read in premises
        while (ssline >> tmp) {
            premises.push_back(tmp);
        }

        if (dead_knowledge_functions.find(word) == dead_knowledge_functions.end()) {
            std::cerr << "unknown justification for dead set knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
        knowledge_is_correct = dead_knowledge_functions[word](knowledge_id, dead_set_id, premises);

    } else if(word == "u") { // unsolvability knowledge
        int premise;

        // read in with which derivation rule unsolvability should be proven
        ssline >> word;
        ssline >> premise;

        if (word.compare("ci") == 0) {
            knowledge_is_correct = check_rule_ci(knowledge_id, premise);
        } else if (word.compare("cg") == 0) {
            knowledge_is_correct = check_rule_cg(knowledge_id, premise);
        } else {
            std::cerr << "unknown justification for unsolvability knowledge " << word << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }

    } else {
        std::cerr << "unknown knowledge type " << word << std::endl;
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
bool ProofChecker::check_rule_ed(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.empty());

    SSVConstant *f = dynamic_cast<SSVConstant *>(formulas[stateset_id].get());
    if ((!f) || (f->get_constant_type() != ConstantType::EMPTY)) {
        std::cerr << "Error when applying rule ED: set expression #" << stateset_id
                  << " is not the constant empty set." << std::endl;
        return false;
    }
    add_knowledge(std::unique_ptr<Knowledge>(new DeadKnowledge(stateset_id)), conclusion_id);
    return true;
}


// Union Dead: given (1) S is dead and (2) S' is dead, set=S \cup S' is dead
bool ProofChecker::check_rule_ud(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

    // set represents S \cup S'
    StateSetUnion *f = dynamic_cast<StateSetUnion *>(formulas[stateset_id].get());
    if (!f) {
        std::cerr << "Error when applying rule UD to conclude knowledge #" << conclusion_id
                  << ": set expression #" << stateset_id << "is not a union." << std::endl;
        return false;
    }
    int left_id = f->get_left_id();
    int right_id = f->get_right_id();

    // check if premise_ids[0] says that S is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[0]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != left_id)) {
        std::cerr << "Error when applying rule UD: Knowledge #" << premise_ids[0]
                  << "does not state that set expression #" << left_id
                  << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[1] says that S' is dead
    dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_sd(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

    // check if premise_ids[0] says that S is a subset of S' (S' can be anything)
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
    if (!subset_knowledge || (subset_knowledge->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule SD: knowledge #" << premise_ids[0]
                  << " does not state that set expression #" << stateset_id
                  << " is a subset of another set." << std::endl;
        return false;
    }

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_pg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]] &&
           kbentries[premise_ids[2]]);

    // check if premise_ids[0] says that S[A] \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[0]
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[subset_knowledge->get_left_id()].get());
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
            dynamic_cast<StateSetUnion *>(formulas[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << stateset_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that S \cap S_G^\Pi is dead
    dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[2]].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule PG: knowledge #" << premise_ids[2]
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_and_goal =
            dynamic_cast<StateSetIntersection *>(formulas[dead_knowledge->get_set_id()].get());
    // check if left side of s_and_goal is S
    if ((!s_and_goal) || (s_and_goal->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule PG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with set expression #" << stateset_id
                  << " on the left side." << std::endl;
        return false;
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(formulas[s_and_goal->get_right_id()].get());
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
bool ProofChecker::check_rule_pi(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]] &&
           kbentries[premise_ids[2]]);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(formulas[stateset_id].get());
    if(!s_not) {
        std::cerr << "Error when applying rule PI: set expression #" << stateset_id
                  << " is not a negation." << std::endl;
        return false;
    }
    int s_id = s_not->get_child_id();

    // check if premise_ids[0] says that S[A] \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[0]
                  << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is S[A]
    StateSetProgression *s_prog =
            dynamic_cast<StateSetProgression *>(formulas[subset_knowledge->get_left_id()].get());
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
            dynamic_cast<StateSetUnion *>(formulas[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != s_id)) {
        std::cerr << "Error when applying rule PI: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << s_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that {I} \subseteq S
    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[2]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule PI: knowledge #" << premise_ids[2]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise_ids[2] is {I}
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(formulas[subset_knowledge->get_left_id()].get());
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
bool ProofChecker::check_rule_rg(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]] &&
           kbentries[premise_ids[2]]);

    // check if set corresponds to s_not
    StateSetNegation *s_not = dynamic_cast<StateSetNegation *>(formulas[stateset_id].get());
    if(!s_not) {
        std::cerr << "Error when applying rule RG: set expression #" << stateset_id
                  << " is not a negation." << std::endl;
        return false;
    }
    int s_id = s_not->get_child_id();

    // check if premise_ids[0] says that [A]S \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
    if(!subset_knowledge) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[0]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[subset_knowledge->get_left_id()].get());
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
            dynamic_cast<StateSetUnion *>(formulas[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != s_id)) {
        std::cerr << "Error when applying rule RG: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << s_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if premise_ids[1] says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that S_not \cap S_G(\Pi) is dead
    dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[2]].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule RG: knowledge #" << premise_ids[2]
                 << " is not of type DEAD." << std::endl;
        return false;
    }
    StateSetIntersection *s_not_and_goal =
            dynamic_cast<StateSetIntersection *>(formulas[dead_knowledge->get_set_id()].get());
    // check if left side of s_not_and goal is S_not
    if ((!s_not_and_goal) || (s_not_and_goal->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule RG: the set expression declared dead in knowledge #"
                  << premise_ids[2] << " is not an intersection with set expression #" << stateset_id
                  << " on the left side." << std::endl;
        return false;
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(formulas[s_not_and_goal->get_right_id()].get());
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
bool ProofChecker::check_rule_ri(int conclusion_id, int stateset_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 3 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]] &&
           kbentries[premise_ids[2]]);

    // check if premise_ids[0] says that [A]S \subseteq S \cup S'
    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
    if(!subset_knowledge) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[0]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check if the left side of premise_ids[0] is [A]S
    StateSetRegression *s_reg =
            dynamic_cast<StateSetRegression *>(formulas[subset_knowledge->get_left_id()].get());
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
            dynamic_cast<StateSetUnion *>(formulas[subset_knowledge->get_right_id()].get());
    if((!s_cup_sp) || (s_cup_sp->get_left_id() != stateset_id)) {
        std::cerr << "Error when applying rule RI: the right side of subset knowledge #" << premise_ids[0]
                  << " is not a union of set expression #" << stateset_id
                  << " and another set expression." << std::endl;
        return false;
    }

    int sp_id = s_cup_sp->get_right_id();

    // check if k2 says that S' is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_ids[1]].get());
    if (!dead_knowledge || (dead_knowledge->get_set_id() != sp_id)) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[1]
                  << " does not state that set expression #" << sp_id << " is dead." << std::endl;
        return false;
    }

    // check if premise_ids[2] says that {I} \subseteq S_not
    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[2]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule RI: knowledge #" << premise_ids[2]
                 << " is not of type SUBSET." << std::endl;
        return false;
    }
    // check that left side of premise_ids[2] is {I}
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(formulas[subset_knowledge->get_left_id()].get());
    if((!init) || (init->get_constant_type() != ConstantType::INIT)) {
        std::cerr << "Error when applying rule RI: the left side of subset knowledge #" << premise_ids[2]
                  << " is not the constant initial set." << std::endl;
        return false;
    }
    // check that right side of premise_ids[2] is S_not
    StateSetNegation *s_not =
            dynamic_cast<StateSetNegation *>(formulas[subset_knowledge->get_right_id()].get());
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
bool ProofChecker::check_rule_ci(int conclusion_id, int premise_id) {
    assert(kbentries[premise_id] != nullptr);

    // check that premise says that {I} is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_id].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule CI: knowledge #" << premise_id
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SSVConstant *init =
            dynamic_cast<SSVConstant *>(formulas[dead_knowledge->get_set_id()].get());
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
bool ProofChecker::check_rule_cg(int conclusion_id, int premise_id) {
    assert(kbentries[premise_id] != nullptr);

    // check that premise says that S_G^\Pi is dead
    DeadKnowledge *dead_knowledge = dynamic_cast<DeadKnowledge *>(kbentries[premise_id].get());
    if (!dead_knowledge) {
        std::cerr << "Error when applying rule CG: knowledge #" << premise_id
                  << " is not of type DEAD." << std::endl;
        return false;
    }
    SSVConstant *goal =
            dynamic_cast<SSVConstant *>(formulas[dead_knowledge->get_set_id()].get());
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
bool ProofChecker::check_rule_ur(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
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
bool ProofChecker::check_rule_ul(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
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
bool ProofChecker::check_rule_ir(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
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
bool ProofChecker::check_rule_il(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
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
bool ProofChecker::check_rule_di(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
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
bool ProofChecker::check_rule_su(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

    int e0,e1,e2;
    const StateSetUnion *su = dynamic_cast<const StateSetUnion *>(get_set_expression<T>(left_id));
    if (!su) {
        std::cerr << "Error when applying rule SU: left is not a union" << std::endl;
        return false;
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = right_id;

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(kbentries[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e0 || subset_knowledge->get_right_id() != e2) {
        std::cerr << "Error when applying rule SU: knowledge #" << premise_ids[0] << " does not state (E subset E'')" << std::endl;
        return false;
    }
    subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_si(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

    int e0,e1,e2;
    const StateSetIntersection *si = dynamic_cast<const StateSetIntersection*>(get_set_expression<T>(right_id));
    if (!si) {
        std::cerr << "Error when applying rule SI: right is not an intersection" << std::endl;
        return false;
    }
    e0 = left_id;
    e1 = si->get_left_id();
    e2 = si->get_right_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(kbentries[premise_ids[0]].get());

    if (!subset_knowledge) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e0 || subset_knowledge->get_right_id() != e1) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E')" << std::endl;
        return false;
    }
    subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_st(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

    int e0,e1,e2;
    e0 = left_id;
    e2 = right_id;

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(kbentries[premise_ids[0]].get());
    if (!subset_knowledge) {
        std::cerr << "Error when applying rule ST: knowledge #" << premise_ids[0] << " is not subset knowledge" << std::endl;
        return false;
    }
    if (subset_knowledge->get_left_id() != e0) {
        std::cerr << "Error when applying rule SI: knowledge #" << premise_ids[0] << " does not state (E subset E')" << std::endl;
        return false;
    }
    e1 = subset_knowledge->get_right_id();
    subset_knowledge = dynamic_cast<SubsetKnowledge<T> *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_at(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

    int s0, s1, a0, a1;
    s1 = right_id;
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        std::cerr << "Error when applying rule AT: left side is not a progression" << std::endl;
        return false;
    }
    s0 = progression->get_stateset_id();
    a1 = progression->get_actionset_id();

    auto subset_knowledege0 = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
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

    auto subset_knowledege1 = dynamic_cast<SubsetKnowledge<ActionSet> *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_au(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

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

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
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

    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_pt(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

    int s0,s1,s2,a0;
    s2 = right_id;
    const StateSetProgression *progression = dynamic_cast<const StateSetProgression *>(get_set_expression<StateSet>(left_id));
    if (!progression) {
        std::cerr << "Error when applying rule PT: left side is not a progression" << std::endl;
        return false;
    }
    s1 = progression->get_stateset_id();
    s0 = progression->get_actionset_id();

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
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

    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_pu(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 2 &&
           kbentries[premise_ids[0]] &&
           kbentries[premise_ids[1]]);

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

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
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

    subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[1]].get());
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
bool ProofChecker::check_rule_pr(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 1 &&
           kbentries[premise_ids[0]]);

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

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
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
bool ProofChecker::check_rule_rp(int conclusion_id, int left_id, int right_id, std::vector<int> &premise_ids) {
    assert(premise_ids.size() == 1 &&
           kbentries[premise_ids[0]]);

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

    auto subset_knowledge = dynamic_cast<SubsetKnowledge<StateSet> *>(kbentries[premise_ids[0]].get());
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
bool ProofChecker::check_statement_B1(int conclusion_id, int left_id, int right_id, std::vector<int> &) {
    bool ret = false;

    try {
        std::vector<const StateSetVariable *> left;
        std::vector<const StateSetVariable *> right;
        bool gather_variables_successfull = false;

        gather_variables_successfull = get_set_expression<StateSet>(left_id)->gather_intersection_variables(formulas, left, right);
        if (!gather_variables_successfull) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(left_id)
                    + " is not a intersection of literals of the same type.";
            throw std::runtime_error(msg);
        }
        gather_variables_successfull =  get_set_expression<StateSet>(right_id)->gather_union_variables(formulas, right, left);
        if (!gather_variables_successfull) {
            std::string msg = "Error when checking statement B1: set expression #"
                    + std::to_string(right_id)
                    + " is not a union of literals of the same type.";
            throw std::runtime_error(msg);
        }
        assert(left.size() + right.size() > 0);

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
bool ProofChecker::check_statement_B2(int conclusion_id, int left_id, int right_id, std::vector<int> &) {
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
        bool gather_variables_successfull = prog_formula->gather_intersection_variables(formulas, prog, right);
        if (!right.empty() || !gather_variables_successfull) {
            std::string msg = "Error when checking statement B2: "
                              "the progression in set expression #"
                    + std::to_string(left_id)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // left_formula is empty if the left side contains only a progression
        if(left_formula) {
            gather_variables_successfull = left_formula->gather_intersection_variables(formulas, left, right);
            if(!gather_variables_successfull) {
                std::string msg = "Error when checking statement B2: "
                                  "the non-progression part in set expression #"
                        + std::to_string(left_id)
                        + " is not an intersection of set literals.";
                throw std::runtime_error(msg);
            }
        }
        gather_variables_successfull = right_formula->gather_union_variables(formulas, right, left);
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
bool ProofChecker::check_statement_B3(int conclusion_id, int left_id, int right_id, std::vector<int> &) {
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
        bool gather_variables_successfull = reg_formula->gather_intersection_variables(formulas, reg, right);
        if (!right.empty() || !gather_variables_successfull) {
            std::string msg = "Error when checking statement B3: "
                              "the regression in set expression #"
                    + std::to_string(left_id)
                    + " is not an intersection of set variables.";
            throw std::runtime_error(msg);
        }

        // left_formula is empty if the left side contains only a regression
        if(left_formula) {
            gather_variables_successfull = left_formula->gather_intersection_variables(formulas, left, right);
            if(!gather_variables_successfull) {
                std::string msg = "Error when checking statement B3: "
                                  "the non-regression part in set expression #"
                        + std::to_string(left_id)
                        + " is not an intersection of set literals.";
                throw std::runtime_error(msg);
            }
        }
        gather_variables_successfull = right_formula->gather_union_variables(formulas, right, left);
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
bool ProofChecker::check_statement_B4(int conclusion_id, int left_id, int right_id, std::vector<int> &) {
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
bool ProofChecker::check_statement_B5(int conclusion_id, int left_id, int right_id, std::vector<int> &) {
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
