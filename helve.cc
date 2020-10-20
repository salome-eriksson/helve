#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <limits>
#include <memory>
#include <regex>

#include "global_funcs.h"
#include "task.h"
#include "timer.h"
#include "proofchecker.h"

// TODO: check for virtual destructors!

void expand_environment_variables(std::string &file) {
    size_t found = file.find('$');
    while(found != std::string::npos) {
        size_t end = file.find('/');
        std::string envvar;
        if(end == std::string::npos) {
            envvar = file.substr(found+1);
        } else {
            envvar = file.substr(found+1,end-found-1);
        }
        // to upper case
        for(size_t i = 0; i < envvar.size(); i++) {
            envvar.at(i) = toupper(envvar.at(i));
        }
        std::string expanded = std::getenv(envvar.c_str());
        file.replace(found,envvar.length()+1,expanded);
        found = file.find('$');
    }
}

void print_help_and_exit() {
    std::cout << "Usage: verify <task-file> <certificate-file> [--timeout=x]" << std::endl;
    std::cout << "timeout is an optional parameter in seconds" << std::endl;
    exit(0);
}


int main(int argc, char** argv) {
    if(argc < 3 || argc > 4) {
        print_help_and_exit();
    }
    register_event_handlers();
    initialize_timer();
    std::string task_file = argv[1];
    expand_environment_variables(task_file);
    std::string certificate_file = argv[2];
    expand_environment_variables(certificate_file);

    int timeout = std::numeric_limits<int>::max();
    for(int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.substr(0,10).compare("--timeout=") == 0) {
            std::istringstream ss(arg.substr(10));
            if (!(ss >> timeout) || timeout < 0) {
            }
            std::cout << "using timeout of " << timeout << " seconds" << std::endl;
        } else {
            print_help_and_exit();
        }
    }
    set_timeout(timeout);

    // TODO: should task be a global variable? (would save references)
    ProofChecker proofchecker(task_file);

    std::ifstream certstream;
    certstream.open(certificate_file);
    if(!certstream.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }
    std::string input_type;
    std::string input;
    while(certstream >> input_type) {
        // check if timeout is reached
        if(timer() > g_timeout) {
            exit_timeout("");
        }

        // read in rest of line
        std::getline(certstream, input);

        if(input_type.compare("e") == 0) {
            proofchecker.add_state_set(input);
        } else if(input_type.compare("k") == 0) {
            proofchecker.verify_knowledge(input);
        } else if(input_type.compare("a") == 0) {
            proofchecker.add_action_set(input);
        } else if(input_type.at(0) == '#') {
            std::getline(certstream, input);
        } else {
            std::cerr << "unknown start of line: " << input_type << std::endl;
            exit_with(ExitCode::CRITICAL_ERROR);
        }
    }

    std::cout << "Verify total time: " << timer() << std::endl;
    std::cout << "Verify memory: " << get_peak_memory_in_kb() << "KB" << std::endl;
    if(proofchecker.is_unsolvability_proven()) {
        std::cout << "unsolvability proven" << std::endl;
        exit_with(ExitCode::CERTIFICATE_VALID);
    } else {
        std::cout << "unsolvability NOT proven" << std::endl;
        exit_with(ExitCode::CERTIFICATE_NOT_VALID);
    }
}
