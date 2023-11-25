// Neculae Andrei-Fabian

#include "Interface.h"

#define clear() std::cout << "\033[H\033[J"

bool Interface::m_aborted = false;

void Interface::ctrlCHandler(int a_signum)
{
    static_cast<void>(a_signum); // to avoid warning
    m_aborted = true;

    // exit the program to simulate Ctrl + C
    // but this time, the HistoryManager's destructor will be called
    exit(1);
}

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
    std::cout << "    @@@@%#                  \n\n";
}

void Interface::evaluateCommand(const std::string& a_command)
{
    // quit exists the program, but doesn't close the shell (unlike exit)
    // mostly used for testing before solving Ctrl + C
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
            // if the command is history -c, clear the history
            if (a_command[9] == 'c')
            {
                HistoryManager::getInstance().clearHistory();
                std::cout << "Successfully cleared history\n";
                HistoryManager::getInstance().addInstr(a_command);
                return;
            }

            // if the command is history -number, print the last <number> commands
            number = std::stoi(a_command.substr(9, a_command.length() - 8));
        }

        std::vector<std::string> myHistory{ HistoryManager::getInstance().getInstrList(number) };
        for (const std::string& command : myHistory)
        {
            std::cout << command << '\n';
        }
    }
    else if (a_command == "clear")
    {
        clear();
    }
    else if (a_command == "pwd")
    {
        std::cout << fs::current_path() << '\n';
    }
    else if (a_command.substr(0, 2) == "cd")
    {
        std::string directory = a_command.substr(3, a_command.length() - 3);

        // checks if the directory is between quotes or has the last quote missing
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

        fs::path newPath{ fs::current_path() / directory };
        if (fs::exists(newPath))
        {
            fs::current_path(newPath);
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

    // for better visibility
    if (a_command != "clear")
    {
        std::cout << '\n';
    }
}

void Interface::run()
{
    printLogo();
    signal(SIGINT, ctrlCHandler);
    while (!m_aborted)
    {
        std::cout << fs::current_path().string() + ">";
        std::string myCommand{};
        std::getline(std::cin, myCommand);
        evaluateCommand(myCommand);
    }
}

Interface& Interface::getInstance()
{
    static Interface myInterface{};
    return myInterface;
}