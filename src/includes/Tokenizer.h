// Buzatu Giulian
// Neculae Andrei-Fabian

#ifndef FSL_TOKENIZER_H
#define FSL_TOKENIZER_H

#include <vector>
#include <string>

namespace Tokenizer
{
    // Returns a vector of tokens from the command
    // A token is a small word/set of symbols that helps us to understand what command the user wants to execute
    // Internally, a token is a std::string
    std::vector<std::string> tokenize(const std::string&);

    std::string detokenize(const std::vector<std::string>&);
}

#endif // FSL_TOKENIZER_H