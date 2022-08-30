#include "global_funcs.h"
#include "proofchecker.h"
#include "task.h"
#include "timer.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>

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
    std::cout << "Usage: verify <task-file> <certificate-file> [--timeout=x] [--continue-upon-error] [--print-line]" << std::endl;
    std::cout << "timeout is an optional parameter in seconds" << std::endl;
    exit(0);
}


int main(int argc, char** argv) {
    if(argc < 3 || argc > 6) {
        print_help_and_exit();
    }
    register_event_handlers();
    initialize_timer();
    std::string task_file = argv[1];
    expand_environment_variables(task_file);
    std::string certificate_file = argv[2];
    expand_environment_variables(certificate_file);
    int timeout = std::numeric_limits<int>::max();
    bool exit_upon_error = true;
    bool print_line = false;

    for(int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.substr(0,10).compare("--timeout=") == 0) {
            std::istringstream ss(arg.substr(10));
            if (!(ss >> timeout) || timeout < 0) {
            }
            std::cout << "using timeout of " << timeout << " seconds" << std::endl;
        } else if (arg.compare("--continue-upon-error") == 0) {
            exit_upon_error = false;
        } else if (arg.compare("--print-line") == 0) {
            print_line = true;
        } else {
            std::cout << "unknown argument: "<< arg << std::endl;
            print_help_and_exit();
        }
    }
    set_timeout(timeout);

    ProofChecker proofchecker(task_file);
    unsigned int line = 0;

    std::ifstream certstream;
    certstream.open(certificate_file);
    if(!certstream.is_open()) {
        exit_with(ExitCode::NO_CERTIFICATE_FILE);
    }
    std::string input_type;
    std::string input;
    while(certstream >> input_type) {
        line++;
        // check if timeout is reached
        if(timer() > g_timeout) {
            exit_timeout("");
        }

        // read in rest of line
        std::getline(certstream, input);
        if (print_line) {
            std::cout << "Checking line " << line << ": " << input_type
                      << " " << input << std::endl;
        }

        try {
            if(input_type.compare("e") == 0) {
                proofchecker.add_state_set(input);
            } else if(input_type.compare("k") == 0) {
                proofchecker.verify_knowledge(input);
            } else if(input_type.compare("a") == 0) {
                proofchecker.add_action_set(input);
            } else if(input_type.at(0) == '#') {
                continue;
            } else {
                throw std::runtime_error("Unknown start of line: " +
                                         input_type + ".");
            }
        // TODO: case distinction for different kinds of errors
        } catch (const std::exception &e) {
            std::cerr << "Critical error when checking line " << line << ":\n"
                      << input_type << input << "\n"
                      << e.what() << "\n";
            if (exit_upon_error) {
                exit_with(ExitCode::CRITICAL_ERROR);
            }
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
