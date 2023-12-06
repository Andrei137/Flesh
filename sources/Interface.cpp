// Ilie Dumitru (some refactoring)
// Neculae Andrei-Fabian (TODOs)

#include "HistoryManager.h"
#include "Interface.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

// OLD
// \033[H moves the cursor to the top left corner of the screen
// \033[J clears the screen from the cursor to the end of the screen

// NEW
// \033c clears the screen and moves the cursor to the top left corner of the screen
#define clear() std::cout << "\033c"

#define CTRL_D 4
#define ESCAPE 27
#define ONE 49
#define FIVE 53
#define SEMICOLON 59
#define UP_ARROW 65
#define DOWN_ARROW 66
#define RIGHT_ARROW 67
#define LEFT_ARROW 68
#define RIGHT_SQUARE_BRACKET 91
#define BACKSPACE 127

Interface::Interface() : m_aborted(false) 
{
    // Get an instance of HistoryManager to initialize the location of the history file
    HistoryManager::getInstance();
}

int Interface::configTerminal(bool a_change) const
{
    struct termios ttystate;
    memset(&ttystate, 0, sizeof(ttystate));

    // Get the current terminal state
    if (tcgetattr(STDIN_FILENO, &ttystate) == -1) 
    {
        perror("Error getting terminal attributes");
        return errno;
    }

    if (a_change)
    {
        // Disable canonical mode so that input characters are immediately available
        // Disable echo so that characters such as the arrow keys aren't printed
        // Output has to be handled manually
        ttystate.c_lflag &= ~(ICANON | ECHO);
    }
    else
    {
        // Enable canonical mode and echo back
        ttystate.c_lflag |= (ICANON | ECHO);
    }

    // Apply the new settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ttystate) == -1) 
    {
        perror("Error setting terminal attributes");
        return errno;
    }
    
    return 0;
}

void Interface::refreshDisplay(int a_cursorPosition) const
{
    // Move to the beginning of the line, printing the path and the command again
    std::cout << "\r" << this->m_path << this->m_command << "\033[K";

    // Move cursor to the correct position
    std::cout << "\033[" << a_cursorPosition + this->m_path.size() + 1 << "G";

    // Ensure that the output is printed immediately
    std::cout.flush();
}

void Interface::handleArrowKeys(char a_arrowKey, int& a_cursorPosition, int& a_historyPosition)
{
    if (a_arrowKey == UP_ARROW) // Up Arrow pressed, move to the next command if possible
    {
        const std::string* command = HistoryManager::getInstance().getInstr(a_historyPosition + 1);
        
        if (command != nullptr)
        {
            // We didn't hit the history limit so just get that command
            this->m_command = *command;
            ++a_historyPosition;
        }
        a_cursorPosition = static_cast<int>(this->m_command.size());
        refreshDisplay(a_cursorPosition);
    }
    else if (a_arrowKey == DOWN_ARROW) // Down Arrow pressed, move to the previous command if possible
    {
        if (a_historyPosition > 0)
        {
            --a_historyPosition;
            this->m_command = *HistoryManager::getInstance().getInstr(a_historyPosition);
        }
        else
        {
            a_historyPosition = -1;
            this->m_command = "";
        }
        a_cursorPosition = static_cast<int>(this->m_command.size());
        refreshDisplay(a_cursorPosition);
    }
    else if (a_arrowKey == RIGHT_ARROW) // Right Arrow pressed, move cursor to the right if possible
    {
        if (a_cursorPosition < static_cast<int>(this->m_command.size())) 
        {
            ++a_cursorPosition;
            refreshDisplay(a_cursorPosition);
        }
    }
    else  // Left Arrow pressed, move cursor to the left if possible
    {
        if (a_cursorPosition > 0) 
        {
            --a_cursorPosition;
            refreshDisplay(a_cursorPosition);
        }
    }
}

