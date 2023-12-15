// Ilie Dumitru
// Neculae Andrei-Fabian

#include "Color.h"
#include "HistoryManager.h"
#include "Interface.h"
#include "Interpreter.h"
#include "Tokenizer.h"
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

// Defining the ASCII codes for some special characters
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
    HistoryManager::get_instance();

    // Get an instance of Interpreter to have the signal handlers initialized
    Interpreter::get_instance();
}

void Interface::refresh_display(int a_cursor_position) const
{
    // Move to the beginning of the line, printing the path and the command again
    std::cout << "\r" << Color::BLUE << this->m_path << Color::DEFAULT << this->m_command << "\033[K";

    // Move cursor to the correct position
    std::cout << "\033[" << a_cursor_position + this->m_path.size() + 1 << "G";

    // Ensure that the output is printed immediately
    std::cout.flush();
}

void Interface::handle_arrow_keys(char a_arrow_key, int& a_cursor_position, int& a_history_position)
{
    if (a_arrow_key == UP_ARROW) // Up Arrow pressed, move to the next command if possible
    {
        const std::string* command{ HistoryManager::get_instance().get_instr(a_history_position + 1) };
        
        if (command != nullptr)
        {
            // We didn't hit the history limit so just get that command
            this->m_command = *command;
            ++a_history_position;
        }
        a_cursor_position = static_cast<int>(this->m_command.size());
        refresh_display(a_cursor_position);
    }
    else if (a_arrow_key == DOWN_ARROW) // Down Arrow pressed, move to the previous command if possible
    {
        if (a_history_position > 0)
        {
            --a_history_position;
            this->m_command = *HistoryManager::get_instance().get_instr(a_history_position);
        }
        else
        {
            a_history_position = -1;
            this->m_command = "";
        }
        a_cursor_position = static_cast<int>(this->m_command.size());
        refresh_display(a_cursor_position);
    }
    else if (a_arrow_key == RIGHT_ARROW) // Right Arrow pressed, move cursor to the right if possible
    {
        if (a_cursor_position < static_cast<int>(this->m_command.size())) 
        {
            ++a_cursor_position;
            refresh_display(a_cursor_position);
        }
    }
    else  // Left Arrow pressed, move cursor to the left if possible
    {
        if (a_cursor_position > 0) 
        {
            --a_cursor_position;
            refresh_display(a_cursor_position);
        }
    }
}

void Interface::handle_ctrl_arrow_keys(char a_arrow_key, int& a_cursor_position)
{
    // Remove all trailing spaces from the command
    while (this->m_command[this->m_command.length() - 1] == ' ')
    {
        this->m_command.pop_back();
    }
    if (a_arrow_key == LEFT_ARROW) // Ctrl + Left Arrow pressed, move cursor to the beginning of the current word
    {
        if (a_cursor_position != 0)
        {
            // Add a dummy space at the beginning of the command and later remove it
            this->m_command.insert(0, 1, ' ');

            // Find the previous space before the cursor
            size_t last_space = this->m_command.rfind(' ', a_cursor_position - 1);
            this->m_command.erase(0, 1);

            // If there is no previous space, set the cursor to the beginning of the command
            // Else, set the cursor before the previous space
            a_cursor_position = static_cast<int>(last_space);
            refresh_display(a_cursor_position);
        }
    }
    else // Ctrl + Right Arrow pressed, move cursor to the end of the current word
    {
        if (a_cursor_position != static_cast<int>(this->m_command.length()))
        {
            // Add a dummy space at the end of the command and later remove it
            this->m_command.insert(this->m_command.length(), 1, ' ');

            // Find the next space after the cursor
            size_t next_space = this->m_command.find(' ', a_cursor_position + 1);
            this->m_command.erase(this->m_command.length() - 1, 1);

            // If there is no next space, set the cursor to the end of the command
            // Else, set the cursor after the next space
            if (next_space == this->m_command.length())
            {
                --next_space;
            }
            a_cursor_position = static_cast<int>(next_space) + 1;
            refresh_display(a_cursor_position);
        }
    }
}

// Simulates backspace (as it was disabled by config_terminal)
void Interface::handle_backspace(int& a_cursor_position) 
{
    if (a_cursor_position > 0) 
    {
        this->m_command.erase(a_cursor_position - 1, 1);
        --a_cursor_position;
        refresh_display(a_cursor_position);
    }
}

