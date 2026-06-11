#include "MyShell.hpp"
#include <vector>
#include <string>
#include <iostream>
using std::string;
using std::vector;


MyShell::MyShell(){}
MyShell::~MyShell(){}





void MyShell::loop(){
	string line;
	vector<string> args;
	int status;

	do {
		printf("> ");
		line=read_line();
		args=split_line(line);
		status=execute(args);
	}while(status);
}

const int BUF_MAX_SIZE = 1024;

string MyShell::read_line(){
	string line;
	std::getline(std::cin, line);
	return line;
}
vector<string> MyShell::split_line(const string&line){
	vector<string>args;
	string token;
	for(auto ch:line){
		if(ch==' '||ch=='\t'){
			if(!token.empty()){
				args.push_back(token);
				token="";
			}
		}else{
			token+=ch;
		}
	}
	if(!token.empty()){
		args.push_back(token);
	}
	return args;
}

int MyShell::execute(const vector<string>&args){
	int status=1;
	if(args.empty())return status;
	pid_t pid,wpid;
	
	
	return status;
}

