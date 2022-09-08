#include "rules.h"

namespace rules{
std::unordered_map<std::string, DeadnessRuleFunction> DeadnessRule::deadness_rules;
std::unordered_map<std::string, SubsetRuleFunction> SubsetRule::subset_rules;
std::unordered_map<std::string, UnsolvableRuleFunction> UnsolvableRule::unsolvable_rules;

DeadnessRule::DeadnessRule(std::string key, DeadnessRuleFunction rule_function) {
    if (deadness_rules.count(key) != 0) {
        std::cerr << "Multiple definition of deadness rule \""
                  << key << "\"" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    deadness_rules[key] = rule_function;
}

DeadnessRuleFunction DeadnessRule::get_deadness_rule(std::string key) {
    if (deadness_rules.count(key) == 0) {
        throw std::runtime_error("Rule " + key + " is not a deadness rule.");
    }
    return deadness_rules[key];
}

SubsetRule::SubsetRule(std::string key, SubsetRuleFunction rule_function) {
    if (subset_rules.count(key) != 0) {
        std::cerr << "Multiple definition of subset rule \""
                  << key << "\"" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    subset_rules[key] = rule_function;
}

SubsetRuleFunction SubsetRule::get_subset_rule(std::string key) {
    if (subset_rules.count(key) == 0) {
        throw std::runtime_error("Rule " + key + " is not a subset rule.");
    }
    return subset_rules[key];
}

UnsolvableRule::UnsolvableRule(std::string key, UnsolvableRuleFunction rule_function) {
    if (unsolvable_rules.count(key) != 0) {
        std::cerr << "Multiple definition of subset rule \""
                  << key << "\"" << std::endl;
        exit_with(ExitCode::CRITICAL_ERROR);
    }
    unsolvable_rules[key] = rule_function;
}

UnsolvableRuleFunction UnsolvableRule::get_unsolvable_rule(std::string key) {
    if (unsolvable_rules.count(key) == 0) {
        throw std::runtime_error("Rule " + key + " is not an unsolvable rule.");
    }
    return unsolvable_rules[key];
}


}
