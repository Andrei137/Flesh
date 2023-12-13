// Neculae Andrei-Fabian
// Dumitru Ilie
// Buzatu Giulian

#include "HistoryManager.h"
#include "Interface.h"
#include "Interpreter.h"
#include "Tokenizer.h"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

Interpreter::Interpreter()
{
    // Register the signal handlers
    signal(SIGINT, Interpreter::handle_sig_int);
    signal(SIGQUIT, Interpreter::handle_sig_quit);
    signal(SIGTSTP, Interpreter::handle_sig_tstp);
}

// Handles Ctrl + C's signal (SIGINT)
// If the child process is still running, it is killed
// Else, the program exits
void Interpreter::handle_sig_int(int)
{
    Interpreter& interpreter{ Interpreter::get_instance() };
    int status{};
    if (waitpid(interpreter.m_child_pid, &status, WNOHANG) == 0)
    {
        kill(interpreter.m_child_pid, SIGKILL);
    }
    else
    {
        std::cout << "^C" << std::endl;
        Interface& interface{ Interface::get_instance() };
        interface.abort();
        interface.config_terminal(false);
        exit(0);
    }
}

// Handles Ctrl + \'s signal (SIGQUIT)
// The program exits no matter what
void Interpreter::handle_sig_quit(int)
{
    Interface& interface{ Interface::get_instance() };
    interface.abort();
    interface.config_terminal(false);
    interface.clear();
    exit(0);
}

// Handles Ctrl + Z's signal (SIGTSTP)
// Only works if the child process is still running
void Interpreter::handle_sig_tstp(int)
{
    Interpreter& interpreter{ Interpreter::get_instance() };
    int status{};
    if (waitpid(interpreter.m_child_pid, &status, WNOHANG) == 0)
    {
        std::cout << "\nStopped\n";
        kill(interpreter.m_child_pid, SIGKILL);
        Interface::get_instance().config_terminal(false);
    }
}

int Interpreter::operator_and(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    int result{ evaluate_command(a_left) };
    if (result == 1)
    {
        return evaluate_command(a_right);
    }
    return result;
}

int Interpreter::operator_or(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    int result{ evaluate_command(a_left) };
    if (result == 0)
    {
        return evaluate_command(a_right);
    }
    return result;
}

int Interpreter::operator_semicolon(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    evaluate_command(a_left);
    return evaluate_command(a_right);
}

bool Interpreter::is_operator(const std::string& a_operator)
{
    const std::vector<std::string> operators{ "&&", "||", ";" };
    return std::any_of(operators.begin(), operators.end(), [&a_operator](const std::string& a_current_operator)
    {
        return a_operator == a_current_operator;
    });
}

int Interpreter::evaluate_command(const std::vector<std::string>& a_tokens)
{
    if (a_tokens.size() == 0u)
    {
        return 0;
    }

    int operator_idx{};

    for (operator_idx = 0; operator_idx < static_cast<int>(a_tokens.size()); ++operator_idx)
    {
        if (this->is_operator(a_tokens[operator_idx]))
        {
            break;
        }
    }

    if (operator_idx != static_cast<int>(a_tokens.size()))
    {
        std::vector<std::string> left{}, right{};

        for (int i = 0; i < operator_idx; ++i)
        {
            left.push_back(a_tokens[i]);
        }

        for (int i = operator_idx + 1; i < static_cast<int>(a_tokens.size()); ++i)
        {
            right.push_back(a_tokens[i]);
        }
            
        if (a_tokens[operator_idx] == "&&")
        {
            return operator_and(left, right);
        }
        else if (a_tokens[operator_idx] == "||")
        {
            return operator_or(left, right);
        }
        else if (a_tokens[operator_idx] == ";")
        {
            return operator_semicolon(left, right);
        }
        return 0;
    }
    return evaluate_instr(a_tokens);
}

