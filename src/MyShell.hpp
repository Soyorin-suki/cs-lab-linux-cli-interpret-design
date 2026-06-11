#pragma once
#include<string>
#include<vector>

class MyShell{
private:
	std::string read_line();
	std::vector<std::string> split_line(const std::string &line);
	int execute(const std::vector<std::string> &args);

public:
	MyShell();
	~MyShell();
	void loop();
};
