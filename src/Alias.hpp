#pragma once
#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief 命令别名系统
 *
 * 使用静态 unordered_map 存储「别名 → 展开后的参数列表」映射。
 * 例如：l → {"ls", "--color=auto"}，则用户输入 l 等价于 ls --color=auto。
 *
 * Alias::replace() 在 MyShell::execute_cmd() 中、执行内置命令之前调用，
 * 仅替换命令名（args[0]），不修改后续参数。
 */
class Alias {
private:
	/// 别名映射表：原始命令名 → 展开后的完整参数列表
	static std::unordered_map<std::string, std::vector<std::string>> replace_map;

public:
	/// 初始化默认别名（在 MyShell::init() 中调用）
	static void init();

	/**
	 * @brief 对参数列表的 0 号位置进行别名展开
	 * @param argv 原始参数列表（argv[0] 为命令名）
	 * @return 若 argv[0] 命中别名则返回展开后的新列表；否则原样返回
	 */
	static std::vector<std::string> replace(const std::vector<std::string>& argv);
};