// Buzatu Giulian
// Neculae Andrei-Fabian

#include "Tokenizer.h"

namespace Tokenizer
{
    // Returns a vector of tokens from the command
    // A token is a small word/set of symbols that helps us to understand what command the user wants to execute
    // Internally, a token is a std::string
    std::vector<std::string> tokenize(const std::string& a_command)
    {
        // The current token is the one that we are currently building
        // The current operator is the current operator between the tokens
        std::string curr_token{};
        std::string curr_operator{};

        // This vector stores the resulting tokens
        std::vector<std::string> tokens{};

        for(int i = 0; i < static_cast<int>(a_command.size()); ++i)
        {
            if (a_command[i] == ' ') // If we have a space as a separator, we add the current token (as long as it is not empty)
            {
                if (!curr_token.empty())
                {
                    tokens.push_back(curr_token);
                }
                curr_token = "";
            }
            // If we have an operator, we add the current token (as long as it is not empty)
            else if (a_command[i] == '|' || a_command[i] == '&' || a_command[i] == '>' || a_command[i] == '<' || a_command[i] == ';')
            {
                if (!curr_token.empty())
                {
                    tokens.push_back(curr_token);
                }
                curr_token = "";
                curr_operator += a_command[i];

                // For |, & and > we can have ||, &&, >>, so we check this case here
                if ((a_command[i] == '|' || a_command[i] == '&' || a_command[i] == '>') && 
                    i != static_cast<int>(a_command.size()) - 1 && a_command[i] == a_command[i + 1])
                {
                    curr_operator += a_command[++i];
                }
                tokens.push_back(curr_operator);
                curr_operator = "";
            }
            // If we don't have a separator, we continue to add to the current token
            else
            {
                // If we encounter a quotation mark or an apostrophe,
                // we continue until that quotation mark or apostrophe is closed
                // because, until that happens, we won't break the string into tokens NO MATTER WHAT
                if (a_command[i] == '\"' || a_command[i] == '\'')
                {
                    int j{ i++ };
                    while (i < static_cast<int>(a_command.size()))
                    {
                        if (a_command[i] == a_command[j])
                        {
                            break;
                        }
                        else
                        {
                            curr_token += a_command[i];
                        }
                        ++i;
                    }
                }
                else
                {
                    curr_token += a_command[i];
                }
            }
        }
        tokens.push_back(curr_token);
        return tokens;
    }
}