#ifndef PROOFCHECKER_H
#define PROOFCHECKER_H

#include "actionset.h"
#include "knowledge.h"
#include "stateset.h"
#include "task.h"

#include <deque>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class ProofChecker
{
private:
    Task task;
    std::deque<std::unique_ptr<StateSet>> statesets;
    std::deque<std::unique_ptr<ActionSet>> actionsets;
    std::deque<std::unique_ptr<Knowledge>> knowledgebase;
    bool unsolvability_proven;

    void add_knowledge(std::unique_ptr<Knowledge> entry, KnowledgeID id);
public:
    ProofChecker(std::string &task_file);

    void add_state_set(std::stringstream &line);
    void add_action_set(std::stringstream &line);
    void verify_knowledge(std::stringstream &line);
    bool is_unsolvability_proven();

    template<class T, typename std::enable_if<std::is_base_of<ActionSet, T>::value>::type * = nullptr>
    const T *get_set(SetID set_id) const {
        if (set_id < 0 || set_id >= actionsets.size() || !actionsets[set_id]) {
            throw std::runtime_error("Action set expression #" +
                                     std::to_string(set_id) +
                                     " does not exist.");
        }
        const T* ret= dynamic_cast<const T *>(actionsets[set_id].get());
        if (!ret) {
            throw std::runtime_error("Action set expression #" +
                                     std::to_string(set_id) +
                                     " is not of type " + typeid(T).name());
        }
        return ret;
    }

    template<class T, typename std::enable_if<std::is_base_of<StateSet, T>::value>::type * = nullptr>
    const T *get_set(SetID set_id) const {
        if (set_id < 0 || set_id >= statesets.size() || !statesets[set_id]) {
            throw std::runtime_error("State set expression #" +
                                     std::to_string(set_id) +
                                     " does not exist.");
        }
        const T* ret= dynamic_cast<const T *>(statesets[set_id].get());
        if (!ret) {
            throw std::runtime_error("State set expression #" +
                                     std::to_string(set_id) +
                                     " is not of type " + typeid(T).name());
        }
        return ret;
    }

    template<class T, typename std::enable_if<std::is_base_of<Knowledge, T>::value>::type * = nullptr>
    const T *get_knowledge(KnowledgeID knowledge_id) const {
        if (knowledge_id < 0 || knowledge_id >= knowledgebase.size()
                || !knowledgebase[knowledge_id]) {
            throw std::runtime_error("Knowledge #" + std::to_string(knowledge_id) +
                                     " does not exist.");
        }
        const T* ret= dynamic_cast<const T *>(knowledgebase[knowledge_id].get());
        if (!ret) {
            throw std::runtime_error("Knowledge #" + std::to_string(knowledge_id) +
                                     " is not of type " + typeid(T).name());
        }
        return ret;
    }
};

#endif // PROOFCHECKER_H