// Returns the command when the user presses enter
// Until then, the command can be modified in any way
std::string Interface::get_command()
{
    int cursor_position{ 0 };
    int history_position{ -1 };
    bool changed{ false };
    char changed_ch{ '\n' };
    this->m_command = "";

    config_terminal(true);
    refresh_display(cursor_position);

    while (true)
    {
        // Changed is used to check if the user pressed tab when autocomplete was available and then pressed something else
        // It implies the user wants to use the current autocomplete option and probably pressed /, enter or space
        // To avoid reading the same character twice, we use changed_ch to remember it
        // We only read a new character if changed is false, otherwise we use the one we already have
        char curr_ch{ changed_ch };
        if (!changed)
        {
            curr_ch = getchar();
        }
        if (curr_ch == CTRL_D)
        {
            // The user pressed CTRL + D
            // We need to exit the program
            this->m_aborted = true;
            config_terminal(false);
            clear();
            return "";
        }
        // Check for escape sequence
        if (curr_ch == ESCAPE)
        {
            curr_ch = getchar();
            // Check for the '[' character
            if (curr_ch == RIGHT_SQUARE_BRACKET) 
            {
                curr_ch = getchar();
                // Check for arrow keys
                if (curr_ch == UP_ARROW || curr_ch == DOWN_ARROW || curr_ch == RIGHT_ARROW || curr_ch == LEFT_ARROW)
                {
                    handle_arrow_keys(curr_ch, cursor_position, history_position);
                }
                // Check For Ctrl + Left / Right Arrow Key
                else if (curr_ch == ONE)
                {
                    curr_ch = getchar();
                    if (curr_ch == SEMICOLON)
                    {
                        curr_ch = getchar();
                        if (curr_ch == FIVE)
                        {
                            curr_ch = getchar();
                            if (curr_ch == RIGHT_ARROW || curr_ch == LEFT_ARROW)
                            {
                                handle_ctrl_arrow_keys(curr_ch, cursor_position);
                            }
                        }
                    }
                }
            }
        }
        // Check for backspace
        else if (curr_ch == BACKSPACE)
        {
            handle_backspace(cursor_position);
        }
        // Check for tab
        else if (curr_ch == '\t')
        {
            // Get the current word from the command
            m_command.insert(0, 1, ' ');
            int last_space{ static_cast<int>(m_command.rfind(' ', cursor_position - 1)) - 1 };
            m_command.erase(0, 1);
            std::string path{ m_command.substr(last_space + 1, cursor_position - last_space - 1) };
            
            // If it has / in it, it is most likely a path or the command has cd in it
            if (path.find('/') != std::string::npos)
            {
                // Get the information from the path
                // The directory is everything before the last /
                // The file_name is everything after the last /, most likely a part of the file name
                std::string directory{ path.substr(0, path.find_last_of('/')) };
                std::string file_name{ path.substr(path.find_last_of('/') + 1, path.length() - path.find_last_of('/')) };
                
                // Get all the files from the directory
                std::vector<std::string> files{};
                for (const auto& file : std::filesystem::directory_iterator(directory))
                {
                    files.push_back(file.path().filename().string());
                }

                // Find all the files that start with file_name
                std::vector<std::string> matches{};
                for (const auto& file : files)
                {
                    if (file.substr(0, file_name.length()) == file_name)
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
                    m_command.insert(cursor_position, matches[0].substr(file_name.length(), matches[0].length() - file_name.length()));
                    cursor_position += matches[0].length() - file_name.length();
                    refresh_display(cursor_position);
                }
                // If there are more matches, add them to the command one by one
                else if (matches.size() > 1)
                {
                    // Remember the default values so that we can get back to them if the user presses something else than tab
                    std::string default_command{ m_command };
                    int default_cursor_position{ cursor_position };

                    for (const auto& match : matches)
                    {
                        // Get the old values back
                        m_command = default_command;
                        cursor_position = default_cursor_position;

                        // Change them to the new ones
                        m_command.insert(cursor_position, match.substr(file_name.length(), match.length() - file_name.length()));
                        cursor_position += match.length() - file_name.length();
                        refresh_display(cursor_position);

                        curr_ch = getchar();
                        // If the user presses tab, continue to the next match
                        if (curr_ch == '\t' || curr_ch == ' ')
                        {
                            continue;
                        }
                        // If the user pressed enter or any other character, it means that they want to use the current match
                        else
                        {
                            changed = true;
                            changed_ch = curr_ch;
                            break;
                        }
                    }

                    // If the user didn't keep a value, get back to the default ones
                    if (!changed)
                    {
                        m_command = default_command;
                        cursor_position = default_cursor_position;
                        refresh_display(cursor_position);
                    }
                }
            }
        
        }
        // Finalize the command when the user presses enter
        // \r for when we enter Python
        // Becuase it changes the terminal mode for some reason
        else if (curr_ch == '\n' || curr_ch == '\r')
        {
            std::cout << '\n';
            if (this->m_command == "")
            {
                // The user pressed enter without typing anything
                return "";
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
            this->m_command.insert(cursor_position, 1, curr_ch);
            ++cursor_position;
            refresh_display(cursor_position);
        }
    }

    // Something went terribly wrong, return an empty string
    return "";
}

void Interface::abort()
{
    this->m_aborted = true;
}

void Interface::clear()
{
    // \033c clears the screen and moves the cursor to the top left corner of the screen
    std::cout << "\033c";
}

int Interface::config_terminal(bool a_change) const
{
    struct termios ttystate{};
    memset(&ttystate, 0, sizeof(ttystate));

    // Get the current terminal state
    if (tcgetattr(STDIN_FILENO, &ttystate) == -1) 
    {
        perror("Error getting terminal attributes");
        return errno;
    }

    if (a_change)
    {
        // Disable canonical mode and echo so that characters such as the arrow keys aren't printed
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

// Mom can we have Cooked Porkchop?
// We have Cooked Porkchop at home
// Cooked Porkchop at home:
void Interface::print_logo()
{
    clear();
    const std::string flesh_logo
    {
         "                .--###                            \n"
         "            .==+#*#@@#                            \n"
         "          =#@@%@@=-=*@@#                          \n"
         "        ..-@@-.-+%@@@*+@%..+%#                    \n"
         "        :@#-:=#@@%+-=*#%#=+@@=                    \n"
         "      -##*=-+@@+-+**+::+#%%@@=                    \n"
         "    .==::=#@@*==*%%*=::%@@@*                      \n"
         "  .===-.-%++-.-@@+=+#@@@@@                        \n"
         ".-=++:-%@-*-=*#*+=++#@@#+##=                      \n"
         ".++::@@*+=*%@@*:-%@*+*#*##%+                      \n"
         ".-==*@%-:=++=-+%@##@@@@@@#                        \n"
         "=@@@@==#%=:-*@@@@@@####____ __    ____ ____ __ __ \n"
         "  -#@@@::#@@@%%@@####*/ __// /   / __// __// // / \n"
         "  .=-::*@@@@####     / _/ / /__ / _/ _\\ \\ / _  /\n"
         "    @@@@##          /_/  /____//___//___//_//_/   \n"
         "                                                  \n"
    };

    for (auto ch : flesh_logo)
    {
        std::cout << Color::BG_DEFAULT;
        if (ch == '%')
        {
            std::cout << Color::GREEN << ch;
        }
        else if (ch == '#' || ch == '.')
        {
            std::cout << Color::DARK_GRAY << ch;
        }
        else if (ch == '+' || ch == '*')
        {
            std::cout << Color::LIGHT_RED << ch;
        }
        else if (ch == '@' || ch == '=' || ch == '-' || ch == ':')
        {
            std::cout << Color::RED << ch;
        }
        else
        {
            std::cout << Color::MAGENTA << ch;
        }
    }
    std::cout << Color::DEFAULT;
}

void Interface::run()
{
    print_logo();
    
    while (!this->m_aborted)
    {
        this->m_path = std::filesystem::current_path().string() + ">";
        this->m_command = get_command();
        Interpreter::get_instance().evaluate_command(this->m_command, this->m_path);
    }
}

Interface& Interface::get_instance()
{
    static Interface interface{};
    return interface;
}
