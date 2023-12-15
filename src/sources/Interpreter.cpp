// Neculae Andrei-Fabian
// Dumitru Ilie
// Buzatu Giulian

#include "HistoryManager.h"
#include "Interface.h"
#include "Interpreter.h"
#include "Tokenizer.h"
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

Interpreter::Interpreter() : m_old_path()
{
    // Register the signal handlers
    signal(SIGINT , Interpreter::handle_sigint);
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

// std::string Interpreter::modify_command(const std::string& a_old_command, bool a_change_all)
// {
//     // Modifying the initial command to replace !! and ~
//     // If a_change_all is 0, we only change !!
//     // Else we change both
//     const char* home_env = getenv("HOME");
//     std::string home_path = (home_env != nullptr) ? home_env : "";

//     std::string modified_command, last_command { *HistoryManager::get_instance().get_instr(0) };
//     for(int i = 0; i < static_cast<int>(a_old_command.size()); ++i)
//     {
//         // If we encounter an apostrophe, we continue until that apostrophe is closed
//         if(a_old_command[i] == '\'')
//         {
//             ++i;
//             while (i < static_cast<int>(a_old_command.size()))
//             {
//                 if (a_old_command[i] == '\'')
//                 {
//                     break;
//                 }
//                 else
//                 {
//                     modified_command += a_old_command[i];
//                 }
//                 ++i;
//             }
//         }
//         // If we encounter !! we replace it with the last command
//         else if(a_old_command[i] == '!' && i != static_cast<int>(a_old_command.size())-1 && a_old_command[i+1] == '!')
//         {
//             modified_command += last_command;
//             i++;
//         }
//         // If we encounter ~ and before it is a space and after it is (nothing or space or /) we change it into home_path
//         else if(a_change_all && a_old_command[i]=='~' && i != 0 && a_old_command[i-1] == ' '
//             && (i == static_cast<int>(a_old_command.size())-1
//             || (i != static_cast<int>(a_old_command.size())-1 && (a_old_command[i+1]==' ' || a_old_command[i+1]=='/'))))
//         {
//             modified_command += home_path;
//         }
//         else
//         {
//             modified_command += a_old_command[i];
//         }
//     }
//     return modified_command;
// }

int Interpreter::operator_pipe(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
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

int Interpreter::operator_output(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // Opens the file in which we want to write
    int destination_file_fd = open(a_right[0].c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (destination_file_fd < 0)
    {
        std::cout << "An error occurred when opening the destination file\n";
        return errno;
    }

    // If the command is "> output.txt"
    if (a_left.empty())
    {
        return 1;
    }

    // Closes the file descriptor with id=1, which is stdout
    // and creates a copy for destination_file_fd, and then assign to it id=1
    // The old descriptor is not closed! Both may be used interchangeably
    // When we will call system call execvp, the program we start will write using destination_file_fd
    // dup2(destination_file_fd, 1);

    // We execute the first command, which will write to the destination_file
    int success_val{ evaluate_instr(a_left, 1, destination_file_fd) };

    // Closing the file descriptor
    close(destination_file_fd);

    return success_val;
}

int Interpreter::operator_output_append(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // We open the destination file only to append to it
    FILE *destination_file=fopen(a_right[0].c_str(),"a");
    if(!destination_file)
    {
        std::cout<<"An error occurred when opening the pipe file\n";
        return errno;
    }

    // If the command is "> output.txt"
    if (a_left.empty())
    {
        fclose(destination_file);
        return 1;
    }

    // File descriptor for the destination file
    int fd{ fileno(destination_file) };

    // Closes the file descriptor with id=1, which is stdout
    // and creates a copy for fd, and then assign to it id=1
    // The old descriptor is not closed! Both may be used interchangeably
    // When we will call system call execvp, the program we start will write using fd
    // dup2(fd, 1);

    // We execute the first command, which will append to the destination_file
    int status{ evaluate_instr(a_left, 1, fd) };

    fclose(destination_file);
    return status;
}

int Interpreter::operator_input(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // Opens the file from which we want to read
    int source_file_fd = open(a_right[0].c_str(), O_RDONLY);
    if (source_file_fd < 0)
    {
        std::cout << "Source file does not exist\n";
        return errno;
    }

    // Closes the file descriptor with id=0, which is stdin
    // and creates a copy for source_file_fd , and then assign to it the id=0.
    // The old descriptor is not closed! Both may be used interchangeably
    // When we will call system call execvp, the program we start will read using source_file_fd
    // dup2(source_file_fd, 0);

    // We execute the first command, which will take the input from source_file
    int success_val{ evaluate_instr(a_left, 0, source_file_fd) };

    // Closing the file descriptor
    close(source_file_fd);

    return success_val;
}

int Interpreter::operator_and(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // The first command executed successfully
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
    // The first command executed successfully
    return 1;
}

int Interpreter::operator_semicolon(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    evaluate_command(a_left);
    evaluate_command(a_right);
    return 1; // We return 1 because the semicolon operator doesn't care about the exit value of the commands
}

// Checks if the string is an operator
bool Interpreter::is_operator(const std::string& a_operator)
{
    const std::vector<std::string> operators{ "&&", "||", ";", "|", ">", ">>","<" };
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

    // Find the first operator
    // We will change this later considering the priority of the operators
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
        if (a_tokens[operator_idx] == "|")
        {
            return operator_pipe(left, right);
        }
        else if (a_tokens[operator_idx] == ">")
        {
            return operator_output(left, right);
        }
        else if (a_tokens[operator_idx] == ">>")
        {
            return operator_output_append(left, right);
        }
        else if (a_tokens[operator_idx] == "<")
        {
            return operator_input(left, right);
        }
        else if (a_tokens[operator_idx] == "&&")
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

    int fd{ a_fd_to_dup };
    if (fd == -1)
    {
        fd = 1; // fd=1 because it represents stdout
    }

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
                if (a_fd_to_dup == -1)
                {
                    // fd=1 because it represents stdout
                    write(1, "Successfully cleared history\n\n", 30);
                }
                else
                {
                    write(a_fd_to_dup, "Successfully cleared history\n\n", 30);
                }
                manager.clear_history();
                return 1;
            }
            // For testing, -n outputs the count of elements
            else if (command[9 + offset] == 'n')
            {
                int no_elements{ manager.get_instr_count() };

                char message[64] = "";
                sprintf(message, "Number of stored commands is %d\n", no_elements);
                write(fd, message, strlen(message));

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
            const char* instr{ manager.get_instr(number)->c_str() };
            write(fd, instr, strlen(instr));
            write(fd, "\n", 1);
            number--;
        }
    }
    else if (a_tokens[exec_idx] == "clear")
    {
        interface.clear();
        interface.print_logo();
    }
    else if (a_tokens[exec_idx] == "pwd")
    {
        std::string directory{ std::filesystem::current_path().string() };
        directory[0] = toupper(directory[0]);

        write(fd, directory.c_str(), directory.size());
        write(fd, "\n", 1);
    }
    else if (a_tokens[exec_idx] == "cd")
    {
        std::filesystem::path new_path{ };

        if (exec_idx + 1 == static_cast<int>(a_tokens.size())) // because cd to work as cd ~
        {
            const char* home_env = getenv("HOME");
            new_path = (home_env != nullptr) ? home_env : "";

            if (new_path == "")
            {
                write(fd, "cd: HOME not set\n", 17);
                return 0;
            }
        }
        else
        {
            std::string directory{ command.substr(3 + offset, command.length() - (3 + offset)) };

            if (directory == "-")
            {
                if(this->m_old_path.empty())
                {
                    std::string warning = "cd: OLDPWD not set";
                    write(fd, warning.c_str(), warning.size());
                    write(fd, "\n", 1);
                    return 0;
                }

                write(fd, this->m_old_path.c_str(), this->m_old_path.size());
                write(fd, "\n", 1);

                new_path = this->m_old_path;
            }
            else
            {
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
                new_path = std::filesystem::current_path() / directory;
            }
        }

        if (std::filesystem::exists(new_path))
        {
            std::filesystem::current_path(new_path);

            if (this->m_curr_path != new_path.generic_string())
            {
                this->m_old_path = this->m_curr_path;
                this->m_curr_path = std::filesystem::current_path().generic_string();
            }
        }
        else
        {
            write(fd, "Invalid directory\n", 18);
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

        this->m_child_pid = fork();
        if (this->m_child_pid == -1)
        {
            perror("Error forking");
            return 0;
        }
        if (this->m_child_pid == 0)
        {
            if (a_fd_to_close != -1)
            {
                dup2(a_fd_to_dup, a_fd_to_close);
            }

            if (execvp(argv[0], argv) == -1)
            {
                perror("Error executing command");
                return 0;
            }
        }
        int status{};
        int result{ waitpid(this->m_child_pid, &status, 0) };
        delete[] argv;
        if (result == -1)
        {
            perror("Error waiting for child process");
            return 0;
        }
        if (WIFEXITED(status))
        {
            return !WEXITSTATUS(status);
        }
        return 0;
    }
    return 1;
}

void Interpreter::evaluate_command(const std::string& a_command,const std::string& a_path)
{
    // modified_command is the command where we replace both !! and ~
    // std::string modified_command = this->modify_command(a_command,1);
    std::vector<std::string> tokens{ Tokenizer::tokenize(a_command) };
    if (!a_command.empty())
    {
        // speical_command is the command where we replace only !!
        // std::string special_command = this->modify_command(a_command,0);
        // if(special_command!=a_command)
        // {
        //     write(0, special_command.c_str(), special_command.size());
        //     write(0, "\n", 1);
        // }
        HistoryManager::get_instance().add_instr(a_command);
    }
    this->m_curr_path = a_path;
    this->m_curr_path.pop_back();
    this->evaluate_command(tokens);
}

Interpreter& Interpreter::get_instance()
{
    static Interpreter interpreter;
    return interpreter;
}
