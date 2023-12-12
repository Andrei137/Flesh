// Neculae Andrei-Fabian

#include "Color.h"

namespace Color
{
    Modifier::Modifier(int a_code) : m_code(a_code) {}

    std::ostream& operator<<(std::ostream& a_os, const Modifier& a_mod)
    {
        return a_os << "\033[" << a_mod.m_code << "m";
    }
}