// Ilie Dumitru
// Neculae Andrei-Fabian

#ifndef FSL_HISTORYMANAGER_H
#define FSL_HISTORYMANAGER_H

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// A singleton class that stores data regarding the instruction history 
// Will be updated soon to allow multiple instances of flesh to run at once without issues
class HistoryManager
{
public:
	// Maximum number of instructions stored
	static const int m_MAX_INSTR_CNT = 4096;

private:
	// Instruction list, hard capped at m_MAX_INSTR_CNT instructions
	// Stored as a circular buffer, where currInstr is the most recent one,
	// (m_currInstr - 1 + m_MAX_INSTR_CNT) % m_MAX_INSTR_CNT is the second most recent,
	// ...
	std::string m_history[m_MAX_INSTR_CNT];

	// The most recent instruction. Check m_history and m_isFull for more info.
	int m_currInstr;
	
	// Is the buffer full? (Can we loop around or do we stop at 0)
	bool m_isFull;

	// Loads the history. Is private to make the class a Singleton
	HistoryManager();

	// Saves the history. Is private to make the class a Singleton
	~HistoryManager();

	// Loads the history from the disk. Check implementation for more details
	void load();

	// Saves the history to the disk. Check implementation for more details
	void save();

public:
	int getInstrCount() const;

	// Adds the instruction in the buffer.
	// Should be called when an instruction was ran by the user
	void addInstr(const std::string&);

	// Get the instruction specified by the index.
	// If the index exceeds MAX_INSTR_CNT then a nullptr is returned.
	// If the index is negative or there are not enough instructions a
	// nullptr is returned.
	// getInstr(0) returns the most recent instruction,
	// getInstr(1) returns the second most recent instruction,
	// ...
	const std::string* getInstr(int) const;

	// Get the list of all instructions
	const std::vector<std::string> getInstrList(int) const;

	// clears both the vector and the file
	void clearHistory();

	// The class is a singleton thus we need a get method
	// This is that method
	static HistoryManager& getInstance();
};

#endif // FSL_HISTORYMANAGER_H
