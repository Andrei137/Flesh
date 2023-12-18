// Neculae Andrei-Fabian
// Dumitru Ilie
// Buzatu Giulian

#ifndef FSL_INTERPRETER_H
#define FSL_INTERPRETER_H

#include <queue>
#include <string>
#include <sys/types.h>
#include <vector>

class Interpreter
{
    // Old path, used for cd -
    std::string m_old_path;

    // Current path, used for cd -
    std::string m_curr_path;

    // If true, we will print a new line after each command
    // So the terminal will look cleaner
    bool m_beautify;

    // If we use the separator &, the commands will be executed in background
    // This means that we will not wait for the child process to finish
    bool m_is_background;

    // Process id of the child process
    // We need it for the signal handlers
    pid_t m_child_pid;

    // Queue of background processes
    std::queue<pid_t> m_background_processes;

    Interpreter();

    // The Interpreter class is a singleton, thus we need to delete the copy and
    // move constructors and assignment operators
    Interpreter(const Interpreter&) = delete;
    Interpreter(Interpreter&&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;
    Interpreter& operator=(Interpreter&&) = delete;

    // Handles Ctrl + C's signal (SIGINT)
    // This function must be static as it is a generic handler
    static void handle_sigint(int);

    // Handles Ctrl + \'s signal (SIGQUIT)
    // This function must be static as it is a generic handler
    static void handle_sigquit(int);

    // Handles Ctrl + Z's signal (SIGTSTP)
    // This function must be static as it is a generic handler
    static void handle_sigtstp(int);

    // This function replaces !! and ~
    std::string modify_command(const std::string&,bool);

    // Handler for PIPE operator
    int operator_pipe(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for OUTPUT(>) operator
    int operator_output(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for OUTPUT APPEND(>>) operator
    int operator_output_append(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for INPUT(<) operator
    int operator_input(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for AND operator
    int operator_and(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for OR operator
    int operator_or(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for separators
    // If the separator is ;, call the function with the bool false
    // If the separator is &, call the function with the bool true
    int separator(const std::vector<std::string>&, const std::vector<std::string>&, bool a_background = false);

    // Checks if the given string is an operator
    bool is_operator(const std::string&);

    // Finds the index of the operator which splits the command
    int current_operator(const std::vector<std::string>&);

    // Finds the operator according to which we split the command
    int evaluate_command(const std::vector<std::string>&, int a_fd_to_close = -1, int a_fd_to_dup = -1);

    // Function to evaluate the exit and quit commands
    int evaluate_exit();

    // Function to evaluate the clear command
    int evaluate_clear();

    // Function to evaluate the cd command
    int evaluate_cd(const std::vector<std::string>&, int, int, int);

    // Function to evaluate the history command
    int evaluate_history(const std::vector<std::string>&, int, int);

    // Evaluates an instruction and returns the exit status
    // !! Very Important: 0 = failure, 1 = success !!
    int evaluate_instr(const std::vector<std::string>&, int a_fd_to_close = -1, int a_fd_to_dup = -1);

public:
    void evaluate_command(const std::string&,const std::string&);

    static Interpreter& get_instance();
};

#endif // FSL_INTERPRETER_H
