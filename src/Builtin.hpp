#pragma once
#include <functional>
#include <unordered_map>
#include <string>

/**
 * @brief 内置命令函数类型
 *
 * 所有内置命令遵循统一签名：
 *   - 参数 args: args[0] = 命令名, args[1..] = 参数
 *   - 返回值: 0=成功, 1=退出shell, 2=错误, -1=未找到（仅 Builtin::execute 使用）
 */
using Builtin_Func = std::function<int(const std::vector<std::string>&)>;

/**
 * @brief 内置命令注册与调度
 *
 * 使用静态的 unordered_map 存储「命令名 → 处理函数」的映射。
 * init() 注册所有内置命令，execute() 根据命令名查找并调用。
 */
class Builtin {
private:
	/// 命令注册表：命令名字符串 → 处理函数
	static std::unordered_map<std::string, Builtin_Func> commands;

public:
	/**
	 * @brief 根据命令名查找注册函数并执行
	 * @param func_name 命令名（如 "cd", "exit"）
	 * @param args      完整参数列表（args[0] == func_name）
	 * @return 0=正常执行, 1=应退出shell, 2=遇到错误, -1=未找到该命令
	 */
	static int execute(const std::string& func_name, const std::vector<std::string>& args);

	/// 注册所有内置命令（在 MyShell::init() 中调用）
	static void init();
};

// ─── 内置命令函数声明 ────────────────────────────────────────

/// cd <路径>：切换当前工作目录（调用 chdir() 系统调用）
int builtin_cd(const std::vector<std::string>& args);

/// exit：退出 user-sh 解释器
int builtin_exit(const std::vector<std::string>& args);

/// help：打印帮助信息
int builtin_help(const std::vector<std::string>& args);

/// history：显示本次会话中输入过的所有命令
int builtin_history(const std::vector<std::string>& args);
