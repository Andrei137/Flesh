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
        std::cout << "\n^Z\nStopped\n";
        kill(interpreter.m_child_pid, SIGKILL);
    }
}

// Modify the old_string to that we replace !! and ~
// If a_change_all is 0, we only change !!
// Else we change both
std::string Interpreter::modify_command(const std::string& a_old_command, bool a_change_all)
{
    const char* home_env{ getenv("HOME") };
    std::string home_path{ (home_env != nullptr) ? home_env : "" }; // If HOME is not set, we set it to "", so cd ~ will not work

    std::string modified_command{};
    const std::string* last_command{ HistoryManager::get_instance().get_instr(0) };

    for (int i = 0; i < static_cast<int>(a_old_command.size()); ++i)
    {
        // If we encounter an apostrophe, we continue until that apostrophe is closed
        if(a_old_command[i] == '\'')
        {
            ++i;
            while (i < static_cast<int>(a_old_command.size()))
            {
                if (a_old_command[i] == '\'')
                {
                    break;
                }
                else
                {
                    modified_command += a_old_command[i];
                }
                ++i;
            }
        }
        // If we encounter !! we replace it with the last command
        else if(a_old_command[i] == '!' && i != static_cast<int>(a_old_command.size())-1 && a_old_command[i+1] == '!')
        {
            modified_command += *last_command;
            i++;
        }
        // If we encounter ~ and before it is a space and after it is (nothing or space or /) we change it into home_path
        else if(a_change_all && a_old_command[i]=='~' && i != 0 && a_old_command[i-1] == ' '
            && (i == static_cast<int>(a_old_command.size())-1
            || (i != static_cast<int>(a_old_command.size())-1 && (a_old_command[i+1]==' ' || a_old_command[i+1]=='/'))))
        {
            modified_command += home_path;
        }
        else
        {
            modified_command += a_old_command[i];
        }
    }
    return modified_command;
 }

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

        if (pipe(fd) < 0)
        {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }

        pid_t grandson = fork(); // The child creates another child that will execute the first command
        if (grandson < 0)
        {
            perror("Error at second fork");
            exit(EXIT_FAILURE);
        }

        if (grandson == 0)
        {
            // Grandson code for the left command

            // Close the read end because we will not use it
            close(fd[0]);

            // Redirect stdout to the write end of the pipe
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            // Execute the left command
            int left_status{ evaluate_command(a_left) };

            // We stop the grandson with an exit value equal to the opposite of Flesh implementation
            // i.e. if it was successful then we exit(0); if it failed then we exit(1)
            exit(!left_status);
        }

        // Child code for the right command

        // Close the write end because we will not use it
        close(fd[1]);

        // Redirect stdin to the read end of the pipe
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        // Execute the right command
        evaluate_command(a_right);

        int status;
        waitpid(grandson, &status, WUNTRACED);
        int grandson_return{ WEXITSTATUS(status) };

        exit(grandson_return);
    }

    // Flesh code.

    // Wait for the son to complete
    int status{};
    waitpid(son, &status, WUNTRACED);
    int son_return{ WEXITSTATUS(status) };

    // Return a success value equal to the opposite of son exit value
    // i.e., if it was successful, then we return 1; if it failed, then we return 0
    return !son_return;
}

