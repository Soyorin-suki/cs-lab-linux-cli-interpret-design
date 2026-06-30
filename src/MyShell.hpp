#pragma once
#include <string>
#include <vector>
#include "parser/AST.hpp"

/**
 * @brief 命令行解释器主类
 *
 * 负责 Shell 的主循环 (REPL)、AST 执行调度、以及系统调用封装。
 * 依赖 Parser（词法/语法分析）、Builtin（内置命令）、Alias（别名）、History（历史记录）。
 */
class MyShell {
private:
	/**
	 * @brief 读取一行输入，同时写入历史记录
	 * @return 不含换行符的原始字符串
	 */
	std::string read_line();

	/**
	 * @brief 递归遍历 AST 并执行
	 * @param node AST 根节点（可为 nullptr 表示空输入）
	 * @return -1 → 退出 shell；0~255 → 当前命令的退出码，shell 继续运行
	 *
	 * 根据 node->type 分发到不同的执行分支：
	 *   CMD → execute_cmd()
	 *   AND → 左边成功才执行右边（短路与）
	 *   OR  → 左边失败才执行右边（短路或）
	 *   PIPE → 暂未实现
	 */
	int execute_ast(const parser::ASTNode* node);

	/**
	 * @brief 执行单个简单命令（CMD 节点）
	 * @param node CMD 类型的 AST 节点
	 * @return -1 → 收到 exit 命令；0~255 → 命令的退出码（0=成功，非0=失败）
	 *
	 * 执行流程：
	 *   1. 检查重定向/后台（暂未实现则打印提示）
	 *   2. 构建参数列表，应用别名替换
	 *   3. 尝试匹配内置命令
	 *   4. 若非内置，则 fork() + execvp() 执行外部程序
	 *   5. 父进程 waitpid() 等待子进程结束，返回其退出码
	 */
	int execute_cmd(const parser::ASTNode* node);

	/**
	 * @brief 获取当前工作目录的绝对路径
	 * @details 封装 getcwd() 系统调用，缓冲区 4096 字节
	 */
	std::string get_pwd();

	/**
	 * @brief 生成命令提示符
	 * @return 格式为 "[user-sh]/current/path$ " 的字符串
	 */
	std::string get_prompt();

public:
	/// 初始化内置命令注册表与别名表
	void init();
	MyShell();
	~MyShell();

	/**
	 * @brief 启动 REPL 主循环
	 *
	 * 循环执行：打印提示符 → 读取一行 → Parser::parse() 构建 AST → execute_ast() 执行
	 * 当 execute_ast 返回 -1 时退出循环。
	 */
	void loop();
};
