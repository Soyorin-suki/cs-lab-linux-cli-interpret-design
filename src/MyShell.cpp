#include "MyShell.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "History.hpp"
#include "Builtin.hpp"
#include "Alias.hpp"
#include "parser/Parser.hpp"
using std::string;
using std::vector;


MyShell::MyShell(){}
MyShell::~MyShell(){}

/**
 * 初始化
 * 
 * 同时初始化`builtin`注册表与`Alias`注册表
 */
void MyShell::init(){
	Builtin::init();
	Alias::init();
}

/**
 * 获取当前工作目录
 * 
 */
string MyShell::get_pwd(){
	char buf[4096];
	getcwd(buf,sizeof(buf));
	string pwd(buf);
	return pwd;
}

/**
 * 获取当前 prompt 用作提示
 */
string MyShell::get_prompt(){
	string prompt;
	prompt+="[user-sh]";
	prompt+=get_pwd();
	prompt+="$ ";
	return prompt;
}

/**
 * shell 主循环
 * 
 * 每次 read 一行并进行构建 ast 和递归执行
 * 
 */
void MyShell::loop(){
	string line;
	int status;

	do {
		std::cout<<get_prompt();
		line=read_line();
		auto ast = parser::Parser::parse(line);
		status = execute_ast(ast.get());
	}while(status);
}

const int BUF_MAX_SIZE = 1024;

/**
 * 读取一行，同时将其加入 History
 * 
 */
string MyShell::read_line(){
	string line;
	std::getline(std::cin, line);
	History::push(line);
	return line;
}

// ============================================================================
//  AST 执行 — 递归遍历语法树
// ============================================================================

int MyShell::execute_ast(const parser::ASTNode* node){
	if(!node) return 1;  // 空输入，继续

	switch(node->type){
	case parser::ASTType::CMD:
		return execute_cmd(node);

	case parser::ASTType::PIPE:
		std::cout << "暂未实现：管道 (|) 功能\n";
		return 1;

	case parser::ASTType::AND:
		std::cout << "暂未实现：逻辑与 (&&) 功能\n";
		return 1;

	case parser::ASTType::OR:
		std::cout << "暂未实现：逻辑或 (||) 功能\n";
		return 1;
	}
	return 1;
}

int MyShell::execute_cmd(const parser::ASTNode* node){
	// ── 未实现的重定向功能 ──
	if(!node->redirect_in.empty() || !node->redirect_out.empty()){
		std::cout << "暂未实现：重定向 (< > >>) 功能\n";
		return 1;
	}
	// ── 未实现的后台运行功能 ──
	if(node->background){
		std::cout << "暂未实现：后台运行 (&) 功能\n";
		return 1;
	}

	// 构建完整参数列表: {命令名, 参数1, 参数2, ...}
	vector<string> full_args;
	full_args.push_back(node->cmd_name);
	for(const auto& arg : node->args){
		full_args.push_back(arg);
	}

	// 别名替换
	full_args = Alias::replace(full_args);

	// 尝试执行内置命令
	int status;
	status = Builtin::execute(full_args[0], full_args);
	if(status == 0){
		return 1;   // 内置命令执行成功，继续
	}else if(status == 1){
		return 0;   // exit 命令，退出
	}else if(status == 2){
		return 1;   // 内置命令错误，继续
	}
	// status == -1: 非内置命令，fallthrough 到外部命令

	// 构建 argv
	vector<char*> argv;
	for(auto& arg : full_args){
		argv.push_back(const_cast<char*>(arg.c_str()));
	}
	argv.push_back(nullptr);

	pid_t pid = fork();
	if(pid < 0){
		perror("MyShell启动失败");
	}else if(pid == 0){
		// 子进程
		if(execvp(argv[0], argv.data()) == -1){
			perror("MyShell");
		}
		exit(EXIT_FAILURE);
	}else{
		// 父进程
		int wstatus;
		pid_t wpid;
		do{
			wpid = waitpid(pid, &wstatus, WUNTRACED);
		}while(!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
	}

	return 1;
}

