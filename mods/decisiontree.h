#ifndef DECISIONTREE_H
#define DECISIONTREE_H

#include "../global_funcs.h"
#include "../cnf/cnfformula.h"

#include <deque>
#include <vector>

//TODO: remove
#include <iostream>

namespace mods {
struct Node {
    Node *true_child;
    Node *false_child;
    Node(Node *true_successor = nullptr, Node *false_successor = nullptr)
        : true_child(true_successor), false_child(false_successor) {}
    Node *get_child(bool val) { return val ? true_child : false_child; }
    Node **get_child_pointer(bool val) { return val ? &true_child : &false_child; }

    void print(std::string indent = "") {
        if (true_child == nullptr) {
            std::cout << indent << "true" << std::endl;
        } else if (false_child == nullptr) {
            std::cout << indent << "false" << std::endl;
        } else {
            std::cout << indent << this << std::endl;
            false_child->print(indent + "  ");
            true_child->print(indent + "  ");
        }
    }
};

class DecisionTree
{
private:
    std::deque<Node> nodes;
    Node* root_pointer;
    Node true_node;
    Node false_node;
    const VariableOrder &variable_order;

    void add_negation_as_clause(std::vector<Clause> &clauses, const Model &model);
    void backtrack(std::vector<Node *> &nodes, Model &model);
public:
    DecisionTree(const VariableOrder &variable_order);

    void insert_model(const Model &model);
    cnf::CNFFormula get_cnf_formula();
};
}

#endif // DECISIONTREE_H
