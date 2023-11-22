// Ilie Dumitru
#include<string>
#include<vector>

// A singleton class that stores data regarding the instruction history 
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

	// Loads the history from the disk
	void load();

	// Saves the history to the disk
	void save();

public:
	// Adds the instruction in the memory
	void addInstr(const std::string& instr);

	// Get the instruction specified by the index.
	// If the index exceeds MAX_INSTR_CNT then a nullptr is returned.
	// If the index is negative or there are not enough instructions a
	// nullptr is returned.
	// getInstr(0) returns the most recent instruction,
	// getInstr(1) returns the second most recent instruction,
	// ...
	std::string* getInstr(int index);

	// The class is a singleton thus we need a get method
	// This is that method
	HistoryManager& getManager();
};
