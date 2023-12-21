// Neculae Andrei-Fabian
// Ilie Dumitru
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

    // Constructor
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

    // This function replaces !! and ~. If the second parameter is true then
    // both !! and ~ will be replaced. Otherwise only !! will be replaced.
    // TODO: split this function into 2 different functions.
    // TODO: after this function is split in 2 functions add a function
    //       to replace environment variables
    std::string modify_command(const std::string&, bool);

    // Handler for PIPE(|) operator. The output of the left command is the
    // input to the right command.
    // The result is 0 if the commands executed successfully. Otherwise
    // it is the first error code returned.
    int operator_pipe(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for OUTPUT(>) operator. The output of the left command is
    // written in the output file specified on the right.
    // If the file already exists then it is overwritten. Otherwise
    // we create it.
    // There might be undefined behaviour if we write to a file we are
    // reading from.
    // The result is 0 if the operation is successful. Otherwise it is
    // an error code.
    int operator_output(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for OUTPUT APPEND(>>) operator. The output of the left
    // command is written in the output file specified on the right.
    // If the file already exists then we append. Otherwise we create it.
    // There might be undefined behaviour if we write to a file we are
    // reading from.
    // The result is 0 if the operation is successful. Otherwise it is
    // an error code.
    int operator_output_append(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for INPUT(<) operator. The input of the left command is
    // the content of the file specified on the right. If the file does
    // not exist then the command is not run.
    // There might be undefined behaviour if we write to a file we are
    // reading from.
    // The result is 0 if the operation is successful. Otherwise it is
    // an error code.
    int operator_input(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for AND(&&) operator. We execute the left command. If the
    // left command is successful then we execute the right one.
    // The result is the first error return code(the left) if there are
    // any or 0 if both are successful
    int operator_and(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for OR(||) operator. We execute the left command. If the
    // left command is not successful then we execute the right one.
    // The result is 0 if any of the commands executes successfully.
    // Otherwise it is the first return value (for the left command).
    int operator_or(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for separators
    // If the separator is ;, call the function with the bool false
    // If the separator is &, call the function with the bool true
    // TODO: Check logic and refactoring
    int separator(const std::vector<std::string>&, const std::vector<std::string>&, bool a_background = false);

    // Checks if the given string is an operator
    bool is_operator(const std::string&);

    // Finds the index of the operator which splits the command
    // (the operator with the highest priority).
    // The operators and the priorities can be found in README.md
    // and in the implementation of this class.
    int current_operator(const std::vector<std::string>&);

    // Changes the file descriptors (See "man dup2" and README.md for more), splits the command according
    // to current_operator and executes the commands recursively
    int evaluate_command(const std::vector<std::string>&, int a_fd_to_close = -1, int a_fd_to_dup = -1);

    // Function to evaluate the exit and quit commands
    int evaluate_exit();

    // Function to evaluate the clear command
    int evaluate_clear();

    // Function to evaluate the cd command
    // TODO: Simplify the logic (fd, exec_idx, sudo)
    int evaluate_cd(const std::vector<std::string>&, int, int, int);

    // Function to evaluate the history command
    // TODO: Simplify the logic (fd, sudo)
    int evaluate_history(const std::vector<std::string>&, int, int);

    // Evaluates an instruction and returns the exit status.
    // !! Very Important: 0 = failure, 1 = success !!
    // TODO: Change to return error code on error and 0 on success.
    // TODO: Remove fd logic as it is handled somewhere else
    int evaluate_instr(const std::vector<std::string>&, int a_fd_to_close = -1, int a_fd_to_dup = -1);

public:
    // Given a complex command and the current path interpret the
    // command.
    void evaluate_command(const std::string&, const std::string&);

    // The class is a singleton, thus it needs a get method.
    // This is that method.
    static Interpreter& get_instance();
};

#endif // FSL_INTERPRETER_H
