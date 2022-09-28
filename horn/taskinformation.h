#ifndef TASKFORMULAS_H
#define TASKFORMULAS_H

#include <memory>
#include <unordered_map>

class Task;

namespace horn {
class HornStateSet;

class TaskInformation
{
private:
    using TaskInformationMap = std::unordered_map<const Task *, TaskInformation>;

    const Task &task;
    std::unique_ptr<HornStateSet> empty_set;
    std::unique_ptr<HornStateSet> goal_set;
    std::unique_ptr<HornStateSet> initial_state_set;

    TaskInformation(const Task &task);
    void initialize();
public:
    const Task &get_task() const;
    const HornStateSet *get_empty_set() const;
    const HornStateSet *get_goal_set() const;
    const HornStateSet *get_initial_state_set() const;

    static const TaskInformation &get_task_information(const Task& task);
};
}

#endif // TASKFORMULAS_H
