// Ilie Dumitru (some refactoring)
// Neculae Andrei-Fabian (TODOs)

#include "HistoryManager.h"
#include "Interface.h"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

// \033[H moves the cursor to the top left corner of the screen
// \033[J clears the screen from the cursor to the end of the screen
#define clear() std::cout << "\033[H\033[J"

#define ESCAPE 27
#define UP_ARROW 'A'
#define DOWN_ARROW 'B'
#define RIGHT_ARROW 'C'
#define LEFT_ARROW 'D'
#define BACKSPACE 127

Interface::Interface() : m_aborted(false) {}

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
    std::cout << "\r" << m_path << m_command << "\033[K";

    // Move cursor to the correct position
    std::cout << "\033[" << a_cursorPosition + m_path.size() + 1 << "G";

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
            m_command = *command;
            ++a_historyPosition;
        }
        a_cursorPosition = static_cast<int>(m_command.size());
        refreshDisplay(a_cursorPosition);
    }
    else if (a_arrowKey == DOWN_ARROW) // Down Arrow pressed, move to the previous command if possible
    {
        if (a_historyPosition > 0)
        {
            --a_historyPosition;
            m_command = *HistoryManager::getInstance().getInstr(a_historyPosition);
        }
        else
        {
            a_historyPosition = -1;
            m_command = "";
        }
        a_cursorPosition = static_cast<int>(m_command.size());
        refreshDisplay(a_cursorPosition);
    }
    else if (a_arrowKey == RIGHT_ARROW) // Right Arrow pressed, move cursor to the right if possible
    {
        if (a_cursorPosition < static_cast<int>(m_command.size())) 
        {
            ++a_cursorPosition;
            refreshDisplay(a_cursorPosition);
        }
    }
    else if (a_arrowKey == LEFT_ARROW) // Left Arrow pressed, move cursor to the left if possible
    {
        if (a_cursorPosition > 0) 
        {
            --a_cursorPosition;
            refreshDisplay(a_cursorPosition);
        }
    }
    else // Unwanted key pressed, do nothing
    {
        return;
    }
}

// Simulates backspace (as it was disabled by configTerminal)
void Interface::handleBackspace(int& a_cursorPosition) 
{
    if (a_cursorPosition > 0) 
    {
        m_command.erase(a_cursorPosition - 1, 1);
        --a_cursorPosition;
        refreshDisplay(a_cursorPosition);
    }
}

void Interface::handleCtrlC(int)
{
    getInstance().m_aborted = true;
    std::cout << '\n';
    getInstance().configTerminal(false);

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
std::string Interface::getCommand()
{
    int cursorPosition{ 0 };
    int historyPosition{ -1 };
    m_command = "";

    getInstance().configTerminal(true);
    refreshDisplay(cursorPosition);

    while (true)
    {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0)
        {
            if (c == ESCAPE) // Check for escape character (ASCII value for escape)
            {
                // TODO Check if something other than ESC and {A, B, C, D} can be read
                // Arrow key detected; read the next two characters to identify the specific arrow key
                if (read(STDIN_FILENO, &c, 1) > 0 && read(STDIN_FILENO, &c, 1) > 0)
                {
                    handleArrowKeys(c, cursorPosition, historyPosition);
                }
            }
            else if (c == BACKSPACE) // Check for backspace
            {
                handleBackspace(cursorPosition);
            }
            else if (c == '\n') // Finalize the command when the user presses enter
            {
                std::cout << '\n';
                return m_command;
            }
            else // Any other character is added to the command after the cursor
            {
                m_command.insert(cursorPosition, 1, c);
                ++cursorPosition;
                refreshDisplay(cursorPosition);
            }
        }
    }

    // Something went wrong, return an empty string
    return "";
}

void Interface::evaluateCommand()
{
    // Quit exits the program, but doesn't close the shell (unlike exit)
    // Mostly used for testing before solving Ctrl + C
    if (m_command == "quit")
    {
        m_aborted = true;
        HistoryManager::getInstance().addInstr(m_command);
        getInstance().configTerminal(false);
        clear();
        return;
    }
    if (m_command.substr(0, 7) == "history")
    {
        int number{ 0 };

        if (m_command.length() > 9)
        {
            // If the command is history -c, clear the history
            if (m_command[9] == 'c')
            {
                HistoryManager::getInstance().clearHistory();
                std::cout << "Successfully cleared history\n\n";
                HistoryManager::getInstance().addInstr(m_command);
                return;
            }

            // If the command is history -number, print the last <number> commands
            number = std::stoi(m_command.substr(9, m_command.length() - 8));
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
    else if (m_command == "clear")
    {
        clear();
    }
    else if (m_command == "pwd" || m_command == "cd")
    {
        std::cout << std::filesystem::current_path() << '\n';
    }
    else if (m_command.substr(0, 2) == "cd")
    {
        std::string directory = m_command.substr(3, m_command.length() - 3);

        // Checks if the directory is between quotes or has the last quote missing
        if (m_command[3] == '\"')
        {
            if (m_command[m_command.length() - 1] == '\"')
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

    HistoryManager::getInstance().addInstr(m_command);

    // For better visibility
    if (m_command != "clear")
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
        m_path = std::filesystem::current_path().string() + ">";
        m_command = getCommand();
        evaluateCommand();
    }
}

Interface& Interface::getInstance()
{
    static Interface myInterface{};
    return myInterface;
}
