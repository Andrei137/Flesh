// Neculae Andrei-Fabian

#ifndef FSL_TERMINALMODIFIER_H
#define FSL_TERMINALMODIFIER_H

#include <iostream>

namespace TerminalModifier
{
    class Modifier
    {
        int m_code;
    public:
        explicit Modifier(int);
        friend std::ostream& operator<<(std::ostream&, const Modifier&);
    };

	// Special formatting
	const Modifier FT_BOLD             {   1 };
	const Modifier FT_UNDERLINE        {   5 };

    // Foreground colors
    const Modifier FG_DEFAULT          {  39 };
    const Modifier FG_BLACK            {  30 };
    const Modifier FG_RED              {  31 };
    const Modifier FG_GREEN            {  32 };
    const Modifier FG_YELLOW           {  33 };
    const Modifier FG_BLUE             {  34 };
    const Modifier FG_MAGENTA          {  35 };
    const Modifier FG_CYAN             {  36 };
    const Modifier FG_LIGHT_GRAY       {  37 };
    const Modifier FG_DARK_GRAY        {  90 };
    const Modifier FG_LIGHT_RED        {  91 };
    const Modifier FG_LIGHT_GREEN      {  92 };
    const Modifier FG_LIGHT_YELLOW     {  93 };
    const Modifier FG_LIGHT_BLUE       {  94 };
    const Modifier FG_LIGHT_MAGENTA    {  95 };
    const Modifier FG_LIGHT_CYAN       {  96 };
    const Modifier FG_WHITE            {  97 };

    // Background colors
    const Modifier BG_DEFAULT       {  49 };
    const Modifier BG_BLACK         {  40 };
    const Modifier BG_RED           {  41 };
    const Modifier BG_GREEN         {  42 };
    const Modifier BG_YELLOW        {  43 };
    const Modifier BG_BLUE          {  44 };
    const Modifier BG_MAGENTA       {  45 };
    const Modifier BG_CYAN          {  46 };
    const Modifier BG_LIGHT_GRAY    {  47 };
    const Modifier BG_DARK_GRAY     { 100 };
    const Modifier BG_LIGHT_RED     { 101 };
    const Modifier BG_LIGHT_GREEN   { 102 };
    const Modifier BG_LIGHT_YELLOW  { 103 };
    const Modifier BG_LIGHT_BLUE    { 104 };
    const Modifier BG_LIGHT_MAGENTA { 105 };
    const Modifier BG_LIGHT_CYAN    { 106 };
    const Modifier BG_WHITE         { 107 };

}

#endif // FSL_TERMINALMODIFIER_H