// This function costed me 3 hours of my life (which felt like 3 days)
// At least it works now
void Interface::handleCtrlArrowKeys(char a_arrowKey, int& a_cursorPosition)
{
    // Remove all trailing spaces from the command
    while (this->m_command[this->m_command.length() - 1] == ' ')
    {
        this->m_command.pop_back();
    }
    if (a_arrowKey == LEFT_ARROW) // Ctrl + Left Arrow pressed, move cursor to the beginning of the current word
    {
        if (a_cursorPosition != 0)
        {
            // Add a dummy space at the beginning of the command and later remove it
            this->m_command.insert(0, 1, ' ');

            // Find the previous space before the cursor
            size_t lastSpace = this->m_command.rfind(' ', a_cursorPosition - 1);
            this->m_command.erase(0, 1);

            // If there is no previous space, set the cursor to the beginning of the command
            // Else, set the cursor before the previous space
            a_cursorPosition = static_cast<int>(lastSpace);
            refreshDisplay(a_cursorPosition);
        }
    }
    else // Ctrl + Right Arrow pressed, move cursor to the end of the current word
    {
        if (a_cursorPosition != static_cast<int>(this->m_command.length()))
        {
            // Add a dummy space at the end of the command and later remove it
            this->m_command.insert(this->m_command.length(), 1, ' ');

            // Find the next space after the cursor
            size_t nextSpace = this->m_command.find(' ', a_cursorPosition + 1);
            this->m_command.erase(this->m_command.length() - 1, 1);

            // If there is no next space, set the cursor to the end of the command
            // Else, set the cursor after the next space
            if (nextSpace == this->m_command.length())
            {
                --nextSpace;
            }
            a_cursorPosition = static_cast<int>(nextSpace) + 1;
            refreshDisplay(a_cursorPosition);
        }
    }
}

// Simulates backspace (as it was disabled by configTerminal)
void Interface::handleBackspace(int& a_cursorPosition) 
{
    if (a_cursorPosition > 0) 
    {
        this->m_command.erase(a_cursorPosition - 1, 1);
        --a_cursorPosition;
        refreshDisplay(a_cursorPosition);
    }
}

// Handles Ctrl + C's signal (SIGINT)
// If the child process is still running, it is killed
// Else, the program exits
void Interface::handleSigInt(int)
{
    int status;
    pid_t result = waitpid(Interface::getInstance().m_child_pid, &status, WNOHANG);

    if (result == 0)
    {
        kill(Interface::getInstance().m_child_pid, SIGKILL);
    }
    else
    {
        std::cout << "^C\n";
        exit(0);
    }
}

// Handles Ctrl + \'s signal (SIGQUIT)
// The program exits no matter what
void Interface::handleSigQuit(int)
{
    exit(0);
}

// Handles Ctrl + Z's signal (SIGTSTP)
void Interface::handleSigTstp(int)
{
    int status;
    pid_t result = waitpid(Interface::getInstance().m_child_pid, &status, WNOHANG);

    if (result == 0)
    {
        std::cout << "\nStopped       " << Interface::getInstance().m_command << '\n';
        kill(Interface::getInstance().m_child_pid, SIGKILL);
    }
}

// Mom can we have Cooked Porkchop?
// We have Cooked Porkchop at home
// Cooked Porkchop at home:
void Interface::printLogo()
{
    clear();
    std::cout << "                .--*#+        \n";
    std::cout << "            .==+#*#@@%        \n";
    std::cout << "          =#%%%%#=-=*@@%      \n";
    std::cout << "        ..-@@-.-+%@@@*+@%..+%*\n";
    std::cout << "        :@#-:=#@@%+-=*#%#=+@@*\n";
    std::cout << "      -##*=-+@@+-+**+::+#%@%%=\n";
    std::cout << "    .==::=#@@*==*%%*=::%@@@*  \n";
    std::cout << "  .===-.-%%+-.-@@+=+#@@@@@    \n";
    std::cout << ".-=++:-%@%*-=*#*+=++#@@#+##=  \n";
    std::cout << ".++::@@*+=*@@@*:-%@*+*#*##%+  \n";
    std::cout << ".-==*@%-:=++=-+%@##@@@@@@#    \n";
    std::cout << "=@@@@==#%=:-*@@@@@@##%%#      \n";
    std::cout << "  -#@@@::#@@@%%@@%%%%*        \n";
    std::cout << "  .=-::*@@@@%%%#              \n";
    std::cout << "    @@@@%#                    \n";
    std::cout << "                              \n";
}

