//Buzatu Giulian

#ifndef FSL_TOKENIZER_H
#define FSL_TOKENIZER_H

#include <vector>
#include <string>

namespace Tokenizer
{
    // Returns a vector of tokens from the command
    // A token is a small word/set of symbols that helps us to understand what command the user wants to execute
    // Internally, a token is a std::string
    std::vector<std::string> tokenize(const std::string& a_command);
}

#endif // FSL_TOKENIZER_H