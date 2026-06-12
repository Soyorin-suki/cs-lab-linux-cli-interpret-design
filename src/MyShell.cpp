#include "MyShell.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "History.hpp"
#include "Builtin.hpp"
#include "Alias.hpp"
using std::string;
using std::vector;


MyShell::MyShell(){}
MyShell::~MyShell(){}

void MyShell::init(){
	Builtin::init();
	Alias::init();
}
string MyShell::get_pwd(){
	char buf[4096];
	getcwd(buf,sizeof(buf));
	string pwd(buf);
	return pwd;
}

string MyShell::get_prompt(){
	string prompt;
	prompt+="[user-sh]";
	prompt+=get_pwd();
	prompt+="$ ";
	return prompt;
}

void MyShell::loop(){
	string line;
	vector<string> args;
	int status;

	do {
		std::cout<<get_prompt();
		line=read_line();
		args=split_line(line);
		status=execute(args);
	}while(status);
}

const int BUF_MAX_SIZE = 1024;

string MyShell::read_line(){
	string line;
	std::getline(std::cin, line);
	History::push(line);
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
	return Alias::replace(args);
}

int MyShell::execute(const vector<string>&args){
	int status=1;
	if(args.empty())return status;
	pid_t pid,wpid;
	// check是否是内置命令
	status = Builtin::execute(args[0],args);
	if(status==0){
		return 1;
	}else if(status==1){
		return 0;
	}else if(status==2){
		return 1;
	}

	vector<char*>argv;
	for(auto&arg:args){
		argv.push_back(const_cast<char*>(arg.c_str()));
	}
	argv.push_back(nullptr);
	pid=fork();
	if(pid<0){
		// 启动失败
		perror("MyShell启动失败");
	}else if(pid==0){
		// 子进程
		if(execvp(argv[0],argv.data())==-1){
			perror("MyShell");
		}
		exit(EXIT_FAILURE);
	}else{
		do{
			wpid=waitpid(pid,&status,WUNTRACED);
		}while(!WIFEXITED(status)&&!WIFSIGNALED(status));
	}


	return 1;
}

