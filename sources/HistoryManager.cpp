// Ilie Dumitru
// Neculae Andrei-Fabian

#include "HistoryManager.h"
#include <filesystem>

std::string HistoryManager::m_historyFileLocation = "history.txt";

// Constructor. Loads the history from the disk
HistoryManager::HistoryManager()
{
	if (m_historyFileLocation == "history.txt")
	{
		std::filesystem::path myPath = std::filesystem::current_path();
		m_historyFileLocation = myPath.string() + "/history.txt";
	}
	this->load();
}

// Destructor. Saves the history to the disk
HistoryManager::~HistoryManager()
{
	this->save();
}

// Load the history from the disk.
// Currently the convention is that the last instruction read is the most recent
void HistoryManager::load()
{
	std::ifstream in(m_historyFileLocation);
	std::string str;
	
	this->m_isLoaded = false;

	for (this->m_currInstr = -1; getline(in, str, '\n'); )
	{
		this->addInstr(str);
	}

	this->m_isLoaded = true;
	this->save();
}

// Save the memory to the disk
// Currently the convention is that the last instruction read is the most recent
void HistoryManager::save()
{
	int i;
	
	this->m_output = std::ofstream(HistoryManager::m_historyFileLocation);

	if (this->m_isFull)
	{
		for (i = (this->m_currInstr + 1) % m_MAX_INSTR_CNT; i != this->m_currInstr; i = (i + 1) % m_MAX_INSTR_CNT)
		{
			this->m_output << this->m_history[i] << '\n';
		}
		// This command was not printed before as we start from + 1
		this->m_output << this->m_history[i] << '\n';
	}
	else
	{
		for (i = 0; i <= this->m_currInstr; ++i)
		{
			this->m_output << this->m_history[i] << '\n';
		}
	}

	this->m_output << std::flush;
}

// Add an instruction to the buffer.
// Following the convention, this instruction is the most recent one, thus
// it is at the m_currInstr position. For more info read getInstr
void HistoryManager::addInstr(const std::string& a_instr)
{
	bool shouldRefresh = false;

	if (this->m_currInstr + 1 == m_MAX_INSTR_CNT)
	{
		// If we have at least 2 * m_MAX_INSTR_CNT commands in the history
		// save file then we do a cleanup. This has 2 effects.
		// 1. It is unlikely to ever use a command so old.
		// 2. It does not allow the file to get too big in size (although it is
		// still possible)
		if (this->m_isFull)
		{
			shouldRefresh = true;
		}
		else
		{
			this->m_isFull = true;
		}
	}
	this->m_currInstr = (this->m_currInstr + 1) % m_MAX_INSTR_CNT;
	this->m_history[this->m_currInstr] = a_instr;

	if (this->m_isLoaded)
	{
		if (shouldRefresh)
		{
			this->save();
		}
		else
		{
			this->m_output << a_instr << '\n' << std::flush;
		}
	}
}

// Returns a const std::string pointer pointing to the std::string representing
// the instruction specified by index. The current convention is that
// m_currInstr is the index o the last instruction inserted, m_currInstr - 1 is
// the second to last instruction ... . Keep in mind that the buffer is
// circullar
const std::string* HistoryManager::getInstr(int a_index) const
{
	if (a_index < 0 || a_index >= m_MAX_INSTR_CNT)
	{
		return nullptr;
	}

	a_index = (this->m_currInstr - a_index + m_MAX_INSTR_CNT) % m_MAX_INSTR_CNT;
	if (a_index > this->m_currInstr && ! this->m_isFull)
	{
		return nullptr;
	}

	return this->m_history + a_index;
}

// Clears the history
void HistoryManager::clearHistory()
{
	this->m_currInstr = -1;
	this->m_isFull = false;

	this->m_output = std::ofstream(HistoryManager::m_historyFileLocation);
}

// Get function for the singleton class HistoryManager
HistoryManager& HistoryManager::getInstance()
{
	static HistoryManager myManager;
	return myManager;
}

// The number of currently stored instructions
int HistoryManager::getInstrCount() const
{
	if (this->m_isFull)
	{
		return m_MAX_INSTR_CNT;
	}
	return this->m_currInstr + 1;
}
