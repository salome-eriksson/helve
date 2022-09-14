#include "proofchecker.h"

#include "global_funcs.h"

#include "rules/rules.h"

#include <cmath>
#include <iostream>

inline void check_end_of_line(std::stringstream &line) {
    if (!line.eof()) {
        std::string tmp;
        std::getline(line, tmp);
        throw std::runtime_error("Line should be finished but still contains \""
                                 + tmp + "\"");
    }
}


ProofChecker::ProofChecker(std::string &task_file)
    : task(task_file), unsolvability_proven(false) {
    manager = Cudd(task.get_number_of_facts()*2);
    manager.setTimeoutHandler(exit_timeout);
    manager.InstallOutOfMemoryHandler(exit_oom);
    manager.UnregisterOutOfMemoryCallback();
}

void ProofChecker::add_knowledge(std::unique_ptr<Knowledge> entry,
                                 KnowledgeID id) {
    if(id >= knowledgebase.size()) {
        knowledgebase.resize(id+1);
    }

    if (knowledgebase[id]) {
        throw std::runtime_error("Cannot add knowledge #" + std::to_string(id)
                                 + ", it already exists.");
    }
    knowledgebase[id] = std::move(entry);
}

// line format: <id> <type> <description>
void ProofChecker::add_state_set(std::stringstream &line) {
    SetID set_id = read_uint(line);
    std::string state_set_type = read_word(line);


    std::unique_ptr<StateSet> expression =
            StateSet::get_stateset_constructor(state_set_type)(line, task);
    check_end_of_line(line);

    if (set_id >= statesets.size()) {
        statesets.resize(set_id+1);
    }
    if (statesets[set_id]) {
        throw std::runtime_error("Cannot add state set expression #"
                                 + std::to_string(set_id)
                                 + ", it already exists.");
    }
    statesets[set_id] = std::move(expression);
}

// line format: <id> <type> <description>
void ProofChecker::add_action_set(std::stringstream &line) {
    std::unique_ptr<ActionSet> action_set;

    SetID set_id = read_uint(line);
    std::string action_set_type = read_word(line);

    if(action_set_type.compare("b") == 0) { // basic enumeration of actions
        // the first number denotes the amount of actions being enumerated
        size_t amount = read_uint(line);
        std::unordered_set<size_t> actions;
        for(size_t i = 0; i < amount; ++i) {
            actions.insert(read_uint(line));
        }
        action_set = std::unique_ptr<ActionSet>(new ActionSetBasic(actions));

    } else if(action_set_type.compare("u") == 0) { // union of action sets
        SetID left_id = read_uint(line);
        SetID right_id = read_uint(line);
        action_set = std::unique_ptr<ActionSet>(new ActionSetUnion(left_id, right_id));

    } else if(action_set_type.compare("a") == 0) { // constant (set of all actions)
        action_set = std::unique_ptr<ActionSet>(new ActionSetConstantAll(task));

    } else {
        throw std::runtime_error("Action set expression type " + action_set_type
                                 + " does not exist.");
    }
    check_end_of_line(line);

    if (set_id >= actionsets.size()) {
        actionsets.resize(set_id+1);
    }
    if (actionsets[set_id]) {
        throw std::runtime_error("Cannot add action set expression #"
                                 + std::to_string(set_id)
                                 + ", it already exists.");
    }
    actionsets[set_id] = std::move(action_set);
}

// line format: <id> <type> <description>
void ProofChecker::verify_knowledge(std::stringstream &line) {
    KnowledgeID conclusion_id;
    std::unique_ptr<Knowledge> conclusion;
    std::string knowledge_type, rule;
    bool unsolvable_rule = false;

    conclusion_id = read_uint(line);
    knowledge_type = read_word(line);

    if(knowledge_type == "s") {
        // Subset knowledge is defined by "<left_id> <right_id> <rule> {premise_ids}".
        SetID left_id, right_id;
        std::vector<KnowledgeID> premises;
        // reserve max amount of premises (currently 2)
        premises.reserve(2);

        left_id = read_uint(line);
        right_id = read_uint(line);
        rule = read_word(line);
        while (!line.eof()) {
            premises.push_back(read_uint(line));
        }
        conclusion = rules::SubsetRule::get_subset_rule(rule)(left_id, right_id, premises, *this);
    } else if(knowledge_type == "d") {
        // Dead knowledge is defined by "<dead_id> <rule> {premise_ids}".
        SetID dead_set_id;
        std::vector<KnowledgeID> premises;
        // reserve max amount of premises (currently 3)
        premises.reserve(3);

        dead_set_id = read_uint(line);
        rule = read_word(line);
        while (!line.eof()) {
            premises.push_back(read_uint(line));
        }
        conclusion = rules::DeadnessRule::get_deadness_rule(rule)(dead_set_id, premises, *this);
    } else if(knowledge_type == "u") {
        // Unsolvability knowledge is defined by "<rule> <premise_id>".
        KnowledgeID premise;

        rule = read_word(line);
        premise = read_uint(line);
        conclusion = rules::UnsolvableRule::get_unsolvable_rule(rule)(premise, *this);
        unsolvable_rule = true;
    } else {
        throw std::runtime_error("Knowledge type " + knowledge_type
                                 + " does not exist.");
    }
    check_end_of_line(line);

    // we cannot set it in the knowledge type "u" block directly in case
    // check_end_of_line throws an error
    if (unsolvable_rule) {
        unsolvability_proven = true;
    }
    add_knowledge(std::move(conclusion), conclusion_id);
}

bool ProofChecker::is_unsolvability_proven() {
    return unsolvability_proven;
}
