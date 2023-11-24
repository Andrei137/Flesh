// Ilie Dumitru
#include "HistoryManager.h"
#include <fstream>

// Constructor. Loads the history from the disk
HistoryManager::HistoryManager()
{
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
	std::ifstream in("history.txt");
	std::string str;
	
	for (this->m_currInstr = 0; getline(in, str, '\n'); this->m_isFull = !(this->m_currInstr = (this->m_currInstr + 1) % m_MAX_INSTR_CNT))
	{
		this->m_history[this->m_currInstr] = str;
	}

	this->m_currInstr = (this->m_currInstr - 1 + m_MAX_INSTR_CNT) % m_MAX_INSTR_CNT;
}

// Save the memory to the disk
// Currently the convention is that the last instruction read is the most recent
void HistoryManager::save()
{
	int i;
	std::ofstream out("history.txt");

	if (this->m_isFull)
	{
		for (i = (this->m_currInstr + 1) % m_MAX_INSTR_CNT; i != this->m_currInstr; i = (i + 1) % m_MAX_INSTR_CNT)
		{
			out << this->m_history[i] << '\n';
		}
	}
	else
	{
		for (i = 0; i <= this->m_currInstr; ++i)
		{
			out << this->m_history[i] << '\n';
		}
	}
}

// Add an instruction to the buffer.
// Following the convention, this instruction is the most recent one, thus
// it is at the m_currInstr position. For more info read getInstr
void HistoryManager::addInstr(const std::string& a_instr)
{
	this->m_currInstr = (this->m_currInstr + 1) % m_MAX_INSTR_CNT;
	this->m_history[this->m_currInstr] = a_instr;
}

// Returns a const std::string pointer pointing to the std::string representing
// the instruction specified by index. The current convention is that
// m_currInstr is the index o the last instruction inserted, m_currInstr - 1 is
// the second to last instruction ... . Keep in mind that the buffer is
// circullar
const std::string* HistoryManager::getInstr(int a_index)
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

// Get function for the singleton class HistoryManager
HistoryManager& HistoryManager::getManager()
{
	static HistoryManager managr;
	return managr;
}
