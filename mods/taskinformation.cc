#include "taskinformation.h"

#include "modsstateset.h"
#include "../task.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace mods {
TaskInformation::TaskInformation(const Task &task)
    : task(task) {
}

void TaskInformation::initialize() {

    empty_set = std::unique_ptr<ModsStateSet>(new ModsStateSet({0}, {}, *this));

    std::vector<unsigned> varorder;
    std::vector<bool> model;
    for (size_t i = 0; i < task.get_goal().size(); ++i) {
        if (task.get_goal().at(i) == 1) {
            model.push_back(true);
            varorder.push_back(i);
        } else if (task.get_goal().at(i) == 0) {
            model.push_back(false);
            varorder.push_back(i);
        }
    }
    goal_set = std::unique_ptr<ModsStateSet>(
                new ModsStateSet(std::move(varorder), {model}, *this));


    varorder = VariableOrder();
    model = Model();
    for (size_t i = 0; i < task.get_initial_state().size(); ++i) {
        if (task.get_initial_state().at(i) == 1) {
            model.push_back(true);
        } else {
            assert(task.get_initial_state().at(i) == 0);
            model.push_back(false);
        }
        varorder.push_back(i);
    }
    initial_state_set = std::unique_ptr<ModsStateSet>(
                new ModsStateSet(std::move(varorder), {model}, *this));
}

const Task &TaskInformation::get_task() const {
    return task;
}

const ModsStateSet *TaskInformation::get_empty_set() const {
    return empty_set.get();
}

const ModsStateSet *TaskInformation::get_goal_set() const {
    return goal_set.get();
}

const ModsStateSet *TaskInformation::get_initial_state_set() const {
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
