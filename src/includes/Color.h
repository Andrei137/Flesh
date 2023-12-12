// Neculae Andrei-Fabian

#ifndef FSL_COLOR_H
#define FSL_COLOR_H

#include <iostream>

namespace Color
{
    class Modifier
    {
        int m_code;
    public:
        explicit Modifier(int);
        friend std::ostream& operator<<(std::ostream&, const Modifier&);
    };

    // Foreground colors
    const Modifier DEFAULT          {  39 };
    const Modifier BLACK            {  30 };
    const Modifier RED              {  31 };
    const Modifier GREEN            {  32 };
    const Modifier YELLOW           {  33 };
    const Modifier BLUE             {  34 };
    const Modifier MAGENTA          {  35 };
    const Modifier CYAN             {  36 };
    const Modifier LIGHT_GRAY       {  37 };
    const Modifier DARK_GRAY        {  90 };
    const Modifier LIGHT_RED        {  91 };
    const Modifier LIGHT_GREEN      {  92 };
    const Modifier LIGHT_YELLOW     {  93 };
    const Modifier LIGHT_BLUE       {  94 };
    const Modifier LIGHT_MAGENTA    {  95 };
    const Modifier LIGHT_CYAN       {  96 };
    const Modifier WHITE            {  97 };

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

#endif // FSL_COLOR_H