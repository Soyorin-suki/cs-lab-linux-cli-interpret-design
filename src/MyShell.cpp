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
 * @brief Shell 主循环 (REPL: Read-Eval-Print Loop)
 *
 * 每次迭代：打印提示符 → 读取一行 → 解析为 AST → 递归执行
 * 当 execute_ast 返回 -1 (即收到 exit 命令) 时终止循环。
 */
void MyShell::loop(){
	string line;
	int exit_code;

	do {
		std::cout << get_prompt();
		line = read_line();
		auto ast = parser::Parser::parse(line);
		exit_code = execute_ast(ast.get());
	} while (exit_code != -1);
}

const int BUF_MAX_SIZE = 1024;

/**
 * @brief 读取一行用户输入
 *
 * 使用 std::getline() 从标准输入读取一行（去掉末尾换行符），
 * 并立即将其存入 History 供后续 `history` 命令查询。
 *
 * @return 不含换行符的输入行
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

/**
 * @brief 递归遍历 AST 并执行
 *
 * 返回值约定（整个 shell 统一）：
 *   -1    → 退出 shell（仅 exit 内置命令触发）
 *   0     → 命令执行成功，继续 shell
 *   >0    → 命令执行失败，继续 shell（值即为退出码）
 *
 * @param node AST 根节点，nullptr 表示空输入行
 * @return 退出码或 -1
 */
int MyShell::execute_ast(const parser::ASTNode* node){
	if (!node) return 0;  // 空输入行 → 视为成功，继续

	switch (node->type) {
	// ──────── 简单命令：叶子节点 ────────
	case parser::ASTType::CMD:
		return execute_cmd(node);

	// ──────── 逻辑与 && ────────
	// 语义：左边成功 (退出码==0) 才执行右边；否则短路跳过
	case parser::ASTType::AND: {
		int left_rc = execute_ast(node->left.get());
		if (left_rc == -1) return -1;      // exit 穿透
		if (left_rc == 0) {
			return execute_ast(node->right.get());  // 左边成功 → 执行右边
		}
		return left_rc;                     // 左边失败 → 短路，不执行右边
	}

	// ──────── 逻辑或 || ────────
	// 语义：左边失败 (退出码!=0) 才执行右边；否则短路跳过
	case parser::ASTType::OR: {
		int left_rc = execute_ast(node->left.get());
		if (left_rc == -1) return -1;      // exit 穿透
		if (left_rc != 0) {
			return execute_ast(node->right.get());  // 左边失败 → 执行右边
		}
		return left_rc;                     // 左边成功 → 短路，不执行右边
	}

	// ──────── 管道：暂未实现 ────────
	case parser::ASTType::PIPE:
		std::cout << "暂未实现：管道 (|) 功能\n";
		return 1;  // 非零表示"失败"，但不退出
	}

	return 0;
}


/**
 * @brief 执行单个简单命令（CMD 节点）
 *
 * 完整流程：
 *  1. 检查是否有未实现的功能（重定向、后台）→ 有则打印提示并返回 1
 *  2. 构建完整参数列表 [{cmd_name}, {arg1}, {arg2}, ...]
 *  3. 调用 Alias::replace() 进行别名替换
 *  4. 尝试匹配内置命令表 (Builtin::execute)
 *  5. 若非内置命令：fork() 子进程 → execvp() 加载外部程序 → 父进程 waitpid() 等待
 *
 * @param node CMD 类型的 AST 节点（保证 node->type == ASTType::CMD）
 * @return -1 → exit 命令；0 → 成功；>0 → 命令的退出码
 */
int MyShell::execute_cmd(const parser::ASTNode* node){
	// ──────────── 未实现功能的占位 ────────────

	// 输入/输出重定向（<  >  >>）
	if (!node->redirect_in.empty() || !node->redirect_out.empty()) {
		std::cout << "暂未实现：重定向 (< > >>) 功能\n";
		return 1;
	}

	// 后台运行（&）
	if (node->background) {
		std::cout << "暂未实现：后台运行 (&) 功能\n";
		return 1;
	}

	// ──────────── 构建参数列表 ────────────

	// AST 中 args 不包含命令名本身，所以需要手动拼接：
	//   完整参数 = [命令名, 参数1, 参数2, ...]
	vector<string> full_args;
	full_args.push_back(node->cmd_name);
	for (const auto& arg : node->args) {
		full_args.push_back(arg);
	}

	// 别名替换（如 l → ls --color=auto）
	full_args = Alias::replace(full_args);

	// ──────────── 尝试内置命令 ────────────

	int builtin_status = Builtin::execute(full_args[0], full_args);

	if (builtin_status == 0) {
		// 内置命令执行成功（如 cd、help、history）
		return 0;
	} else if (builtin_status == 1) {
		// exit 命令 → 通知主循环退出
		return -1;
	} else if (builtin_status == 2) {
		// 内置命令执行出错（如 cd 到不存在的路径）
		return 1;
	}
	// builtin_status == -1：未找到内置命令，继续尝试外部命令

	// ──────────── 外部命令：fork() + execvp() ────────────

	// 构建 C 风格字符串数组 argv（execvp 要求 char*[] 并以 nullptr 结尾）
	vector<char*> argv;
	for (auto& arg : full_args) {
		// const_cast 是安全的：execvp 不会修改参数字符串
		argv.push_back(const_cast<char*>(arg.c_str()));
	}
	argv.push_back(nullptr);

	pid_t pid = fork();

	if (pid < 0) {
		// fork 失败（系统进程数达到上限或内存不足）
		perror("MyShell: fork 失败");
		return 1;
	}

	if (pid == 0) {
		// ============ 子进程 ============
		// execvp 会用新程序替换子进程的地址空间
		// 成功：不会返回；失败：返回 -1
		if (execvp(argv[0], argv.data()) == -1) {
			perror("MyShell");
		}
		// 执行到这里说明 execvp 失败（命令不存在等）
		exit(EXIT_FAILURE);
	}

	// ============ 父进程 ============
	// 等待子进程结束，回收其资源（防止僵尸进程）
	int wstatus;
	do {
		// WUNTRACED：如果子进程被暂停（如 SIGSTOP）也返回
		waitpid(pid, &wstatus, WUNTRACED);
	} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

	// 提取子进程的退出码并返回
	if (WIFEXITED(wstatus)) {
		return WEXITSTATUS(wstatus);   // 正常退出：返回 exit(code) 中的 code
	}
	// 子进程被信号杀死（SIGKILL、SIGSEGV 等）
	return 128 + WTERMSIG(wstatus);    // 惯例：128 + 信号编号
}

