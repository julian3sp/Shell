#include <iostream>
#include <map>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>

using namespace std;

const int MAX_INPUT_LENGTH = 128;
const int MAX_ARGS = 32;

string prompt = "==>";  // Default prompt

struct JobInfo {
    string command;
    timespec start_time;
};

map<int, JobInfo> background_jobs; // Track background jobs

void print_rusage(const struct rusage &usage, const struct timespec &start, const struct timespec &end) {
    // Calculate elapsed time in milliseconds
    long elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;

    // Calculate CPU times in milliseconds
    long user_cpu_ms = usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000;
    long system_cpu_ms = usage.ru_stime.tv_sec * 1000 + usage.ru_stime.tv_usec / 1000;

    cout << "User CPU time used: " << user_cpu_ms << " ms" << endl;
    cout << "System CPU time used: " << system_cpu_ms << " ms" << endl;
    cout << "Elapsed wall-clock time: " << elapsed_ms << " ms" << endl;
    cout << "Number of involuntary context switches: " << usage.ru_nivcsw << endl;
    cout << "Number of voluntary context switches: " << usage.ru_nvcsw << endl;
    cout << "Number of major page faults: " << usage.ru_majflt << endl;
    cout << "Number of minor page faults: " << usage.ru_minflt << endl;
    cout << "Maximum resident set size: " << usage.ru_maxrss << " KB" << endl;
}

void execute_command(const vector<string>& args, bool in_background, timespec& start_time) {
    struct rusage usage;
    int pid;
    vector<char*> exec_argv;

    if ((pid = fork()) < 0) {
        cerr << "Fork error" << endl;
        exit(1);
    }

    if (pid == 0) { // Child process
        string fullPath = "/bin/" + args[0];
        char* path = new char[fullPath.length() + 1];
        strcpy(path, fullPath.c_str());
        exec_argv.push_back(path);

        for (int i = 1; i < args.size(); i++) {
            char* arg = new char[args[i].length() + 1];
            strcpy(arg, args[i].c_str());
            exec_argv.push_back(arg);
        }
        exec_argv.push_back(nullptr); // null-terminate the argument list

        execve(exec_argv[0], exec_argv.data(), environ);
        perror("execve error"); // command not found/ my code cant execute it
        exit(1);
    } else { // Parent process
        if (in_background) {
            background_jobs[pid] = {args[0], start_time}; // Store command and start time
            cout << "[" << pid << "] " << pid << endl;
        } else {
            int status;
            waitpid(pid, &status, 0);
            struct timespec end_time;
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            getrusage(RUSAGE_CHILDREN, &usage);
            print_rusage(usage, start_time, end_time);
        }
    }

    // Clean dynamically allocated memory
    for (char* arg : exec_argv) {
        delete[] arg;
    }
}

void check_background_jobs() {
    struct rusage usage;
    struct timespec end_time;
    // we must loop over each job in the map and check if said job is done
    for (auto currentJob = background_jobs.begin(); currentJob != background_jobs.end(); ) {
        int status;
        int pid = waitpid(currentJob->first, &status, WNOHANG);
        //WNOHANG tells waitpid to return immediately if no child process has finished yet
        //preventing your program from stopping and waiting for that child process to finish

        if (pid > 0) {
            //job completed (returned to parent)
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            getrusage(RUSAGE_CHILDREN, &usage);
            cout << "[" << pid << "] " << currentJob->second.command << " Completed" << endl;
            print_rusage(usage, currentJob->second.start_time, end_time);

            currentJob = background_jobs.erase(currentJob); // Remove completed job
        } else {
            //if this job is stil running, move onto the next one
            currentJob++;
        }
    }
}

void run_shell() {
    char input_line[MAX_INPUT_LENGTH];
    struct timespec start_time;

    while (true) {
        cout << prompt;

        if (!cin.getline(input_line, MAX_INPUT_LENGTH)) {
            if (cin.eof()) {
                cout << "EOF detected. Exiting..." << endl;
            } else {
                cerr << "Input error" << endl;
            }
            break;
        }

        vector<string> args;
        bool in_background = false;

        // Parse command line into tokens
        char* token = strtok(input_line, " ");
        while (token != nullptr) {
            if (args.size() < MAX_ARGS) {
                args.push_back(token);
            } else {
                cerr << "Too many arguments" << endl;
                break;
            }
            token = strtok(nullptr, " ");
        }

        //handle new job input
        if (!args.empty() && args.back() == "&") {
            in_background = true;
            args.pop_back(); // Remove the '&' from args
        }

        if (args.empty()) {
            continue;
        }

        check_background_jobs();
        //check if backround job has finished before executing current command

        if (args[0] == "exit") {
            // Wait for all background jobs to complete before exiting
            while (!background_jobs.empty()) {
                check_background_jobs();
            }
            break;
        } else if (args[0] == "jobs") {
            //handle jobs command
            for (const auto& [pid, job] : background_jobs) {
                cout << "[" << pid << "] " << job.command << endl;
            }
        } else if (args[0] == "cd") {
            //handle cd
            if (args.size() > 1) {
                //check if working dir has been changed
                if (chdir(args[1].c_str()) != 0) {
                    perror("cd failed");
                }
            } else {
                cerr << "cd command requires an argument" << endl;
            }
        //handle set prompt
        } else if (args[0] == "set" && args[1] == "prompt" && args.size() > 2 ) {
            prompt = args[3];
        } else {
            clock_gettime(CLOCK_MONOTONIC, &start_time);
            execute_command(args, in_background, start_time);
        }
        //my code uses a lot of if/else if statements to handle different commands
        //I assume if I tried commands that I have not accounted for id get soem execve errors
    }
}

int main(int argc, char** argv) {
    if (argc > 1) {
        // Execute initial arguments
        vector<string> initial_args;
        for (int i = 1; i < argc; ++i) {
            initial_args.push_back(argv[i]);
        }

        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        execute_command(initial_args, false, start_time);
    }

    // Run shell mode
    run_shell();

    return 0;
}
