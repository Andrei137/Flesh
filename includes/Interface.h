// Neculae Andrei-Fabian

#ifndef FSL_INTERFACE_H
#define FSL_INTERFACE_H

#include "HistoryManager.h"
#include <atomic>
#include <iostream>
#include <thread>
#include <signal.h>

class Interface
{
    static bool m_aborted;

    Interface() = default;
    ~Interface() = default;

    static void ctrlCHandler(int);

    // prints the rotten flesh logo
    static void printLogo();

    // evaluates the command and acts accordingly
    static void evaluateCommand(const std::string&);
    
public:
    // runs the shell
    static void run();

    static Interface& getInstance();
};

#endif // FSL_INTERFACE_H
