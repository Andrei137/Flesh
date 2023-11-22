// Ilie Dumitru
#include"historyManager.h"
#include<fstream>

HistoryManager::HistoryManager()
{
	this->load();
}

HistoryManager::~HistoryManager()
{
	this->save();
}

void HistoryManager::load()
{
	std::ifstream in("history.txt");
	int loaded = 0;
	std::string 
	
}
