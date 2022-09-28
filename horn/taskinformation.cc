#include "taskinformation.h"

#include "hornstateset.h"
#include "../task.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace horn {
TaskInformation::TaskInformation(const Task &task)
    : task(task) {
}

void TaskInformation::initialize() {

    std::stringstream emptyss;
    emptyss << "p cnf " << task.get_number_of_facts() << " 1 0 ;";
    empty_set = std::unique_ptr<HornStateSet>(new HornStateSet(emptyss, task));

    const Cube &goal = task.get_goal();
    size_t goal_size = 0;
    for (int val : goal) {
        if (val != 2) {
            goal_size++;
        }
    }
    std::stringstream goalss;
    goalss << "p cnf " << task.get_number_of_facts() << " " << goal_size << " ";
    for (size_t var = 0; var < goal.size(); ++var) {
        // Shift by 1 since DIMACS starts at 1; negate if literal is negative.
        if (goal[var] == 1) {
            goalss << std::to_string(var+1) << " 0 ";
        } else if (goal[var] == 0) {
            goalss << std::to_string(-((int)var+1)) << " 0 ";
        }
    }
    goalss << ";";
    goal_set = std::unique_ptr<HornStateSet>(new HornStateSet(goalss, task));

    const Cube &init_state = task.get_initial_state();
    std::stringstream initialss;
    initialss << "p cnf " << task.get_number_of_facts() << " "
              << init_state.size() << " ";
    for (int var = 0; var < (int) init_state.size(); ++var) {
        assert(init_state[var] == 0 || init_state[var] == 1);
        // Shift by 1 since DIMACS starts at 1; negate if var not true in init.
        if(init_state[var] == 1) {
            initialss << std::to_string((var+1)) << " 0 ";
        } else if (init_state[var] == 0) {
            initialss << std::to_string(-((int)var+1)) << " 0 ";
        }
    }
    initialss << ";";
    initial_state_set = std::unique_ptr<HornStateSet>(new HornStateSet(initialss, task));
}

const Task &TaskInformation::get_task() const {
    return task;
}

const HornStateSet *TaskInformation::get_empty_set() const {
    return empty_set.get();
}

const HornStateSet *TaskInformation::get_goal_set() const {
    return goal_set.get();
}

const HornStateSet *TaskInformation::get_initial_state_set() const {
    return initial_state_set.get();
}

const TaskInformation &TaskInformation::get_task_information(const Task& task) {
    static TaskInformationMap task_information_map;
    if(task_information_map.count(&task) == 0) {
        task_information_map.insert({&task, TaskInformation(task)});
        task_information_map.at(&task).initialize();
    }
    return task_information_map.at(&task);
}
}
