#ifndef RULES_H
#define RULES_H

#include "../proofchecker.h"

#include <functional>
#include <memory>

#include <iostream>

namespace rules {
using DeadnessRuleFunction =
    std::function<std::unique_ptr<Knowledge>
    (SetID, std::vector<KnowledgeID> &, const ProofChecker &)>;
using SubsetRuleFunction =
    std::function<std::unique_ptr<Knowledge>
    (SetID, SetID, std::vector<KnowledgeID> &, const ProofChecker &)>;
using UnsolvableRuleFunction =
    std::function<std::unique_ptr<Knowledge>
    (KnowledgeID, const ProofChecker &)>;

class DeadnessRule {
private:
    static std::unordered_map<std::string, DeadnessRuleFunction> deadness_rules;
public:
    DeadnessRule(std::string key, DeadnessRuleFunction rule_function);
    ~DeadnessRule() = default;
    DeadnessRule(const DeadnessRule &other) = delete;

    static DeadnessRuleFunction get_deadness_rule(std::string key);
};

class SubsetRule {
private:
    static std::unordered_map<std::string, SubsetRuleFunction> subset_rules;
public:
    SubsetRule(std::string key, SubsetRuleFunction rule_function);
    ~SubsetRule() = default;
    SubsetRule(const SubsetRule &other) = delete;
    static SubsetRuleFunction get_subset_rule(std::string key);
};

class UnsolvableRule{
private:
    static std::unordered_map<std::string, UnsolvableRuleFunction> unsolvable_rules;
public:
    UnsolvableRule(std::string key, UnsolvableRuleFunction unsolvable_function);
    ~UnsolvableRule() = default;
    UnsolvableRule(const UnsolvableRuleFunction &other) = delete;
    static UnsolvableRuleFunction get_unsolvable_rule(std::string key);
};
}

#endif // RULES_H
