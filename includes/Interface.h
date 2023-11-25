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

    Interface();
    ~Interface() = default;

    // Disables canonical mode and echo
    void setNonBlocking();

    // Refreshed the display when the user alters the current command
    void refreshDisplay(const std::string&, const std::string&, int);

    // Handles the arrow keys' functionality
    void handleArrowKeys(std::string&, const std::string&, char, int&, int&);

    // Handles the backspace key's functionality
    void handleBackspace(std::string&, const std::string&, int&);

    // Handles Ctrl + C's signal (SIGINT)
    // This function must be static as it is a generic handler
    static void handleCtrlC(int);

    // Not the logo we deserved, but the logo we needed
    void printLogo();

    // When the user presses enter, the command is returned
    std::string getCommand(const std::string&);

    // Evaluates the command and acts accordingly
    void evaluateCommand(const std::string&);
    
public:
    // Runs the flesh
    void run();

    // The class is a singleton thus we need a get method
    // This is that method
    static Interface& getInstance();
};

#endif // FSL_INTERFACE_H
