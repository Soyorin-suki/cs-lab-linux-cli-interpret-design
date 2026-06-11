#include "Builtin.hpp"

std::unordered_map<
    std::string,
    Builtin_Func
> Builtin::commands;


void Builtin::init(){
	commands["cd"]=buildin_cd;
	commands["exit"]=buildin_exit;
}

int buildin_cd(const std::vector<std::string>&args){
	return 0;
}

int buildin_exit(const std::vector<std::string>&args){
	return 1;
}

int Builtin::execute(const std::string &func_name, const std::vector<std::string>&args){
	auto func = commands.find(func_name);
	if(func==commands.end()){
		// 没有找到
		return -1;
	}
	return commands[func_name](args);
}