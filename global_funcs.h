#ifndef GLOBAL_FUNCS_H
#define GLOBAL_FUNCS_H

#include "timer.h"

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "cuddObj.hh"

using KnowledgeID = size_t;
using SetID = size_t;
using Literal = std::pair<unsigned, bool>;
using Clause = std::unordered_map<unsigned, bool>;
using PartialAssignment = std::unordered_map<unsigned, bool>;
using Model = std::vector<bool>;
using VariableOrder = std::vector<unsigned>;

extern Timer timer;
extern int g_timeout;
extern Cudd manager;

enum class ExitCode {
    CERTIFICATE_VALID = 0,
    CRITICAL_ERROR = 1,
    CERTIFICATE_NOT_VALID = 2,
    NO_TASK_FILE= 3,
    NO_CERTIFICATE_FILE = 4,
    PARSING_ERROR = 5,
    OUT_OF_MEMORY = 6,
    TIMEOUT = 7
};

struct IntVectorHasher {
    int operator()(const std::vector<int> &V) const {
        int hash=0;
        for(int i=0;i<V.size();i++) {
            hash+=V[i]; // Can be anything
        }
        return hash;
    }
};

std::string read_word(std::stringstream &input);
int read_int(std::stringstream &input);
size_t read_uint(std::stringstream &input);
void initialize_timer();
void set_timeout(int x);
int get_peak_memory_in_kb(bool use_buffered_input = true);
void exit_with(ExitCode code);
void exit_oom(size_t size);
void exit_timeout(std::string);
void register_event_handlers();


#endif /* GLOBAL_FUNCS_H */
