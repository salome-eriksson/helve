#include "decisiontree.h"

#include <cassert>

// TODO: remove
#include <iostream>
#include<unistd.h>

namespace mods {

DecisionTree::DecisionTree(const VariableOrder &variable_order)
    : variable_order(variable_order), true_node(Node(nullptr, &true_node)),
      false_node(Node(&false_node, nullptr)) {
    root_pointer = &false_node;
}

void DecisionTree::add_negation_as_clause(std::vector<Clause> &clauses,
                                          const Model &model) {
    clauses.push_back(Clause());
    Clause &clause = clauses.back();
    for (size_t i = 0; i < model.size(); ++i) {
        clause[variable_order[i]] = !(model[i]);
    }
}

void DecisionTree::backtrack(std::vector<Node *> &nodes, Model &model) {
    while(model.back()) {
        model.pop_back();
        nodes.pop_back();
        if (model.empty()) {
            // empty nodes is the signal for the travers while loop to stop.
            nodes.pop_back();
            return;
        }
    }
    model.back() = true;
    nodes.back() = nodes[nodes.size()-2]->true_child;
}

void DecisionTree::insert_model(const Model &model) {
    assert(model.size() == variable_order.size());

    Node **current_node = &root_pointer;
    Node **insert_point = &root_pointer;
    for (size_t i = 0; i < model.size(); ++i) {
        if (*current_node == &false_node) {
            nodes.push_back(Node(&false_node, &false_node));
            *current_node = &nodes.back();
        }

        bool other_child_equals_true =
                (*current_node)->get_child(!model[i]) == &true_node;
        current_node = (*current_node)->get_child_pointer(model[i]);
        // if from now all other children are true, simplify the subtree to true
        if (!other_child_equals_true) {
            insert_point = current_node;
        }
    }
    *insert_point = &true_node;
}

cnf::CNFFormula DecisionTree::get_cnf_formula() {
    std::vector<Node *>nodes(1,root_pointer);
    nodes.reserve(variable_order.size()+1);
    Model model(0);
    model.reserve(variable_order.size());

    std::vector<Clause> clauses;
    while (!nodes.empty()) {
        if(nodes.back() == &false_node) {
            add_negation_as_clause(clauses, model);
            backtrack(nodes, model);
        } else if (nodes.back() == &true_node) {
            backtrack(nodes, model);
        } else {
            model.push_back(false);
            nodes.push_back(nodes.back()->false_child);
        }
    }
    return cnf::CNFFormula(std::move(clauses));
}
}
