// Neculae Andrei-Fabian
// Dumitru Ilie
// Buzatu Giulian

#ifndef FSL_INTERPRETER_H
#define FSL_INTERPRETER_H

#include <string>
#include <sys/types.h>
#include <vector>

class Interpreter
{
    pid_t m_child_pid;

    Interpreter();

    // The Interpreter class is a singleton, thus we need to delete the copy and
    // move constructors and assignment operators
    Interpreter(const Interpreter&) = delete;
    Interpreter(Interpreter&&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;
    Interpreter& operator=(Interpreter&&) = delete;

    // Handles Ctrl + C's signal (SIGINT)
    // This function must be static as it is a generic handler
    static void handle_sig_int(int);

    // Handles Ctrl + \'s signal (SIGQUIT)
    // This function must be static as it is a generic handler
    static void handle_sig_quit(int);

    // Handles Ctrl + Z's signal (SIGTSTP)
    // This function must be static as it is a generic handler
    static void handle_sig_tstp(int);

    // Handler for AND operator
    int operator_and(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for OR operator
    int operator_or(const std::vector<std::string>&, const std::vector<std::string>&);

    // Handler for SEMICOLON operator
    int operator_semicolon(const std::vector<std::string>&, const std::vector<std::string>&);

    // Finds the operator according to which we split the command
    int evaluate_command(const std::vector<std::string>&);

    // Evaluates an instruction and returns the exit status
    int evaluate_instr(const std::vector<std::string>&);

    // Checks if the given string is an operator
    bool is_operator(const std::string&);

public:
    void evaluate_command(const std::string&);

    static Interpreter& get_instance();
};

#endif // FSL_INTERPRETER_H