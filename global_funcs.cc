#include <stdlib.h>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <iostream>
#include <limits>
#include <string.h>
#include <fcntl.h>
#include <cassert>

#include "global_funcs.h"

Timer timer;
int g_timeout;
Cudd manager;

void initialize_timer() {
    timer = Timer();
}

void set_timeout(int x) {
    g_timeout = x;
}

int get_peak_memory_in_kb(bool use_buffered_input) {
    // On error, produces a warning on cerr and returns -1.
    int memory_in_kb = -1;

    std::ifstream procfile;
    if (!use_buffered_input) {
        procfile.rdbuf()->pubsetbuf(0, 0);
    }
    procfile.open("/proc/self/status");
    std::string word;
    while (procfile.good()) {
        procfile >> word;
        if (word == "VmPeak:") {
            procfile >> memory_in_kb;
            break;
        }
        // Skip to end of line.
        procfile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    if (procfile.fail())
        memory_in_kb = -1;
    if (memory_in_kb == -1)
        std::cerr << "warning: could not determine peak memory" << std::endl;
    return memory_in_kb;
}




// Everything below here is about exit handling
void write_reentrant(int filedescr, const char *message, int len) {
    while (len > 0) {
        int written;
        do {
            written = write(filedescr, message, len);
        } while (written == -1 && errno == EINTR);
        /*
          We could check for other values of errno here but all errors except
          EINTR are catastrophic enough to abort, so we do not need the
          distinction.
        */
        if (written == -1)
            abort();
        message += written;
        len -= written;
    }
}

void write_reentrant_str(int filedescr, const char *message) {
    write_reentrant(filedescr, message, strlen(message));
}

void write_reentrant_char(int filedescr, char c) {
    write_reentrant(filedescr, &c, 1);
}

void write_reentrant_int(int filedescr, int value) {
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d", value);
    if (len < 0)
        abort();
    write_reentrant(filedescr, buffer, len);
}

bool read_char_reentrant(int filedescr, char *c) {
    int result;
    do {
        result = read(filedescr, c, 1);
    } while (result == -1 && errno == EINTR);
    /*
      We could check for other values of errno here but all errors except
      EINTR are catastrophic enough to abort, so we do not need the
      distinction.
    */
    if (result == -1)
        abort();
    return result == 1;
}


void print_peak_memory_reentrant() {
    int proc_file_descr = TEMP_FAILURE_RETRY(open("/proc/self/status", O_RDONLY));
    if (proc_file_descr == -1) {
        write_reentrant_str(
            STDERR_FILENO,
            "critical error: could not open /proc/self/status\n");
        abort();
    }

    const char magic[] = "\nVmPeak:";
    char c;
    size_t pos_magic = 0;
    const size_t len_magic = sizeof(magic) - 1;

    // Find magic word.
    while (pos_magic != len_magic && read_char_reentrant(proc_file_descr, &c)) {
        if (c == magic[pos_magic]) {
            ++pos_magic;
        } else {
            pos_magic = 0;
        }
    }

    if (pos_magic != len_magic) {
        write_reentrant_str(
            STDERR_FILENO,
            "critical error: could not find VmPeak in /proc/self/status\n");
        abort();
    }

    write_reentrant_str(STDOUT_FILENO, "abort memory ");

    // Skip over whitespace.
    while (read_char_reentrant(proc_file_descr, &c) && isspace(c))
        ;

    do {
        write_reentrant_char(STDOUT_FILENO, c);
    } while (read_char_reentrant(proc_file_descr, &c) && !isspace(c));

    write_reentrant_str(STDOUT_FILENO, " KB\n");
    /*
      Ignore potential errors other than EINTR (there is nothing we can do
      about I/O errors or bad file descriptors here).
    */
    TEMP_FAILURE_RETRY(close(proc_file_descr));
}

void exit_with(ExitCode code) {
    switch(code) {
    case ExitCode::CERTIFICATE_VALID:
        write_reentrant_str(1,"Exiting: certificate is valid\n");
        break;
    case ExitCode::CERTIFICATE_NOT_VALID:
        write_reentrant_str(1,"Exiting: certificate is not valid\n");
        break;
    case ExitCode::CRITICAL_ERROR:
        write_reentrant_str(1,"Exiting: unexplained critical error\n");
        break;
    case ExitCode::PARSING_ERROR:
        write_reentrant_str(1,"Exiting: parsing error\n");
        break;
    case ExitCode::NO_CERTIFICATE_FILE:
        write_reentrant_str(1,"Exiting: no certificate file found\n");
        break;
    case ExitCode::NO_TASK_FILE:
        write_reentrant_str(1,"Exiting: no task file found\n");
        break;
    case ExitCode::OUT_OF_MEMORY:
        write_reentrant_str(1,"Exiting: memory limit reached\n");
        break;
    case ExitCode::TIMEOUT:
        write_reentrant_str(1,"Exiting: timeout reached\n");
        break;
    default:
        write_reentrant_str(1,"Exiting: default exitcode\n");
        break;
    }
    exit(static_cast<int>(code));
}

void exit_oom(size_t) {
    print_peak_memory_reentrant();
    write_reentrant_str(1,"abort time ");
    int time = timer() *100;
    write_reentrant_int(1,time/100);
    time -= (time/100)*100;
    write_reentrant_str(1, ".");
    write_reentrant_int(1,time/10);
    time -= (time/10)*10;
    write_reentrant_int(1,time);
    write_reentrant_str(1,"\n");
    exit_with(ExitCode::OUT_OF_MEMORY);
}

void exit_timeout(std::string) {
    std::cout << "abort memory: " << get_peak_memory_in_kb() << "KB" << std::endl;
    std::cout << "abort time: " << timer << std::endl;
    exit_with(ExitCode::TIMEOUT);
}

void signal_handler(int signal_number) {
    std::cout << "signal_handler" << std::endl;
    std::cout << "abort memory " << get_peak_memory_in_kb() << "KB" << std::endl;
    std::cout << "abort time " << timer << std::endl;
    std::cout << "caught signal " << signal_number << std::endl;
    if(signal_number == 24) {
        exit_with(ExitCode::TIMEOUT);
    } else {
        exit_with(ExitCode::CRITICAL_ERROR);
    }
}

void out_of_memory_handler() {
    exit_oom(0);
}

void register_event_handlers() {
    // Terminate when running out of memory.
    std::set_new_handler(out_of_memory_handler);

    struct sigaction default_signal_action;
    default_signal_action.sa_handler = signal_handler;
    // Block all signals we handle while one of them is handled.
    sigemptyset(&default_signal_action.sa_mask);
    sigaddset(&default_signal_action.sa_mask, SIGABRT);
    sigaddset(&default_signal_action.sa_mask, SIGTERM);
    sigaddset(&default_signal_action.sa_mask, SIGSEGV);
    sigaddset(&default_signal_action.sa_mask, SIGINT);
    sigaddset(&default_signal_action.sa_mask, SIGXCPU);
    // Reset handler to default action after completion.
    default_signal_action.sa_flags = SA_RESETHAND;

    sigaction(SIGABRT, &default_signal_action, 0);
    sigaction(SIGTERM, &default_signal_action, 0);
    sigaction(SIGSEGV, &default_signal_action, 0);
    sigaction(SIGINT, &default_signal_action, 0);
    sigaction(SIGXCPU, &default_signal_action, 0);
}
