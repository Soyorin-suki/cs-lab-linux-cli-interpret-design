#pragma once
#include <string>
#include <vector>
#include <memory>

namespace parser {

/**
 * @brief AST 节点类型
 *
 * 对应 LL(1) 文法中的四种语法结构：
 *   CMD  — 简单命令（叶子节点）
 *   PIPE — 管道 |（二元内部节点）
 *   AND  — 逻辑与 &&（二元内部节点，短路求值）
 *   OR   — 逻辑或 ||（二元内部节点，短路求值）
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
 * - CMD 节点（叶子）：使用 cmd_name / redirect_in / redirect_out / append_out / args / background
 * - PIPE / AND / OR 节点（内部节点）：使用 left / right
 *
 * 内存管理：使用 std::unique_ptr 表达所有权，自动析构。
 */
struct ASTNode {
    ASTType type;

    // ─── CMD 节点专用字段 ───
    std::string cmd_name;             // 命令名（如 "ls", "gcc"）
    std::string redirect_in;          // 输入重定向文件名，"" 表示无
    std::string redirect_out;         // 输出重定向文件名，"" 表示无
    bool append_out = false;          // true → >> 追加写入；false → > 覆盖写入
    std::vector<std::string> args;    // 命令参数（不包含命令名本身）
    bool background = false;          // true → 后台运行 &

    // ─── PIPE / AND / OR 节点专用字段 ───
    std::unique_ptr<ASTNode> left;    // 左子节点
    std::unique_ptr<ASTNode> right;   // 右子节点

    // ─── 工厂方法 ────────────────────────────────────────────────

    /**
     * @brief 创建 CMD 叶子节点
     * @param name      命令名
     * @param in_file   输入重定向文件名（空字符串=无）
     * @param out_file  输出重定向文件名（空字符串=无）
     * @param append    输出是否为追加模式（>>）
     * @param arg_list  参数列表（不含命令名）
     * @param bg        是否后台运行
     */
    static std::unique_ptr<ASTNode> make_cmd(
        const std::string& name,
        const std::string& in_file,
        const std::string& out_file,
        bool append,
        const std::vector<std::string>& arg_list,
        bool bg
    );

    /**
     * @brief 创建二元内部节点（PIPE / AND / OR）
     * @param t   节点类型（PIPE、AND 或 OR）
     * @param l   左子树（所有权转移）
     * @param r   右子树（所有权转移）
     */
    static std::unique_ptr<ASTNode> make_binary(
        ASTType t,
        std::unique_ptr<ASTNode> l,
        std::unique_ptr<ASTNode> r
    );
};

} // namespace parser
