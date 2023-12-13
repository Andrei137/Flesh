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
    signal(SIGINT, Interpreter::handle_sigint);
    signal(SIGQUIT, Interpreter::handle_sigquit);
    signal(SIGTSTP, Interpreter::handle_sigtstp);
}

// Handles Ctrl + C's signal (SIGINT)
// If the child process is still running, it is killed
// Else, the program exits
void Interpreter::handle_sigint(int)
{
    std::cout << "^C\n";

    Interpreter& interpreter{ Interpreter::get_instance() };
    int status{};
    if (waitpid(interpreter.m_child_pid, &status, WNOHANG) == 0)
    {
        kill(interpreter.m_child_pid, SIGKILL);
    }
    else
    {
        Interface& interface{ Interface::get_instance() };
        interface.abort();
        interface.config_terminal(false);
        exit(0);
    }
}

// Handles Ctrl + \'s signal (SIGQUIT)
// The program exits no matter what
void Interpreter::handle_sigquit(int)
{
    Interface& interface{ Interface::get_instance() };
    interface.abort();
    interface.config_terminal(false);
    interface.clear();
    exit(0);
}

// Handles Ctrl + Z's signal (SIGTSTP)
// Only works if the child process is still running
void Interpreter::handle_sigtstp(int)
{
    Interpreter& interpreter{ Interpreter::get_instance() };
    int status{};
    if (waitpid(interpreter.m_child_pid, &status, WNOHANG) == 0)
    {
        std::cout << "\nStopped\n";
        kill(interpreter.m_child_pid, SIGKILL);
    }
}

int Interpreter::operator_and(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // The first command executed succesfully
    if (evaluate_command(a_left) == 1)
    {
        // So we run the second one too
        return evaluate_command(a_right);
    }
    // The first command failed
    return 0;
}

int Interpreter::operator_or(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // The first command failed
    if (evaluate_command(a_left) == 0)
    {
        // So we run the second one too
        return evaluate_command(a_right);
    }
    // The first command executed succesfully
    return 1;
}

int Interpreter::operator_semicolon(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    evaluate_command(a_left);
    return evaluate_command(a_right);
}

bool Interpreter::is_operator(const std::string& a_operator)
{
    const std::vector<std::string> operators{ "&&", "||", ";" };
    for (const std::string& op : operators)
    {
        if (a_operator == op)
        {
            return true;
        }
    }
    return false;
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
        // We build the left and the right vectors
        std::vector<std::string> left{}, right{};

        for (int i = 0; i < operator_idx; ++i)
        {
            left.push_back(a_tokens[i]);
        }

        for (int i = operator_idx + 1; i < static_cast<int>(a_tokens.size()); ++i)
        {
            right.push_back(a_tokens[i]);
        }
            
        // We run the commands with the current operator
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

int Interpreter::evaluate_instr(const std::vector<std::string>& a_tokens)
{
    Interface& interface{ Interface::get_instance() };

    // Get the whole command from the tokens
    std::string command{ "" };
    for (const std::string& token : a_tokens)
    {
        command += token + ' ';
    }
    command.pop_back();

    // If we have sudo at the beginning, the offset will be 5
    int offset{ 0 };

    // We find the executable index, it's either the first or second one
    int exec_idx{ 0 };
    if (a_tokens[0] == "sudo")
    {
        ++exec_idx;
        offset = 5;
    }

    // Mostly used for testing before solving Ctrl + C
    if (a_tokens[exec_idx] == "quit" || a_tokens[exec_idx].substr(0, 4) == "exit")
    {
        interface.abort();
        interface.config_terminal(false);
        interface.clear();
        return 1;
    }
    if (a_tokens[exec_idx] == "history")
    {
        HistoryManager& manager{ HistoryManager::get_instance() };
        int number{ 0 };

        if (static_cast<int>(command.length()) > 9 + offset)
        {
            // If the command is history -c, clear the history
            if (command[9 + offset] == 'c')
            {
                std::cout << "Successfully cleared history\n\n";
                manager.clear_history();
                return 1;
            }
            // For testing, -n outputs the count of elements
            else if (command[9 + offset] == 'n')
            {
                int no_elements{ manager.get_instr_count() };
                std::cout << "Number of stored commands is " << no_elements << '\n';
                return 1;
            }
            // If the command is history -number, print the last <number> commands
            number = std::stoi(command.substr(9 + 5 * offset, command.length() - (8 + 5 * offset)));
        }

        // If number is 0, print all the commands
        // Else, print the last <number> commands
        int no_elements{ manager.get_instr_count() };
        if (number == 0 || number > no_elements)
        {
            number = no_elements;
        }
        --number;
        while (number > -1)
        {
            std::cout << *manager.get_instr(number--) << '\n';
        }
    }
    else if (a_tokens[exec_idx] == "clear")
    {
        interface.clear();
        interface.print_logo();
    }
    else if (a_tokens[exec_idx] == "pwd" || (a_tokens[exec_idx] == "cd" && exec_idx == static_cast<int>(a_tokens.size())))
    {
        std::string directory{ std::filesystem::current_path().string() };
        directory[0] = toupper(directory[0]);
        std::cout << directory << '\n';
    }
    else if (a_tokens[exec_idx] == "cd")
    {
        std::string directory{ command.substr(3 + offset, command.length() - (3 + offset)) };

        // Checks if the directory is between quotes or has the last quote missing
        if (command[3 + offset] == '\"')
        {
            if (command[command.length() - 1] == '\"')
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
    else if (command != "")
    {
        // Getting the arguments from the command
        // To use with execvp
        char* argv[128];
        int argc{ 0 };
        std::string temp_command{ command };
        char* p{ strtok(const_cast<char*>(temp_command.c_str()), " ") };
        while (p != nullptr)
        {
            argv[argc++] = p;
            // If the call is echo and p starts with a quote, remove it
            if (strcmp(argv[exec_idx], "echo") == 0 && p[0] == '\"')
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
            return 0;
        }
        if (m_child_pid == 0)
        {
            interface.config_terminal(false);
            if (execvp(argv[0], argv) == -1)
            {
                perror("Error executing command");
                interface.abort();
                return 0;
            }
        }
        int status{};
        waitpid(m_child_pid, &status, WUNTRACED);
        interface.config_terminal(true);
    }

    // For better visibility
    if (command != "clear")
    {
        std::cout << '\n';
    }

    return 1;
}

void Interpreter::evaluate_command(const std::string& a_command)
{
    std::vector<std::string> tokens{ Tokenizer::tokenize(a_command) };
    if (a_command != "")
    {
        HistoryManager::get_instance().add_instr(a_command);
    }
    evaluate_command(tokens);
}

Interpreter& Interpreter::get_instance()
{
    static Interpreter interpreter;
    return interpreter;
}
