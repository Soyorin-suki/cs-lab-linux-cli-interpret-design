#include "History.hpp"

std::vector<std::string>History::history;

void History::push(const std::string&args){
	history.push_back(args);
}

std::vector<std::string>History::get_history(){
	return history;
}