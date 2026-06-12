#pragma once
#include <vector>
#include <string>

class History{
private:
	static std::vector<std::string>history;

public:
	static void push(const std::string&args);
	static std::vector<std::string>get_history();
};