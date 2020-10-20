#include "task.h"
#include "global_funcs.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>


Task::Task(std::string taskfile) {

    std::ifstream in;
    in.open(taskfile.c_str());
    if(!in.is_open()) {
        exit_with(ExitCode::NO_TASK_FILE);
    }
    std::string line;
    std::vector<std::string> linevec;
    std::unordered_map<std::string, int> fact_map;

    //parse number of atoms
    std::getline(in, line);
    split(line, linevec, ':');
    assert(linevec.size() == 2);
    if(linevec[0].compare("begin_atoms") != 0) {
        print_parsing_error_and_exit(line, "begin_atoms:<numatoms>");
    }
    int factamount = stoi(linevec[1]);
    fact_names.resize(factamount);
    //parse atoms
    for(size_t i = 0; i < factamount; ++i) {
        std::getline(in, fact_names[i]);
        fact_map.insert({fact_names[i], i});
    }
    std::getline(in, line);
    if(line.compare("end_atoms") != 0) {
        print_parsing_error_and_exit(line, "end_atoms");
    }

    //parse initial state
    std::getline(in, line);
    if(line.compare("begin_init") != 0) {
        print_parsing_error_and_exit(line, "begin_init");
    }
    //initial state definition: all named variables are true, all others are false
    initial_state.resize(factamount, 0);
    std::getline(in, line);
    while(line.compare("end_init") != 0) {
        int index = stoi(line);
        initial_state[index] = 1;
        std::getline(in, line);
    }

    //parse goal
    std::getline(in, line);
    if(line.compare("begin_goal") != 0) {
        print_parsing_error_and_exit(line, "begin_goal");
    }
    //goal definition: all names variables are true, all others are "don't care"
    std::getline(in, line);
    goal.resize(factamount, 2);
    while(line.compare("end_goal") != 0) {
        int index = stoi(line);
        goal[index] = 1;
        std::getline(in, line);
    }

    //parsing actions
    std::getline(in, line);
    split(line, linevec, ':');
    assert(linevec.size() == 2);
    if(linevec[0].compare("begin_actions") != 0) {
        print_parsing_error_and_exit(line, "begin_actions:<numactions>");
    }
    int actionamount = stoi(linevec[1]);
    actions.resize(actionamount);
    for(int i = 0; i < actionamount; ++i) {
        getline(in, line);
        if(line.compare("begin_action") != 0) {
            print_parsing_error_and_exit(line, "begin_action");
        }
        //parse name
        getline(in, line);
        actions[i].name = line;
        //parse cost
        getline(in, line);
        split(line, linevec, ':');
        if(linevec[0].compare("cost") != 0) {
            print_parsing_error_and_exit(line, "cost:<actioncost>");
        }
        actions[i].cost = stoi(linevec[1]);

        actions[i].change.resize(factamount, 0);
        getline(in, line);
        while(line.compare("end_action")!= 0) {
            split(line, linevec, ':');
            assert(linevec.size() == 2);
            int fact = stoi(linevec[1]);
            if(linevec[0].compare("PRE") == 0) {
                actions[i].pre.push_back(fact);
            } else if(linevec[0].compare("ADD") == 0) {
                actions[i].change[fact] = 1;
            } else if(linevec[0].compare("DEL") == 0) {
                actions[i].change[fact] = -1;
            } else {
                print_parsing_error_and_exit(line, "PRE/ADD/DEL");
            }
            getline(in, line);
        }
    }
    getline(in, line);
    if(line.compare("end_actions") != 0) {
        print_parsing_error_and_exit(line, "end_actions");
    }
    assert(!getline(in, line));
    in.close();
}

void Task::split(const std::string &s, std::vector<std::string> &vec, char delim) {
    vec.clear();
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        vec.push_back(item);
    }
}

void Task::print_parsing_error_and_exit(std::string &line, std::string expected) {
    std::cout << "unexpected line in certificate: " << line
              << " (expected \"" << expected << "\")" << std::endl;
    exit_with(ExitCode::PARSING_ERROR);
}

const std::string& Task::get_fact(int n) const {
    return fact_names[n];
}

const Action& Task::get_action(int n) const {
  return actions[n];
}

int Task::get_number_of_actions() const {
  return actions.size();
}

int Task::get_number_of_facts() const {
  return fact_names.size();
}

const Cube& Task::get_initial_state() const {
  return initial_state;
}

const std::vector<int>& Task::get_goal() const {
  return goal;
}

void Task::dump_action(int n) const {
   const Action& a = actions.at(n);
   std::cout << "Name: " << a.name << ", cost: " << a.cost << std::endl;
   std::cout << "Preconditions: ";
   for(int i = 0; i < a.pre.size(); ++i) {
       std::cout << fact_names.at(a.pre.at(i)) << ", ";
   }std::cout << std::endl;
   for(int i = 0; i < a.change.size(); ++i) {
       if(a.change.at(i) !=0) {
           if(a.change.at(i) < 0) {
               std::cout << "delete " << fact_names.at(i) << std::endl;
           } else {
               std::cout << "add " << fact_names.at(i) << std::endl;
           }
       } else {
           std::cout << "no change to" << fact_names.at(i) << std::endl;
       }
   }
}
