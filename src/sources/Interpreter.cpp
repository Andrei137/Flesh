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

int Interpreter::operator_pipe(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // Still not done

    // We create the child from Flesh, that will execute the two commands that have the pipe operator between them
    pid_t son = fork();
    if (son < 0)
    {
        perror("Error at fork");
        return 0;
    }

    if (son == 0)
    {
        // The child will do the piping operator

        // We create an array with two file descriptors and then call the
        // pipe system call in order to assign those file descriptors accordingly
        // fd[0] - read
        // fd[1] - write
        int fd[2];

        pipe(fd);

        pid_t grandson = fork(); // The child creates another child that will execute the first command
        if (grandson < 0)
        {
            perror("Error at second fork");
            return 0;
        }

        if (grandson == 0)
        {
            // Closes the read end because we will not use it
            close(fd[0]);

            // Closes the file descriptor with id=1, which is stdout
            // and creates a copy for fd[1], and then assigns to it the id 1.
            // The old descriptor is not closed! Both may be used interchangeably
            // When we will call execvp system call, the program we start will write using fd[1]
            // dup2(fd[1], 1);

            int left_status{ evaluate_instr(a_left, 1, fd[1]) };

            // Closes the write end
            close(fd[1]);

            // We stop the grandson with an exit value equal to the opposite of Flesh implementation
            // i.e. if it was successful then we exit(0); if it failed then we exit(1)
            exit(!left_status);
        }

        // Child code

        int status{};
        waitpid(grandson, &status, WUNTRACED);
        int grandson_return{ WEXITSTATUS(status) };

        // Closes the write end because it will not use it
        close(fd[1]);

        if (grandson_return != 0)
        {
            // First command returned an error; We return that we failed

            // Closes the other end of the pipe, which should also free the memory
            close(fd[0]);

            return 0;
        }

        // Closes the file descriptor with id 0, which is stdin
        // and creates a copy for fd[0], and then assign to it the id 0.
        // The old descriptor is not closed! Both may be used interchangeably
        // When we will call system call execvp, the program we start will read with fd[0]
        // dup2(fd[0], 0);

        int right_status{ evaluate_instr(a_right, 0, fd[0]) };

        // Closes the other end of the pipe, which should also free the memory
        close(fd[0]);

        // We stop the child with an exit value equal to the opposite of Flesh implementation
        // i.e. if it was successful then we exit(0); if it failed then we exit(1)
        exit(!right_status);
    }

    // Flesh code.

    // Get exit value and return accordingly
    int status{};
    waitpid(son, &status, WUNTRACED);
    int son_return{ WEXITSTATUS(status) };

    // We return a success value equal to the opposite of son exit value
    // i.e. if it was successful then we return 1; if it failed then we return 0
    return !son_return;
}

bool Interpreter::is_operator(const std::string& a_operator)
{
    const std::vector<std::string> operators{ "&&", "||", ";", "|"};
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
        else if (a_tokens[operator_idx] == "|")
        {
            return operator_pipe(left, right);
        }
        return 0;
    }
    return evaluate_instr(a_tokens);
}

// Old implementation
// Will be removed after merging with the new one

int Interpreter::evaluate_instr(const std::vector<std::string>& a_tokens, int a_fd_to_close, int a_fd_to_dup)
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
        int argc{ static_cast<int>(a_tokens.size()) };
        char** argv{ new char*[argc + 1] };

        for (int i = 0; i < argc; ++i)
        {
            argv[i] = const_cast<char*>(a_tokens[i].c_str());
        }
        argv[argc] = nullptr;

        if (a_fd_to_close != -1)
        {
            dup2(a_fd_to_dup, a_fd_to_close);
        }

        pid_t m_child_pid = fork();
        if (m_child_pid == -1)
        {
            perror("Error forking");
            return 0;
        }
        if (m_child_pid == 0)
        {
            if (execvp(argv[0], argv) == -1)
            {
                perror("Error executing command");
                return 0;
            }
        }
        int status{};
        int result{ waitpid(m_child_pid, &status, 0) };
        delete[] argv;
        if (result == -1)
        {
            perror("Error waiting for child process");
            return 0;
        }
        if (WIFEXITED(status))
        {
            interface.config_terminal(true);
            // For better visibility
            if (command != "clear")
            {
                std::cout << '\n';
            }
            return !WEXITSTATUS(status);
        }
        return 0;
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
