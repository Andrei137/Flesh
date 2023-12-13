// Ilie Dumitru
// Neculae Andrei-Fabian

#ifndef FSL_HISTORYMANAGER_H
#define FSL_HISTORYMANAGER_H

#include <string>
#include <fstream>

// A singleton class that stores data regarding the instruction history 
// Will be updated soon to allow multiple instances of Flesh to run at once without issues
class HistoryManager
{
public:
	// Maximum number of instructions stored
	static const int m_MAX_INSTR_CNT = 4096;

private:
	// The file where the history is stored
	static std::string m_history_file_location;

	// Instruction list, hard capped at m_MAX_INSTR_CNT instructions
	// Stored as a circular buffer, where currInstr is the most recent one,
	// (m_currInstr - 1 + m_MAX_INSTR_CNT) % m_MAX_INSTR_CNT is the second most recent,
	// ...
	std::string m_history[m_MAX_INSTR_CNT];

	// The most recent instruction. Check m_history and m_isFull for more info.
	int m_curr_instr;
	
	// Is the buffer full? (Can we loop around or do we stop at 0)
	bool m_is_full;

	// Has the history finished loading?
	bool m_is_loaded;

	// We use this to save the history at every instruction added to the history.
	// To do this we are flushing the command to the stream every time.
	// Instead of openning the stream for appending every time we just keep it open.
	std::ofstream m_output;

	// Loads the history. Is private to make the class a Singleton
	HistoryManager();

	// The HistoryManager class is a singleton, thus we need to delete the copy and
	// move constructors and assignment operators
	HistoryManager(const HistoryManager&) = delete;
	HistoryManager(HistoryManager&&) = delete;
	HistoryManager& operator=(const HistoryManager&) = delete;
	HistoryManager& operator=(HistoryManager&&) = delete;

	// Saves the final history. It was private initially to disallow deletion,
	// thus keeping the class a Singleton but can safely be made public now.
	~HistoryManager();

	// Loads the history from the disk. Check implementation for more details
	void load();

	// Saves the history to the disk. Check implementation for more details
	void save();

public:
	// Adds the instruction in the buffer.
	// Should be called when an instruction was ran by the user
	void add_instr(const std::string&);

	// Get the instruction specified by the index.
	// If the index exceeds MAX_INSTR_CNT then a nullptr is returned.
	// If the index is negative or there are not enough instructions a
	// nullptr is returned.
	// getInstr(0) returns the most recent instruction,
	// getInstr(1) returns the second most recent instruction,
	// ...
	const std::string* get_instr(int) const;

	// Clears both the vector and the file
	void clear_history();

	// The number of currently stored instructions
	int get_instr_count() const;

	// The class is a singleton thus we need a get method
	// This is that method
	static HistoryManager& get_instance();
};

#endif // FSL_HISTORYMANAGER_H
