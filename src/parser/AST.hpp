#pragma once
#include <string>
#include <vector>
#include <memory>

namespace parser {

/**
 * @brief AST 节点类型
 *
 * 对应文法中的四种语法结构：
 *   CMD  — 简单命令
 *   PIPE — 管道 |
 *   AND  — 逻辑与 &&
 *   OR   — 逻辑或 ||
 */
enum class ASTType {
    CMD,
    PIPE,
    AND,
    OR,
};

/**
 * @brief AST 节点
 *
 * - CMD 节点：使用 cmd_name / redirect_in / redirect_out / append_out / args / background 字段
 * - PIPE / AND / OR 节点：使用 left / right 字段
 */
struct ASTNode {
    ASTType type;

    // ─── CMD 节点专用字段 ───
    std::string cmd_name;             // 命令名
    std::string redirect_in;          // 输入重定向文件名，"" 表示无
    std::string redirect_out;         // 输出重定向文件名，"" 表示无
    bool append_out = false;          // true → >> 追加，false → > 覆盖
    std::vector<std::string> args;    // 命令参数
    bool background = false;          // true → 后台运行 &

    // ─── PIPE / AND / OR 节点专用字段 ───
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    // ─── 工厂方法 ────────────────────────

    /** 创建 CMD 节点 */
    static std::unique_ptr<ASTNode> make_cmd(
        const std::string& name,
        const std::string& in_file,
        const std::string& out_file,
        bool append,
        const std::vector<std::string>& arg_list,
        bool bg
    );

    /** 创建 PIPE / AND / OR 二元节点 */
    static std::unique_ptr<ASTNode> make_binary(
        ASTType t,
        std::unique_ptr<ASTNode> l,
        std::unique_ptr<ASTNode> r
    );
};

} // namespace parser
