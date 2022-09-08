#include "../rules.h"

#include "../../knowledge.h"
#include "../../setcompositions.h"

namespace rules {
// DIstributivity: Without premises,
// ((E \cup E') \cap E'') \subseteq ((E \cap E'') \cup (E' \cap E''))
template<class T>
std::unique_ptr<Knowledge> distributivity(SetID left_id, SetID right_id,
                                          std::vector<KnowledgeID> & premise_ids,
                                          const ProofChecker &proof_checker) {
    if (!premise_ids.empty()) {
        throw std::runtime_error("Premise list is not empty.");
    }

    SetID e0,e1,e2;

    // Get sets E, E' and E'' from the left side.
    const T *tmp = proof_checker.get_set<T>(left_id);
    const SetIntersection *si = dynamic_cast<const SetIntersection *>(tmp);
    if (!si) {
        throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                 + " is not an intersection.");

    }

    tmp = proof_checker.get_set<T>(si->get_left_id());
    const SetUnion *su = dynamic_cast<const SetUnion *>(tmp);
    if (!su) {
        throw std::runtime_error("Set expression #" + std::to_string(left_id)
                                 + " is not an intersection with a"
                                 + " union on the left side.");
    }
    e0 = su->get_left_id();
    e1 = su->get_right_id();
    e2 = si->get_right_id();

    // Check if the right side matches the left.
    tmp = proof_checker.get_set<T>(right_id);
    su = dynamic_cast<const SetUnion *>(tmp);
    if (!su) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not a union.");
    }

    tmp = proof_checker.get_set<T>(su->get_left_id());
    si = dynamic_cast<const SetIntersection *>(tmp);
    if (!si) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not a union with an"
                                 " intersection on the left side");
    }

    if (si->get_left_id() != e0 || si->get_right_id() != e2) {
        throw std::runtime_error("Left side of set expression #"
                                 + std::to_string(right_id)
                                 + " does not match the sets in set expression #"
                                 + std::to_string(left_id) + ".");
    }

    tmp = proof_checker.get_set<T>(su->get_right_id());
    si = dynamic_cast<const SetIntersection *>(tmp);
    if (!si) {
        throw std::runtime_error("Set expression #" + std::to_string(right_id)
                                 + " is not a union with an"
                                 " intersection on the right side.");
    }

    if (si->get_left_id() != e1 || si->get_right_id() != e2) {
        throw std::runtime_error("Right side of set expression #"
                                 + std::to_string(right_id)
                                 + " does not match the sets in set expression #"
                                 + std::to_string(left_id) + ".");
    }

    return std::unique_ptr<Knowledge>(new SubsetKnowledge<T>(left_id,right_id));
}

SubsetRule _dis_plugin("dis", distributivity<StateSet>);
SubsetRule _dia_plugin("dia", distributivity<ActionSet>);
}
