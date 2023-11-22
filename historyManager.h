// Ilie Dumitru
#include<string>
#include<vector>

// A singleton class that stores data regarding the instruction history 
class historyManager
{
public:
	// Maximum number of instructions stored
	static const int MAX_INSTR_CNT = 4096;

private:
	// Instruction list, hard capped at MAX_INSTR_CNT instructions
	// Stored as a circular buffer, where currInstr is the most recent one,
	// (currInstr - 1 + MAX_INSTR_CNT) % MAX_INSTR_CNT is the second most recent,
	// ...
	std::string history[MAX_INSTR_CNT];

	// Is the buffer full? (Can we loop around or do we stop at 0)
	bool isFull;

	// The most recent instruction. Check history and isFull for more info.
	int currInstr;
	
	// Loads the history. Is private to make the class a Singleton
	historyManager();

	// Saves the history. Is private to make the class a Singleton
	~historyManager();

	// Loads the history from the disk
	void load();

	// Saves the history on the disk
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
	historyManager& getHistory();
};
