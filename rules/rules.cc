#include "rules.h"

namespace rules{
DeadnessRule::FunctionMap &DeadnessRule::get_deadness_rules_map() {
    static FunctionMap deadness_rules_map = {};
    return deadness_rules_map;
}

DeadnessRule::DeadnessRule(std::string key, VerifyFunction rule_function) {
    if (get_deadness_rules_map().count(key) != 0) {
        std::cerr << "Multiple definition of deadness rule \""
                  << key << "\"" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    get_deadness_rules_map()[key] = rule_function;
}

DeadnessRule::VerifyFunction DeadnessRule::get_deadness_rule(std::string key) {
    if (get_deadness_rules_map().count(key) == 0) {
        throw std::runtime_error("Rule " + key + " is not a deadness rule.");
    }
    return get_deadness_rules_map()[key];
}


SubsetRule::FunctionMap &SubsetRule::get_subset_rules_map() {
    static FunctionMap subset_rules_map = {};
    return subset_rules_map;
}

SubsetRule::SubsetRule(std::string key, VerifyFunction rule_function) {
    if (get_subset_rules_map().count(key) != 0) {
        std::cerr << "Multiple definition of subset rule \""
                  << key << "\"" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    get_subset_rules_map()[key] = rule_function;
}

SubsetRule::VerifyFunction SubsetRule::get_subset_rule(std::string key) {
    if (get_subset_rules_map().count(key) == 0) {
        throw std::runtime_error("Rule " + key + " is not a subset rule.");
    }
    return get_subset_rules_map()[key];
}


UnsolvableRule::FunctionMap &UnsolvableRule::get_unsolvable_rules_map() {
    static FunctionMap unsolvable_rules_map = {};
    return unsolvable_rules_map;
}

UnsolvableRule::UnsolvableRule(std::string key, VerifyFunction rule_function) {
    if (get_unsolvable_rules_map().count(key) != 0) {
        std::cerr << "Multiple definition of subset rule \""
                  << key << "\"" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    get_unsolvable_rules_map()[key] = rule_function;
}

UnsolvableRule::VerifyFunction UnsolvableRule::get_unsolvable_rule(std::string key) {
    if (get_unsolvable_rules_map().count(key) == 0) {
        throw std::runtime_error("Rule " + key + " is not an unsolvable rule.");
    }
    return get_unsolvable_rules_map()[key];
}
}
