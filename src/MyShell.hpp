#pragma once
#include <string>
#include <vector>
#include "parser/AST.hpp"

class MyShell {
private:
	std::string read_line();
	int execute_ast(const parser::ASTNode* node);
	int execute_cmd(const parser::ASTNode* node);
	std::string get_pwd();
	std::string get_prompt();

public:
	void init();
	MyShell();
	~MyShell();
	void loop();
};
