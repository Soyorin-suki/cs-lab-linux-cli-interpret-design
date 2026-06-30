#include "Alias.hpp"

/// 静态成员变量定义
std::unordered_map<std::string, std::vector<std::string>> Alias::replace_map;

/**
 * @brief 注册默认别名
 *
 * l   → ls --color=auto  （短别名）
 * ls  → ls --color=auto  （为 ls 默认开启彩色输出）
 */
void Alias::init() {
	replace_map["ls"] = {"ls", "--color=auto"};
	replace_map["l"]  = {"ls", "--color=auto"};
}

/**
 * @brief 查找并执行别名替换
 *
 * 查找逻辑：仅检查 argv[0]（命令名）是否命中别名表。
 * 若命中，返回「展开后的别名参数 + 原始额外参数」合并后的新列表；
 * 若未命中，原样返回。
 *
 * 示例：
 *   输入 {"l", "-la"} → 输出 {"ls", "--color=auto", "-la"}
 *   输入 {"gcc", "main.c"} → 输出 {"gcc", "main.c"}（未命中，原样）
 */
std::vector<std::string> Alias::replace(const std::vector<std::string>& argv) {
	if (argv.empty()) return argv;

	auto it = replace_map.find(argv[0]);
	if (it == replace_map.end()) {
		// 未命中别名 → 原样返回
		return argv;
	}

	// 命中别名 → 以别名展开后的列表为基础，追加上原始额外参数
	std::vector<std::string> expanded;
	for (const auto& alias_arg : it->second) {
		expanded.push_back(alias_arg);
	}
	for (size_t i = 1; i < argv.size(); ++i) {
		expanded.push_back(argv[i]);
	}
	return expanded;
}