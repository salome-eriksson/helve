#include "modsformula.h"

#include "decisiontree.h"

#include <cassert>
// TODO try to get rid of map (currently used because vector<> doesn't have hash)
#include <map>
#include <math.h>

namespace mods {
std::vector<bool> ModsFormula::get_bool_vector_from_hex(std::string hex, size_t varamount) {
    static std::unordered_map<char, std::vector<bool>> hex2vec = {
        {'0',{false, false, false, false}},
        {'1',{false, false, false, true }},
        {'2',{false, false, true,  false}},
        {'3',{false, false, true , true }},
        {'4',{false, true , false, false}},
        {'5',{false, true , false, true }},
        {'6',{false, true , true,  false}},
        {'7',{false, true , true , true }},
        {'8',{true , false, false, false}},
        {'9',{true , false, false, true }},
        {'a',{true , false, true,  false}},
        {'b',{true , false, true , true }},
        {'c',{true , true , false, false}},
        {'d',{true , true , false, true }},
        {'e',{true , true , true,  false}},
        {'f',{true , true , true , true }}};

    assert(ceil(varamount/4.0) == hex.length());
    std::vector<bool> ret;
    for (int i = 0; i < hex.length()-1; ++i) {
        ret.insert(ret.end(), hex2vec[hex[i]].begin(), hex2vec[hex[i]].end());
    }
    int remaining = varamount - 4*(hex.length()-1);
    ret.insert(ret.end(), hex2vec[hex.back()].begin(),
            hex2vec[hex.back()].begin()+remaining);
    return ret;
}

ModsFormula::ModsFormula(const VariableOrder &variable_order_,
                           std::unordered_set<Model> &&models_)
    : variable_order(variable_order_), models(std::move(models_)) {
    for (const Model& model : models) {
        assert(model.size() == variable_order.size());
    }
}

ModsFormula::ModsFormula(std::vector<const ModsFormula *> elements,
                           bool conjunction) {
    assert(elements.size() >= 2);
    variable_order = elements[0]->get_variable_order();
    for (size_t i = 1; i < elements.size(); ++i) {
        assert(elements[i]->get_variable_order() == variable_order);
    }

    if (conjunction) {
        // Move the set with smallest size to beginning (less models to check).
        size_t smallest_conjunct_pos = 0;
        for (size_t i = 1; i < elements.size(); ++i) {
            if (elements[i]->models.size()
                    < elements[smallest_conjunct_pos]->models.size()) {
                smallest_conjunct_pos = i;
            }
        }
        if (smallest_conjunct_pos > 0) {
            std::swap(elements[0], elements[smallest_conjunct_pos]);
        }

        // check that each model in elements[0] is also in all other elements.
        for (const Model &model : elements[0]->models) {
            bool contained_in_all = true;
            for (size_t i = 1; i < elements.size(); ++i) {
                if(!elements[i]->models.count(model)) {
                    contained_in_all = false;
                    break;
                }
            }
            if (contained_in_all) {
                models.insert(model);
            }
        }
    } else { // disjunction
        for (size_t i = 0; i < elements.size(); ++i) {
            models.insert(elements[i]->models.begin(), elements[i]->models.end());
        }
    }
}

ModsFormula::ModsFormula(std::stringstream &input) {
    unsigned varamount= read_uint(input);
    for (unsigned i = 0; i < varamount; ++i) {
        variable_order.push_back(read_uint(input));
    }
    std::string word = read_word(input);
    if (word != ":") {
        throw std::runtime_error("Wrong MODS syntax: "
                                 "Colon after variable order missing.");
    }
    word = read_word(input);
    while (word.compare(";") != 0) {
        models.insert(get_bool_vector_from_hex(word, variable_order.size()));
        word = read_word(input);
    }
}

void ModsFormula::aggregate_by_varorder(std::vector<const ModsFormula *> &elements,
                                         std::vector<ModsFormula> &new_formulas,
                                         bool conjunction) {
    std::map<VariableOrder, std::vector<const ModsFormula *>> grouped_elements;
    for (const ModsFormula *element : elements) {
        // If the con/disjunction became trivial, keep this as only formula.
        if ((conjunction && element->is_unsatisfiable()) ||
                (!conjunction && element->is_valid())) {
            elements.clear();
            elements.push_back(element);
            return;
        // If element is the netural element of the con/disjunction, ignore it.
        } else if ((conjunction && element->is_valid()) ||
                   (!conjunction && element->is_unsatisfiable())) {
            continue;
        }
        grouped_elements[element->variable_order].push_back(element);
    }

    // Create merged ModsStateSets and update elements vector.
    elements.clear();
    elements.reserve(grouped_elements.size());
    // TODO: choose the reserve better (we MUST reserve, pointers to vec elements)
    new_formulas.reserve(grouped_elements.size()+1);
    for (auto it: grouped_elements) {
        std::vector<const ModsFormula *> group = it.second;
        if(group.size() == 1) {
            elements.push_back(group[0]);
        } else {
            new_formulas.emplace_back(group, conjunction);
            elements.push_back(&new_formulas.back());
        }
    }

    // Empty con/disjunction -> insert "true"/"false" formula.
    if (elements.empty()) {
        new_formulas.emplace_back(VariableOrder({}), std::unordered_set<Model>());
        if (conjunction) {
            new_formulas.back().models.insert(Model({}));
        }
        elements.push_back(&new_formulas.back());
    }
}

cnf::CNFFormula ModsFormula::transform_to_cnf() const {
    DecisionTree decision_tree(variable_order);
    for (const Model &model : models) {
        decision_tree.insert_model(model);
    }
    return decision_tree.get_cnf_formula();
}

bool ModsFormula::is_valid() const {
    return models.size() == (1 << variable_order.size());
}

bool ModsFormula::is_unsatisfiable() const {
    return models.empty();
}

bool ModsFormula::contains(const Model &model) const {
    return (models.count(model) > 0);
}

int ModsFormula::get_model_count() const {
    return models.size();
}

const VariableOrder &ModsFormula::get_variable_order() const {
    return variable_order;
}

std::unordered_set<Model>::const_iterator ModsFormula::get_cbegin() const {
    return models.cbegin();
}

std::unordered_set<Model>::const_iterator ModsFormula::get_cend() const {
    return models.cend();
}

void ModsFormula::dump() const {
    for (unsigned var : variable_order) {
        std::cout << var << " ";
    }std::cout << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
    for (Model model : models) {
        for (bool val : model) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}
}
