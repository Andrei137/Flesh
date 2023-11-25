// Neculae Andrei-Fabian

#ifndef FSL_INTERFACE_H
#define FSL_INTERFACE_H

#include "HistoryManager.h"
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

class Interface
{
    static bool m_aborted;

    Interface() = default;
    ~Interface() = default;

    static void setNonBlocking();

    static void refreshDisplay(const std::string&, const std::string&, int);

    static void handleArrowKeys(std::string&, const std::string&, char, int&, int&);

    static void handleBackspace(std::string&, const std::string&, int&);

    static void handleCtrlC(int);

    // prints the rotten flesh logo
    static void printLogo();

    static std::string getCommand(const std::string&);

    // evaluates the command and acts accordingly
    static void evaluateCommand(const std::string&);
    
public:
    // runs the shell
    static void run();

    static Interface& getInstance();
};

#endif // FSL_INTERFACE_H
