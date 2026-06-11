#pragma once
#include<string>
#include<vector>

class MyShell{
private:
	std::string read_line();
	std::vector<std::string> split_line(const std::string &line);
	int execute(const std::vector<std::string> &args);
	std::string get_pwd();
	std::string get_prompt();

public:
	void init();
	MyShell();
	~MyShell();
	void loop();
};
