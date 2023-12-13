// Neculae Andrei-Fabian
// Ilie Dumitru (some refactoring)

#ifndef FSL_INTERFACE_H
#define FSL_INTERFACE_H

#include <string>

// A singleton class that handles the shell's interface
class Interface
{
    // Checks if the user pressed Ctrl+C or typed quit or exit to exit the shell
    bool m_aborted;

    // The path to the current directory
    std::string m_path;

    // The current command
    std::string m_command;
    
    pid_t m_child_pid;

    Interface();
    Interface(const Interface&) = delete;
    Interface(Interface&&) = delete;
    Interface& operator=(const Interface&) = delete;
    Interface& operator=(Interface&&) = delete;
    ~Interface() = default;

    // Disables canonical mode and echo or enables them back
    int config_terminal(bool) const;

    // Refreshed the display when the user alters the current command
    void refresh_display(int) const;

    // Handles the arrow keys' functionality
    void handle_arrow_keys(char, int&, int&);

    // Handles the Ctrl + arrow keys' functionality
    void handle_ctrl_arrow_keys(char, int&);

    // Handles the backspace key's functionality
    void handle_backspace(int&);

    // Handles Ctrl + C's signal (SIGINT)
    // This function must be static as it is a generic handler
    static void handle_sig_int(int);

    // Handles Ctrl + \'s signal (SIGQUIT)
    // This function must be static as it is a generic handler
    static void handle_sig_quit(int);

    // Handles Ctrl + Z's signal (SIGTSTP)
    // This function must be static as it is a generic handler
    static void handle_sig_tstp(int);

    // Not the logo we deserved, but the logo we needed
    void print_logo();

    // When the user presses enter, the command is returned
    std::string get_command();

    // Evaluates the command and acts accordingly
    void evaluate_command();
    
public:
    // Runs the flesh
    void run();

    // The class is a singleton thus we need a get method
    // This is that method
    static Interface& get_instance();
};

#endif // FSL_INTERFACE_H