// Returns the command when the user presses enter
// Until then, the command can be modified in any way
std::string Interface::getCommand()
{
    int cursorPosition{ 0 };
    int historyPosition{ -1 };
    bool changed{ false };
    char cChanged{ '\n' };
    this->m_command = "";

    configTerminal(true);
    refreshDisplay(cursorPosition);

    while (true)
    {
        // Changed is used to check if the user pressed tab when autocomplete was available and then pressed something else
        // It implies the user wants to use the current autocomplete option and probably pressed /, enter or space
        // To avoid reading the same character twice, we use cChanged to remember it
        // We only read a new character if changed is false, otherwise we use the one we already have
        char c{ cChanged };
        if (!changed)
        {
            c = getchar();
        }
        if (c == CTRL_D)
        {
            // The user pressed CTRL + D
            // We need to exit the program
            this->m_aborted = true;
            configTerminal(false);
            clear();
            return "null_command";
        }
        // Check for escape sequence
        if (c == ESCAPE)
        {
            c = getchar();
            // Check for the '[' character
            if (c == RIGHT_SQUARE_BRACKET) 
            {
                c = getchar();
                // Check for arrow keys
                if (c == UP_ARROW || c == DOWN_ARROW || c == RIGHT_ARROW || c == LEFT_ARROW)
                {
                    handleArrowKeys(c, cursorPosition, historyPosition);
                }
                // Check For Ctrl + Left / Right Arrow Key
                // For some reason, the key combination is read as 1;5C / 1;5D
                // My sanity is slowly fading away
                else if (c == ONE)
                {
                    c = getchar();
                    if (c == SEMICOLON)
                    {
                        c = getchar();
                        if (c == FIVE)
                        {
                            c = getchar();
                            if (c == RIGHT_ARROW || c == LEFT_ARROW)
                            {
                                // Look mom, I finally did it!
                                // It only took me 3 hours and 2 mental breakdowns
                                handleCtrlArrowKeys(c, cursorPosition);
                            }
                        }
                    }
                }
            }
        }
        // Check for backspace
        else if (c == BACKSPACE)
        {
            handleBackspace(cursorPosition);
        }
        // Check for tab
        else if (c == '\t')
        {
            // Get the current word from the command
            m_command.insert(0, 1, ' ');
            int lastSpace = m_command.rfind(' ', cursorPosition - 1) - 1;
            m_command.erase(0, 1);
            std::string path = m_command.substr(lastSpace + 1, cursorPosition - lastSpace - 1);
            
            // If it has / in it, it is most likely a path or the command has cd in it
            if (path.find('/') != std::string::npos)
            {
                // Get the information from the path
                // The directory is everything before the last /
                // The fileName is everything after the last /, most likely a part of the file name
                std::string directory = path.substr(0, path.find_last_of('/'));
                std::string fileName = path.substr(path.find_last_of('/') + 1, path.length() - path.find_last_of('/'));
                
                // Get all the files from the directory
                std::vector<std::string> files;
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    files.push_back(entry.path().filename().string());
                }

                // Find all the files that start with fileName
                std::vector<std::string> matches;
                for (const auto& file : files)
                {
                    if (file.substr(0, fileName.length()) == fileName)
                    {
                        // If the command has cd in it, only add directories
                        // We don't know where cd is
                        if (m_command.find("cd") != std::string::npos)
                        {
                            if (std::filesystem::is_directory(directory + '/' + file))
                            {
                                matches.push_back(file);
                            }
                        }
                        else
                        {
                            matches.push_back(file);
                        }
                    }
                }

                // If there is only one match, add it to the command
                if (matches.size() == 1)
                {
                    m_command.insert(cursorPosition, matches[0].substr(fileName.length(), matches[0].length() - fileName.length()));
                    cursorPosition += matches[0].length() - fileName.length();
                    refreshDisplay(cursorPosition);
                }
                // If there are more matches, add them to the command one by one
                else if (matches.size() > 1)
                {
                    // Remember the default values so that we can get back to them if the user presses something else than tab
                    std::string defaultCommand{ m_command };
                    int defaultCursorPosition{ cursorPosition };

                    for (const auto& match : matches)
                    {
                        // Get the old values back
                        m_command = defaultCommand;
                        cursorPosition = defaultCursorPosition;

                        // Change them to the new ones
                        m_command.insert(cursorPosition, match.substr(fileName.length(), match.length() - fileName.length()));
                        cursorPosition += match.length() - fileName.length();
                        refreshDisplay(cursorPosition);

                        c = getchar();
                        // If the user presses tab, continue to the next match
                        if (c == '\t' || c == ' ')
                        {
                            continue;
                        }
                        // If the user pressed enter or any other character, it means that they want to use the current match
                        else
                        {
                            changed = true;
                            cChanged = c;
                            break;
                        }
                    }

                    // If the user didn't keep a value, get back to the default ones
                    if (!changed)
                    {
                        m_command = defaultCommand;
                        cursorPosition = defaultCursorPosition;
                        refreshDisplay(cursorPosition);
                    }
                }
            }
        
        }
        // Finalize the command when the user presses enter
        else if (c == '\n')
        {
            std::cout << '\n';
            if (this->m_command == "")
            {
                // The user pressed enter without typing anything
                return "null_command";
            }
            // Remove trailing spaces
            while (this->m_command[this->m_command.length() - 1] == ' ')
            {
                this->m_command.pop_back();
            }
            return this->m_command;
        }
        // Any other character is added to the command after the cursor
        else
        {
            if (changed)
            {
                changed = false;
            }
            this->m_command.insert(cursorPosition, 1, c);
            ++cursorPosition;
            refreshDisplay(cursorPosition);
        }
    }

    // Something went terribly wrong, return an empty string
    return "";
}

