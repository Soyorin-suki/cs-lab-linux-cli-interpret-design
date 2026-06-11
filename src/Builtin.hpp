#pragma once
#include <functional>
#include <unordered_map>
#include <string>


using Builtin_Func = std::function<int(const std::vector<std::string>&)>;



class Builtin{
private:
	static std::unordered_map<std::string,Builtin_Func>commands;
public:
	/**
	 * @brief 根据传入的`func_name`查找对应注册的函数并执行
	 * @return 返回`0`表示正常执行结束; 返回`1`表示应该退出; 返回`2`表示遇到了未知的错误; 返回`-1`表示未查找到
	 */
	static int execute(const std::string &func_name, const std::vector<std::string>&args);
	static void init();
	
};

int buildin_cd(const std::vector<std::string>&args);
int buildin_exit(const std::vector<std::string>&args);
