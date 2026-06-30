#include "Builtin.hpp"
#include "History.hpp"
#include "Alias.hpp"
#include <unistd.h>
#include <iostream>

// ─── 静态成员定义 ───────────────────────────────────────────────

/// 内置命令注册表（静态成员变量，全局唯一）
std::unordered_map<std::string, Builtin_Func> Builtin::commands;

// ─── init()：注册所有内置命令 ───────────────────────────────────

void Builtin::init() {
	commands["cd"]      = builtin_cd;
	commands["exit"]    = builtin_exit;
	commands["help"]    = builtin_help;
	commands["history"] = builtin_history;
}

// ─── builtin_cd ─────────────────────────────────────────────────
// 返回值：0=成功, 1=exit, 2=参数错误或路径无效

int builtin_cd(const std::vector<std::string>& args) {
	if (args.size() < 2) {
		std::cerr << "cd: 参数过少!\n";
		return 2;
	}
	// chdir() 是 POSIX 系统调用，修改当前进程的工作目录
	// 成功返回 0，失败返回 -1 并设置 errno
	if (chdir(args[1].c_str()) != 0) {
		perror("cd");    // perror 会打印 errno 对应的错误描述
		return 2;
	}
	return 0;
}

// ─── builtin_exit ───────────────────────────────────────────────
// 返回值 1 让 MyShell::execute_cmd 转化为 -1，通知主循环退出

int builtin_exit(const std::vector<std::string>& args) {
	return 1;
}

// ─── builtin_help ───────────────────────────────────────────────

int builtin_help(const std::vector<std::string>& args) {
	std::cout << "这是soyorin-suki制作的whut课设: mini-linux-shell\n";
	std::cout << "目前支持以下功能：\n";
	std::cout << "1. 外部命令调用\n";
	std::cout << "2. 内部builtin命令调用\n";
	std::cout << "3. 管道功能\n";
	std::cout << "4. 逻辑AND和逻辑OR\n";
	std::cout << "键入 exit 退出\n";
	return 0;
}

// ─── builtin_history ────────────────────────────────────────────
// 调用 History::get_history() 获取所有历史记录并带编号打印

int builtin_history(const std::vector<std::string>& args) {
	auto history = History::get_history();
	for (size_t i = 0; i < history.size(); ++i) {
		std::cout << "\t" << i + 1 << "\t" << history[i] << '\n';
	}
	return 0;
}

// ─── Builtin::execute()：命令调度入口 ───────────────────────────
// 在 commands 表中查找命令名，找到则调用对应函数，否则返回 -1

int Builtin::execute(const std::string& func_name, const std::vector<std::string>& args) {
	auto func = commands.find(func_name);
	if (func == commands.end()) {
		return -1;  // 未找到 → 由调用者尝试外部命令
	}
	return commands[func_name](args);
}