void Interface::evaluateCommand()
{
    // Quit exits the program, but doesn't close the shell (unlike exit)
    // Mostly used for testing before solving Ctrl + C
    if (this->m_command == "quit" || this->m_command == "exit")
    {
        this->m_aborted = true;
        HistoryManager::getInstance().addInstr(this->m_command);
        configTerminal(false);
        clear();
        return;
    }
    if (this->m_command.substr(0, 7) == "history")
    {
        int number{ 0 };

        if (this->m_command.length() > 9)
        {
            // If the command is history -c, clear the history
            if (this->m_command[9] == 'c')
            {
                HistoryManager::getInstance().clearHistory();
                std::cout << "Successfully cleared history\n\n";
                HistoryManager::getInstance().addInstr(this->m_command);
                return;
            }

            // If the command is history -number, print the last <number> commands
            number = std::stoi(this->m_command.substr(9, this->m_command.length() - 8));
        }

        // If number is 0, print all the commands
        // Else, print the last <number> commands
        if (number == 0)
        {
            number = -1;
        }
        HistoryManager& manager = HistoryManager::getInstance();
        const std::string* instr = manager.getInstr(0);
        
        for (int i = 1; number && instr; ++i, --number)
        {
            std::cout << *instr << '\n';
            instr = manager.getInstr(i);
        }
    }
    else if (this->m_command == "clear")
    {
        clear();
        printLogo();
    }
    else if (this->m_command == "pwd" || this->m_command == "cd")
    {
        std::cout << std::filesystem::current_path() << '\n';
    }
    else if (this->m_command.substr(0, 2) == "cd")
    {
        std::string directory = this->m_command.substr(3, this->m_command.length() - 3);

        // Checks if the directory is between quotes or has the last quote missing
        if (this->m_command[3] == '\"')
        {
            if (this->m_command[this->m_command.length() - 1] == '\"')
            {
                directory = directory.substr(1, directory.length() - 2);
            }
            else
            {
                directory = directory.substr(1, directory.length() - 1);
            }
        }

        std::filesystem::path newPath{ std::filesystem::current_path() / directory };
        if (std::filesystem::exists(newPath))
        {
            std::filesystem::current_path(newPath);
        }
        else
        {
            std::cout << "Invalid directory\n";
        }
    }
    else
    {
        // Getting the arguments from the command
        // To use with execvp
        char* argv[128];
        int argc{ 0 };
        std::string tempCommand{ this->m_command };
        char* p{ strtok(const_cast<char*>(tempCommand.c_str()), " ") };
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
            configTerminal(false);
            if (execvp(argv[0], argv) == -1)
            {
                perror("Error executing command");
                exit(1);
            }
        }
        wait(nullptr);
        configTerminal(true);
    }

    // If the user actually entered a command, add it to the history
    if (this->m_command != "null_command")
    {
        HistoryManager::getInstance().addInstr(this->m_command);
    }

    // For better visibility
    if (this->m_command != "clear")
    {
        std::cout << '\n';
    }
}

void Interface::run()
{
    signal(SIGINT, handleSigInt);
    signal(SIGQUIT, handleSigQuit);
    signal(SIGTSTP, handleSigTstp);

    printLogo();
    
    while (!this->m_aborted)
    {
        this->m_path = std::filesystem::current_path().string() + ">";
        this->m_command = getCommand();
        evaluateCommand();
    }
}

Interface& Interface::getInstance()
{
    static Interface myInterface{};
    return myInterface;
}