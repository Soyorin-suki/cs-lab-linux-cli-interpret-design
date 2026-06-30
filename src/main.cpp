/**
 * @file main.cpp
 * @brief user-sh 命令行解释器入口
 *
 * 武汉理工大学计算机实训课设 —— Linux 命令行解释器设计
 *
 * 架构概要：
 *   1. MyShell::init() → 初始化内置命令注册表 & 别名表
 *   2. MyShell::loop() → REPL 主循环
 *      ├── Parser::parse()   词法 + LL(1) 语法分析 → AST
 *      ├── execute_ast()     递归遍历 AST 执行
 *      │   ├── CMD  → execute_cmd() → 内置命令 / fork+exec
 *      │   ├── AND  → 逻辑与（短路求值）
 *      │   ├── OR   → 逻辑或（短路求值）
 *      │   └── PIPE → 暂未实现
 *      └── History::push()   历史记录
 */

#include "MyShell.hpp"

int main(int argc, char** argv) {
	MyShell shell;
	shell.init();
	shell.loop();
	return 0;
}