// Old implementation
// Will be removed after merging with the new one
/*
void Interpreter::evaluate_command(const std::string& a_command)
{
    Interface& interface{ Interface::get_instance() };

    // Obtaining the tokens
    // No further usage at the moment
    std::vector<std::string> tokens{ Tokenizer::tokenize(a_command) };
    static_cast<void>(tokens); // so that we don't get a warning

    // Mostly used for testing before solving Ctrl + C
    if (a_command == "quit" || a_command.substr(0, 4) == "exit")
    {
        // exit_d is used for Ctrl + D, so we don't want to add it to the history
        if (a_command != "exit_d")
        {
            HistoryManager::get_instance().add_instr(a_command);
        }
        interface.abort();
        interface.config_terminal(false);
        interface.clear();
        return;
    }
    if (a_command.substr(0, 7) == "history")
    {
        HistoryManager& manager{ HistoryManager::get_instance() };
        int number{ 0 };

        if (a_command.length() > 9)
        {
            // If the command is history -c, clear the history
            if (a_command[9] == 'c')
            {
                std::cout << "Successfully cleared history\n\n";
                manager.clear_history();
                manager.add_instr(a_command);
                return;
            }
            // For testing, -n outputs the count of elements
            else if (a_command[9] == 'n')
            {
                int no_elements{ manager.get_instr_count() };
                std::cout << "Number of stored commands is " << no_elements << '\n';
                manager.add_instr(a_command);
                return;
            }
            // If the command is history -number, print the last <number> commands
            number = std::stoi(a_command.substr(9, a_command.length() - 8));
        }

        // If number is 0, print all the commands
        // Else, print the last <number> commands
        int no_elements{ manager.get_instr_count() };
        if (number == 0 || number > no_elements)
        {
            number = no_elements;
        }

        for ( --number; number > -1; --number)
        {
            std::cout << *manager.get_instr(number) << '\n';
        }
    }
    else if (a_command == "clear")
    {
        interface.clear();
        interface.print_logo();
    }
    else if (a_command == "pwd" || a_command == "cd")
    {
        std::string directory{ std::filesystem::current_path().string() };
        directory = directory.substr(5, directory.length() - 5);
        directory[0] = toupper(directory[0]);
        std::cout << directory << '\n';
    }
    else if (a_command.substr(0, 2) == "cd")
    {
        std::string directory{ a_command.substr(3, a_command.length() - 3) };

        // Checks if the directory is between quotes or has the last quote missing
        if (a_command[3] == '\"')
        {
            if (a_command[a_command.length() - 1] == '\"')
            {
                directory = directory.substr(1, directory.length() - 2);
            }
            else
            {
                directory = directory.substr(1, directory.length() - 1);
            }
        }

        std::filesystem::path new_path{ std::filesystem::current_path() / directory };
        if (std::filesystem::exists(new_path))
        {
            std::filesystem::current_path(new_path);
        }
        else
        {
            std::cout << "Invalid directory\n";
        }
    }
    // If the user actually entered a command, add it to the history
    else if (a_command != "")
    {
        // Getting the arguments from the command
        // To use with execvp
        char* argv[128];
        int argc{ 0 };
        std::string temp_command{ a_command };
        char* p{ strtok(const_cast<char*>(temp_command.c_str()), " ") };
        while (p != nullptr)
        {
            argv[argc++] = p;
            // If the call is echo and p starts with a quote, remove it
            if (strcmp(argv[0], "echo") == 0 && p[0] == '\"')
            {
                p = p + 1;
                // If p ends with a quote, remove it
                if (p[strlen(p) - 1] == '\"')
                {
                    p[strlen(p) - 1] = '\0';
                }
                argv[argc - 1] = p;
            }
            p = strtok(nullptr, " ");
        }
        argv[argc] = nullptr;

        // Creating a new process to run the command
        // So that the program doesn't exit after running the command
        m_child_pid = fork();
        if (m_child_pid == -1)
        {
            perror("Error forking");
            return;
        }
        if (m_child_pid == 0)
        {
            interface.config_terminal(false);
            if (execvp(argv[0], argv) == -1)
            {
                perror("Error executing command");
                interface.abort();
                exit(1);
            }
        }
        int status{};
        waitpid(m_child_pid, &status, WUNTRACED);
        interface.config_terminal(true);
        HistoryManager::get_instance().add_instr(a_command);
    }

    // For better visibility
    if (a_command != "clear")
    {
        std::cout << '\n';
    }
}
*/

void Interpreter::evaluate_command(const std::string& a_command)
{
    std::vector<std::string> tokens{ Tokenizer::tokenize(a_command) };
    evaluate_command(tokens);
}

Interpreter& Interpreter::get_instance()
{
    static Interpreter interpreter;
    return interpreter;
}
