#pragma once
#include <string>

namespace parser {

/**
 * @brief 词法分析器输出的 Token 类型
 *
 * 对应 docs/syntax.md 中定义的 9 种 Token：
 *   STRING       — 命令名 / 参数 / 文件名
 *   PIPE         — |
 *   REDIR_IN     — <
 *   REDIR_OUT    — >
 *   REDIR_APPEND — >>
 *   AND          — &&
 *   OR           — ||
 *   BG           — &（后台运行）
 *   END          — 输入结束标记
 */
enum class TokenType {
    STRING,         // 命令名 / 参数 / 文件名
    PIPE,           // |
    REDIR_IN,       // <
    REDIR_OUT,      // >
    REDIR_APPEND,   // >>
    AND,            // &&
    OR,             // ||
    BG,             // &
    END,            // 输入结束（EOF）
};

/**
 * @brief Token 结构体
 *
 * @field type  Token 类型
 * @field value 词素值（仅 STRING 类型有意义；运算符类型的 value 为其符号字面量，如 ">>"）
 */
struct Token {
    TokenType type;
    std::string value;
};

} // namespace parser
