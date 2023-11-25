// Neculae Andrei-Fabian

#ifndef FSL_INTERFACE_H
#define FSL_INTERFACE_H

#include "HistoryManager.h"
#include <iostream>
#include <cstring>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

// A singleton class that handles the shell's interface
class Interface
{
    // Checks if the user pressed Ctrl+C or typed quit to exit the shell
    static bool m_aborted;

    Interface() = default;
    ~Interface() = default;

    // Disables canonical mode and echo
    static void setNonBlocking();

    // Refreshed the display when the user alters the current command
    static void refreshDisplay(const std::string&, const std::string&, int);

    // Handles the arrow keys' functionality
    static void handleArrowKeys(std::string&, const std::string&, char, int&, int&);

    // Handles the backspace key's functionality
    static void handleBackspace(std::string&, const std::string&, int&);

    // Handles Ctrl + C's signal (SIGINT)
    static void handleCtrlC(int);

    // Not the logo we deserved, but the logo we needed
    static void printLogo();

    // When the user presses enter, the command is returned
    static std::string getCommand(const std::string&);

    // Evaluates the command and acts accordingly
    static void evaluateCommand(const std::string&);
    
public:
    // Runs the shell
    static void run();

    // The class is a singleton thus we need a get method
    // This is that method
    static Interface& getInstance();
};

#endif // FSL_INTERFACE_H
