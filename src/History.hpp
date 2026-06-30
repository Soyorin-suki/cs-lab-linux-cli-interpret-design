#pragma once
#include <vector>
#include <string>

/**
 * @brief 命令历史记录
 *
 * 使用静态 std::vector<std::string> 存储本次会话中所有输入行。
 * 每次 read_line() 时自动 push，内置命令 history 调用 get_history() 展示。
 *
 * @note 当前仅内存存储，退出 shell 后历史丢失。
 *       改进方向：持久化到 ~/.user-sh_history 文件。
 */
class History {
private:
	/// 静态存储：所有输入过的命令行（按时间顺序）
	static std::vector<std::string> history;

public:
	/// 追加一条历史记录（在 MyShell::read_line() 中自动调用）
	static void push(const std::string& line);

	/// 返回完整历史记录的只读副本
	static std::vector<std::string> get_history();
};