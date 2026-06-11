#include "Builtin.hpp"
#include <unistd.h>
#include <iostream>


std::unordered_map<
    std::string,
    Builtin_Func
> Builtin::commands;


void Builtin::init(){
	commands["cd"]=buildin_cd;
	commands["exit"]=buildin_exit;
	commands["help"]=buildin_help;
}

int buildin_cd(const std::vector<std::string>&args){
	if(args.size()<2){
		std::cerr<<"cd: 参数过少!\n";
		return 2;
	}
	if(chdir(args[1].c_str())!=0){
		perror("cd");
		return 1;
	}
	return 0;
}

int buildin_exit(const std::vector<std::string>&args){
	return 1;
}

int buildin_help(const std::vector<std::string>&args){
	std::cout<<"这是Anon_chan制作的whut课设: mini-linux-shell\n";
	return 0;
}

int Builtin::execute(const std::string &func_name, const std::vector<std::string>&args){
	auto func = commands.find(func_name);
	if(func==commands.end()){
		// 没有找到
		return -1;
	}
	return commands[func_name](args);
}