int Interpreter::operator_output(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // Opens the file in which we want to write
    int destination_file_fd{ open(a_right[0].c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR) };
    if (destination_file_fd < 0)
    {
        std::cout << "An error occurred when opening the destination file\n";
        return 0;
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
    int success_val{ evaluate_command(a_left, 1, destination_file_fd) };

    // Closing the file descriptor
    close(destination_file_fd);

    return success_val;
}

int Interpreter::operator_output_append(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // We open the destination file only to append to it
    FILE* destination_file{ fopen(a_right[0].c_str(), "a") };

    if (!destination_file)
    {
        std::cout << "An error occurred when opening the pipe file\n";
        return 0;
    }

    // If the command is "> output.txt"
    if (a_left.empty())
    {
        fclose (destination_file);
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
    int status{ evaluate_command(a_left, 1, fd) };

    fclose(destination_file);
    return status;
}

int Interpreter::operator_input(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right)
{
    // Opens the file from which we want to read
    int source_file_fd{ open(a_right[0].c_str(), O_RDONLY) };
    if (source_file_fd < 0)
    {
        std::cout << "Source file does not exist\n";
        return 0;
    }

    // Closes the file descriptor with id=0, which is stdin
    // and creates a copy for source_file_fd , and then assign to it the id=0.
    // The old descriptor is not closed! Both may be used interchangeably
    // When we will call system call execvp, the program we start will read using source_file_fd
    // dup2(source_file_fd, 0);

    // We execute the first command, which will take the input from source_file
    int success_val{ evaluate_command(a_left, 0, source_file_fd) };

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

int Interpreter::separator(const std::vector<std::string>& a_left, const std::vector<std::string>& a_right, bool a_background)
{
    this->m_is_background = a_background; // We set the background flag so we know whether to wait for the child process or not

    evaluate_command(a_left);
    evaluate_command(a_right);

    return 1; // We return 1 because the separator doesn't care about the exit value of the commands
}

// Checks if the string is an operator
bool Interpreter::is_operator(const std::string& a_operator)
{
    const std::vector<std::string> operators{ "|", ">", ">>", "<", "&&", "||", ";", "&" };
    for (const std::string& op : operators)
    {
        if (a_operator == op)
        {
            return true;
        }
    }
    return false;
}

// We find the least important operator and return its index
// The priorities are: |, >, >>, <, &&, ||, ;, &
// | has right-to-left associativity
// The rest have left-to-right associativity
// If there are no operators, we return -1
int Interpreter::current_operator(const std::vector<std::string>& a_tokens)
{
    int operator_idx{ -1 };
    int priority{ 0 };

    for (int i = 0; i < static_cast<int>(a_tokens.size()); ++i)
    {
        if (a_tokens[i] == "|")
        {
            if (priority <= 1)
            {
                priority = 1;
                operator_idx = i;
            }
        }
        else if (a_tokens[i] == ">" || a_tokens[i] == ">>" || a_tokens[i] == "<")
        {
            if (priority < 2)
            {
                priority = 2;
                operator_idx = i;
            }
        }
        else if (a_tokens[i] == "&&")
        {
            if (priority < 3)
            {
                priority = 3;
                operator_idx = i;
            }
        }
        else if (a_tokens[i] == "||")
        {
            if (priority < 4)
            {
                priority = 4;
                operator_idx = i;
            }
        }
        else if (a_tokens[i] == ";")
        {
            if (priority < 5)
            {
                priority = 5;
                operator_idx = i;
            }
        }
        else if (a_tokens[i] == "&")
        {
            if (priority < 6)
            {
                priority = 6;
                operator_idx = i;
            }
        }
    }
    return operator_idx;
}

int Interpreter::evaluate_command(const std::vector<std::string>& a_tokens, int a_fd_to_close, int a_fd_to_dup)
{
    if (a_tokens.size() == 0u || Interface::get_instance().is_aborted())
    {
        return 0;
    }

    // Find the lowest priority operator
    // So we can split the command into two parts and evaluate them separately
    int operator_idx{ current_operator(a_tokens) };

    if (operator_idx != -1)
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
            return separator(left, right);
        }
        else if (a_tokens[operator_idx] == "&")
        {
            return separator(left, right, true);
        }

        perror("Operator defined but not implemented");
        perror(a_tokens[operator_idx].c_str());
        perror("\n");
        return 0;
    }

    return evaluate_instr(a_tokens, a_fd_to_close, a_fd_to_dup);
}

int Interpreter::evaluate_exit()
{
    this->m_beautify = false;
    Interface& interface{ Interface::get_instance() };
    interface.abort();
    interface.config_terminal(false);
    interface.clear();
    return 1;
}

int Interpreter::evaluate_clear()
{
    this->m_beautify = false;
    Interface& interface{ Interface::get_instance() };
    interface.clear();
    interface.print_logo();
    return 1;
}

int Interpreter::evaluate_cd(const std::vector<std::string>& a_tokens, int a_fd, int a_exec_idx, int a_offset)
{
    std::filesystem::path new_path{};

    if (a_exec_idx + 1 == static_cast<int>(a_tokens.size())) // cd is the same as cd ~
    {
        const char* home_env{ getenv("HOME") };
        new_path = (home_env != nullptr) ? home_env : "";

        if (new_path == "") // If HOME is not set, cd ~ will not work
        {
            std::string warning{ "cd: HOME not set\n" };
            write(a_fd, warning.c_str(), warning.size());
            return 0;
        }
    }
    else
    {
        std::string command{ Tokenizer::detokenize(a_tokens) };
        std::string directory{ command.substr(3 + a_offset, command.length() - (3 + a_offset)) };

        if (directory == "-")
        {
            if (this->m_old_path.empty())  // If cd was never used before, cd - will not work
            {
                std::string warning{ "cd: OLDPWD not set\n" };
                write(a_fd, warning.c_str(), warning.size());
                return 0;
            }

            write(a_fd, this->m_old_path.c_str(), this->m_old_path.size());
            write(a_fd, "\n", 1);
            new_path = this->m_old_path;
        }
        else
        {
            // Checks if the directory is between quotes or has the last quote missing
            if (command[3 + a_offset] == '\"')
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

    if (std::filesystem::exists(new_path)) // If the directory exists, we change the current path
    {
        std::filesystem::current_path(new_path);

        // If the current path is different from the new one,
        // we update the old path and the current path
        if (this->m_curr_path != new_path.string())
        {
            this->m_old_path = this->m_curr_path;
            this->m_curr_path = std::filesystem::current_path().string();
        }
    }
    else // Else, we print an error message
    {
        std::string warning{ "Invalid directory\n" };
        write(a_fd, warning.c_str(), warning.size());
        return 0;
    }
    return 1;
}

int Interpreter::evaluate_history(const std::vector<std::string>& a_tokens, int a_fd, int a_offset)
{
    HistoryManager& manager{ HistoryManager::get_instance() };
    std::string command{ Tokenizer::detokenize(a_tokens) };
    int number{ 0 };

    // If the command isn't only history
    if (static_cast<int>(command.length()) > 9 + a_offset)
    {
        // If the command is history -c, clear the history
        if (command[9 + a_offset] == 'c')
        {
            std::string message{ "Successfully cleared history\n" };

            write(a_fd, message.c_str(), message.size());
            manager.clear_history();

            return 1;
        }
        // For testing, -n outputs the count of elements
        else if (command[9 + a_offset] == 'n')
        {
            int no_elements{ manager.get_instr_count() };
            char message[64]{ "" };

            sprintf(message, "Number of stored commands is %d\n", no_elements);
            write(a_fd, message, strlen(message));

            return 1;
        }
        // If the command is history -number, print the last <number> commands
        number = std::stoi(command.substr(9 + 5 * a_offset, command.length() - (8 + 5 * a_offset)));
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
        write(a_fd, instr, strlen(instr));
        write(a_fd, "\n", 1);
        --number;
    }
    return 1;
}

// !! Very Important: 0 = failure, 1 = success !!
int Interpreter::evaluate_instr(const std::vector<std::string>& a_tokens, int a_fd_to_close, int a_fd_to_dup)
{
    if (Interface::get_instance().is_aborted())
    {
        return 0;
    }

    int fd{ a_fd_to_dup };
    if (fd == -1)
    {
        fd = 1; // fd = 1 because it represents stdout
    }

    // If we have sudo at the beginning, the offset will be 5
    int offset{ 0 };
    int exec_idx{ 0 };
    if (a_tokens[0] == "sudo") // We find the executable index, it's either the first or second one
    {
        ++exec_idx;
        offset = 5;
    }

    // Mostly used for testing before solving Ctrl + C
    if (a_tokens[exec_idx] == "quit" || a_tokens[exec_idx].substr(0, 4) == "exit")
    {
        return evaluate_exit();
    }
    if (a_tokens[exec_idx] == "clear")
    {
        return evaluate_clear();
    }
    else if (a_tokens[exec_idx] == "cd")
    {
        return evaluate_cd(a_tokens, fd, exec_idx, offset);
    }
    else if (a_tokens[exec_idx] == "history")
    {
        return evaluate_history(a_tokens, fd, offset);
    }
    else if (!a_tokens.empty()) // If the user actually entered a command, add it to the history
    {
        // We prepare argv for the execvp system call
        // argv is an array of strings, where the first element is the executable name,
        // the last element is nullptr and the rest are the arguments
        int argc{ static_cast<int>(a_tokens.size()) };
        char** argv{ new char*[argc + 1] };

        for (int i = 0; i < argc; ++i)
        {
            argv[i] = const_cast<char*>(a_tokens[i].c_str());
        }
        argv[argc] = nullptr;

        this->m_child_pid = fork();

        // We add the child process to the queue of background processes
        this->m_background_processes.push(this->m_child_pid);

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
                exit(1);
                return 0;
            }
        }
        delete[] argv; // We don't need argv anymore, so we free the memory
        if (!this->m_is_background) // No separator &, so we wait for the child process to finish
        {
            int status{};
            int result{ waitpid(this->m_child_pid, &status, 0) };
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
        else
        {
            // We wait for all the background processes to finish
            while (waitpid(this->m_background_processes.front(), nullptr, WNOHANG) > 0)
            {
                this->m_background_processes.pop();
            }
            usleep(1000); // We wait so that the child processes have time to finish
            return 1;
        }
    }

    // User entered nothing or CTRL + D, we ignore those
    return 1;
}

void Interpreter::evaluate_command(const std::string& a_command,const std::string& a_path)
{
    // modified_command is the command where we replace both !! and ~
    std::string modified_command = this->modify_command(a_command, 1);

    if (!modified_command.empty())
    {
        this->m_beautify = true;
        this->m_is_background = false;

        // special_command is the command where we replace only !!
        std::string special_command{ this->modify_command(a_command, 0) };
        if (special_command != a_command)
        {
            // We show the user the command that will be executed
            write(0, special_command.c_str(), special_command.size());
            write(0, "\n", 1);
        }

        this->m_curr_path = a_path;
        this->m_curr_path.pop_back(); // Remove the > from the end of the path

        std::vector<std::string> tokens{ Tokenizer::tokenize(modified_command) };
        this->evaluate_command(tokens);

        HistoryManager::get_instance().add_instr(special_command);

        // If the command is not clear, we print a new line to make the terminal look cleaner
        if (this->m_beautify)
        {
            std::cout << '\n';
        }
    }
}

Interpreter& Interpreter::get_instance()
{
    static Interpreter interpreter;
    return interpreter;
}
