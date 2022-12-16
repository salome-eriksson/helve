#ifndef MODS_TASKINFORMATION_H
#define MODS_TASKINFORMATION_H


#include "taskinformation.h"

#include <memory>
#include <unordered_map>

class Task;

namespace mods {
class ModsStateSet;

class TaskInformation
{
private:
    using TaskInformationMap = std::unordered_map<const Task *, TaskInformation>;

    const Task &task;
    std::unique_ptr<ModsStateSet> empty_set;
    std::unique_ptr<ModsStateSet> goal_set;
    std::unique_ptr<ModsStateSet> initial_state_set;

    TaskInformation(const Task &task);
    void initialize();
public:
    const Task &get_task() const;
    const ModsStateSet *get_empty_set() const;
    const ModsStateSet *get_goal_set() const;
    const ModsStateSet *get_initial_state_set() const;

    static const TaskInformation &get_task_information(const Task& task);
};
}

#endif // MODS_TASKINFORMATION_H
