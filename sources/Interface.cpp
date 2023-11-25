// Neculae Andrei-Fabian
// Ilie Dumitru (some refactoring)

#include "Interface.h"
#include "HistoryManager.h"
#include <iostream>
#include <signal.h>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <filesystem>

// \033[H moves the cursor to the top left corner of the screen
// \033[J clears the screen from the cursor to the end of the screen
#define clear() std::cout << "\033[H\033[J"

Interface::Interface() : m_aborted(false) {}

void Interface::setNonBlocking() 
{
    struct termios ttystate;
    memset(&ttystate, 0, sizeof(ttystate));

    // Get the current terminal state
    if (tcgetattr(STDIN_FILENO, &ttystate) == -1) 
    {
        perror("Error getting terminal attributes");
        // TODO return errno
        return;
    }

    // Disable canonical mode so that input characters are immediately available
    // Disable echo so that characters such as the arrow keys aren't printed
    // Output has to be handled manually
    ttystate.c_lflag &= ~(ICANON | ECHO);

    // Apply the new settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &ttystate) == -1) 
    {
        perror("Error setting terminal attributes");
        // TODO return errno
        return;
    }
    
    // TODO return 0
}

void Interface::refreshDisplay(const std::string& a_command, const std::string& a_path, int a_cursorPosition) 
{
    // Move to the beginning of the line, printing the path and the command again
    std::cout << "\r" << a_path << a_command << "\033[K";

    // Move cursor to the correct position
    std::cout << "\033[" << a_cursorPosition + a_path.size() + 1 << "G";

    // Ensure that the output is printed immediately
    std::cout.flush();
}

void Interface::handleArrowKeys(std::string &a_command, const std::string& a_path, char a_arrowKey, int& a_cursorPosition, int& a_historyPosition)
{
    if (a_arrowKey == 'A') // Up Arrow pressed, move to the next command if possible
    {
        const std::string* command = HistoryManager::getInstance().getInstr(a_historyPosition + 1);
        
        if (command != nullptr)
        {
            // We didn't hit the history limit so just get that command
            a_command = *command;
            ++a_historyPosition;
        }
        a_cursorPosition = static_cast<int>(a_command.size());
        refreshDisplay(a_command, a_path, a_cursorPosition);
    }
    else if (a_arrowKey == 'B') // Down Arrow pressed, move to the previous command if possible
    {
        if (a_historyPosition > 0)
        {
            --a_historyPosition;
            a_command = *HistoryManager::getInstance().getInstr(a_historyPosition);
        }
        else
        {
            a_historyPosition = -1;
            a_command = "";
        }
        a_cursorPosition = static_cast<int>(a_command.size());
        refreshDisplay(a_command, a_path, a_cursorPosition);
    }
    else if (a_arrowKey == 'C') // Right Arrow pressed, move cursor to the right if possible
    {
        if (a_cursorPosition < static_cast<int>(a_command.size())) 
        {
            ++a_cursorPosition;
            refreshDisplay(a_command, a_path, a_cursorPosition);
        }
    }
    else if (a_arrowKey == 'D') // Left Arrow pressed, move cursor to the left if possible
    {
        if (a_cursorPosition > 0) 
        {
            --a_cursorPosition;
            refreshDisplay(a_command, a_path, a_cursorPosition);
        }
    }
}

// Simulates backspace (as it was disabled by setNonBlocking)
void Interface::handleBackspace(std::string &a_command, const std::string& a_path, int& a_cursorPosition) 
{
    if (a_cursorPosition > 0) 
    {
        a_command.erase(a_cursorPosition - 1, 1);
        --a_cursorPosition;
        refreshDisplay(a_command, a_path, a_cursorPosition);
    }
}

void Interface::handleCtrlC(int)
{
    getInstance().m_aborted = true;
    std::cout << '\n';

    // Exit the program to simulate Ctrl + C
    // But this time, HistoryManager's destructor will be called
    exit(1);
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
std::string Interface::getCommand(const std::string& a_path)
{
    std::string myCommand{};
    int cursorPosition{ 0 };
    int historyPosition{ -1 };

    setNonBlocking();
    refreshDisplay(myCommand, a_path, cursorPosition);

    while (true)
    {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0)
        {
            // TODO Magic number
            if (c == 27) // Check for escape character (ASCII value for escape)
            {
                // TODO Check if something other than ESC and {A, B, C, D} can be read
                // Arrow key detected; read the next two characters to identify the specific arrow key
                if (read(STDIN_FILENO, &c, 1) > 0 && read(STDIN_FILENO, &c, 1) > 0)
                {
                    handleArrowKeys(myCommand, a_path, c, cursorPosition, historyPosition);
                }
            }
            // TODO Magic number
            else if (c == 127) // Check for backspace
            {
                handleBackspace(myCommand, a_path, cursorPosition);
            }
            else if (c == '\n') // Finalize the command when the user presses enter
            {
                std::cout << '\n';
                return myCommand;
            }
            else // Any other character is added to the command after the cursor
            {
                myCommand.insert(cursorPosition, 1, c);
                ++cursorPosition;
                refreshDisplay(myCommand, a_path, cursorPosition);
            }
        }
    }

    // Something went wrong, return an empty string
    return "";
}

void Interface::evaluateCommand(const std::string& a_command)
{
    // Quit exits the program, but doesn't close the shell (unlike exit)
    // Mostly used for testing before solving Ctrl + C
    if (a_command == "quit")
    {
        m_aborted = true;
        HistoryManager::getInstance().addInstr(a_command);
        return;
    }
    else if (a_command.substr(0, 7) == "history")
    {
        int number{ 0 };

        if (a_command.length() > 9)
        {
            // If the command is history -c, clear the history
            if (a_command[9] == 'c')
            {
                HistoryManager::getInstance().clearHistory();
                std::cout << "Successfully cleared history\n\n";
                HistoryManager::getInstance().addInstr(a_command);
                return;
            }

            // If the command is history -number, print the last <number> commands
            number = std::stoi(a_command.substr(9, a_command.length() - 8));
        }

        // If number is 0, print all the commands
        // Else, print the last <number> commands
        if (number == 0)
        {
            number = -1;
        }
        HistoryManager& managr = HistoryManager::getInstance();
        const std::string* instr = managr.getInstr(0);
        
        for (int i{ 1 }; number && instr; ++i, --number)
        {
            std::cout << *instr << '\n';
            instr = managr.getInstr(i);
        }
    }
    else if (a_command == "clear")
    {
        clear();
    }
    else if (a_command == "pwd" || a_command == "cd")
    {
        std::cout << std::filesystem::current_path() << '\n';
    }
    else if (a_command.substr(0, 2) == "cd")
    {
        std::string directory = a_command.substr(3, a_command.length() - 3);

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
        std::cout << "Invalid command\n";
    }

    HistoryManager::getInstance().addInstr(a_command);

    // For better visibility
    if (a_command != "clear")
    {
        std::cout << '\n';
    }
}

void Interface::run()
{
    printLogo();
    signal(SIGINT, handleCtrlC);
    while (!m_aborted)
    {
        std::string myCommand{ getCommand(std::filesystem::current_path().string() + ">") };
        evaluateCommand(myCommand);
    }
}

Interface& Interface::getInstance()
{
    static Interface myInterface{};
    return myInterface;
}
