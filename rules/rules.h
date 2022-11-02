#ifndef RULES_H
#define RULES_H

#include "../proofchecker.h"

#include <functional>
#include <memory>

#include <iostream>

namespace rules {
class DeadnessRule {
public:
    using VerifyFunction = std::function<std::unique_ptr<Knowledge>
        (SetID, std::vector<KnowledgeID> &, const ProofChecker &)>;
private:
    using FunctionMap = std::unordered_map<std::string, VerifyFunction>;
    static FunctionMap &get_deadness_rules_map();
public:
    DeadnessRule(std::string key, VerifyFunction rule_function);
    ~DeadnessRule() = default;
    DeadnessRule(const DeadnessRule &other) = delete;

    static VerifyFunction get_deadness_rule(std::string key);
};

class BoundRule {
public:
    using VerifyFunction = std::function<std::unique_ptr<Knowledge>
        (SetID, unsigned, std::vector<KnowledgeID> &, const ProofChecker &)>;
private:
    using FunctionMap = std::unordered_map<std::string, VerifyFunction>;
    static FunctionMap &get_bound_rules_map();
public:
    BoundRule(std::string key, VerifyFunction rule_function);
    ~BoundRule() = default;
    BoundRule(const BoundRule &other) = delete;

    static VerifyFunction get_bound_rule(std::string key);
};

class SubsetRule {
public:
    using VerifyFunction = std::function<std::unique_ptr<Knowledge>
        (SetID, SetID, std::vector<KnowledgeID> &, const ProofChecker &)>;
private:
    using FunctionMap = std::unordered_map<std::string, VerifyFunction>;
    static FunctionMap &get_subset_rules_map();
public:
    SubsetRule(std::string key, VerifyFunction rule_function);
    ~SubsetRule() = default;
    SubsetRule(const SubsetRule &other) = delete;
    static VerifyFunction get_subset_rule(std::string key);
};

class UnsolvableRule{
public:
    using VerifyFunction = std::function<std::unique_ptr<Knowledge>
        (KnowledgeID, const ProofChecker &)>;
private:
    using FunctionMap = std::unordered_map<std::string, VerifyFunction>;
    static FunctionMap &get_unsolvable_rules_map();
public:
    UnsolvableRule(std::string key, VerifyFunction unsolvable_function);
    ~UnsolvableRule() = default;
    UnsolvableRule(const VerifyFunction &other) = delete;
    static VerifyFunction get_unsolvable_rule(std::string key);
};

class OptimalityRule {
public:
    using VerifyFunction = std::function<std::unique_ptr<Knowledge>
        (unsigned, KnowledgeID, const ProofChecker &)>;
private:
    using FunctionMap = std::unordered_map<std::string, VerifyFunction>;
    static FunctionMap &get_optimality_rules_map();
public:
    OptimalityRule(std::string key, VerifyFunction optimality_function);
    ~OptimalityRule() = default;
    OptimalityRule(const VerifyFunction &other) = delete;
    static VerifyFunction get_optimality_rule(std::string key);
};
}

#endif // RULES_H
