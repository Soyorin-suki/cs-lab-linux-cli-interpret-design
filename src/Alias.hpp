#pragma once
#include <vector>
#include <unordered_map>
#include <string>


class Alias{
private:
	static std::unordered_map<std::string,std::vector<std::string>>replace_map;
public:
	static void init();
	static std::vector<std::string>replace(const std::vector<std::string>&argv);
};