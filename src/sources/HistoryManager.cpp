// Ilie Dumitru
// Neculae Andrei-Fabian

#include "HistoryManager.h"
#include <filesystem>

std::string HistoryManager::m_history_file_location{ "history.txt" };

// Constructor. Loads the history from the disk
HistoryManager::HistoryManager()
{
	if (m_history_file_location == "history.txt")
	{
		std::filesystem::path my_path = std::filesystem::current_path();
		m_history_file_location = my_path.string() + "/history.txt";
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
	std::ifstream in(m_history_file_location);
	std::string str{};
	
	this->m_is_loaded = false;

	for (this->m_curr_instr = -1; getline(in, str, '\n'); )
	{
		this->add_instr(str);
	}

	this->m_is_loaded = true;
	this->save();
}

// Save the memory to the disk
// Currently the convention is that the last instruction read is the most recent
void HistoryManager::save()
{
	this->m_output = std::ofstream(HistoryManager::m_history_file_location);

	if (this->m_is_full)
	{
		int i{};
		for (i = (this->m_curr_instr + 1) % m_MAX_INSTR_CNT; i != this->m_curr_instr; i = (i + 1) % m_MAX_INSTR_CNT)
		{
			this->m_output << this->m_history[i] << '\n';
		}
		// This command was not printed before as we start from + 1
		this->m_output << this->m_history[i] << '\n';
	}
	else
	{
		for (int i = 0; i <= this->m_curr_instr; ++i)
		{
			this->m_output << this->m_history[i] << '\n';
		}
	}

	this->m_output << std::flush;
}

// Add an instruction to the buffer.
// Following the convention, this instruction is the most recent one, thus
// it is at the m_curr_instr position. For more info read get_instr
void HistoryManager::add_instr(const std::string& a_instr)
{
	bool should_refresh{ false };

	if (this->m_curr_instr + 1 == m_MAX_INSTR_CNT)
	{
		// If we have at least 2 * m_MAX_INSTR_CNT commands in the history
		// save file then we do a cleanup. This has 2 effects.
		// 1. It is unlikely to ever use a command so old.
		// 2. It does not allow the file to get too big in size (although it is
		// still possible)
		if (this->m_is_full)
		{
			should_refresh = true;
		}
		else
		{
			this->m_is_full = true;
		}
	}
	this->m_curr_instr = (this->m_curr_instr + 1) % m_MAX_INSTR_CNT;
	this->m_history[this->m_curr_instr] = a_instr;

	if (this->m_is_loaded)
	{
		if (should_refresh)
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
// m_curr_instr is the index o the last instruction inserted, m_curr_instr - 1 is
// the second to last instruction ... . Keep in mind that the buffer is
// circullar
const std::string* HistoryManager::get_instr(int a_index) const
{
	if (a_index < 0 || a_index >= m_MAX_INSTR_CNT)
	{
		return nullptr;
	}

	a_index = (this->m_curr_instr - a_index + m_MAX_INSTR_CNT) % m_MAX_INSTR_CNT;
	if (a_index > this->m_curr_instr && ! this->m_is_full)
	{
		return nullptr;
	}

	return this->m_history + a_index;
}

// Clears the history
void HistoryManager::clear_history()
{
	this->m_curr_instr = -1;
	this->m_is_full = false;
	this->m_output = std::ofstream(HistoryManager::m_history_file_location);
}

// The number of currently stored instructions
int HistoryManager::get_instr_count() const
{
	if (this->m_is_full)
	{
		return m_MAX_INSTR_CNT;
	}
	return this->m_curr_instr + 1;
}

HistoryManager& HistoryManager::get_instance()
{
	static HistoryManager manager{};
	return manager;
}