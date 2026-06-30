#pragma once
#include "Token.hpp"
#include "AST.hpp"
#include <string>
#include <memory>
#include <utility>

namespace parser {

/**
 * @brief 命令行解析器
 *
 * 封装了词法分析和语法分析两个阶段：
 *   - Lexer         词法分析器：字符序列 → Token 序列
 *   - SyntaxAnalyzer 语法分析器：Token 序列 → AST
 *
 * 对外统一入口：Parser::parse(input)
 *
 * 用法示例：
 * @code
 *   auto ast = parser::Parser::parse("ls -la | grep foo");
 *   // ast 指向一棵完整的 AST 语法树
 * @endcode
 */
class Parser {
public:
    // ========================================================================
    //  Lexer — 词法分析器
    // ========================================================================

    /**
     * @brief 词法分析器
     *
     * 将输入字符串按顺序切分为 Token 序列。
     * 支持的操作符识别规则（按优先级匹配）：
     *   - ">>" → REDIR_APPEND
     *   - ">"  → REDIR_OUT
     *   - "<"  → REDIR_IN
     *   - "||" → OR
     *   - "|"  → PIPE
     *   - "&&" → AND
     *   - "&"  → BG
     *   - 其余连续非空白字符 → STRING
     */
    class Lexer {
    public:
        /**
         * @param input 原始输入字符串（不包含末尾换行符）
         */
        explicit Lexer(const std::string& input);

        /// 返回当前 lookahead 指向的 Token，不消耗
        Token peek() const;

        /// 消耗当前 Token 并返回它，随后读入下一个 Token
        Token next();

        /// 是否还有 Token（未到达 END）
        bool has_next() const;

    private:
        std::string input_;       // 原始输入
        size_t      pos_ = 0;     // 当前扫描位置（字符索引）
        Token       current_;     // 已预读的当前 Token
        bool        consumed_ = false;  // current_ 是否已被 next() 消费

        /// 从当前位置读取一个 Token 并存入 current_
        void load_next();

        /// 跳过空白字符
        void skip_whitespace();

        /// 读取一个 STRING Token（直到遇到空白或操作符）
        Token read_string();

        /// 输入当前位置字符，越界返回 '\0'
        char cur_char() const;
    };

    // ========================================================================
    //  SyntaxAnalyzer — 语法分析器（递归下降）
    // ========================================================================

    /**
     * @brief 语法分析器
     *
     * 基于 docs/syntax.md 中定义的 LL(1) 文法实现递归下降解析。
     * 内部维护一个 lookahead Token，通过 match() 消费并进入下一个 Token。
     *
     * 文法层级（自顶向下）：
     *   cmdline → cmdline_rest → pipeline → pipeline_rest → command
     */
    class SyntaxAnalyzer {
    public:
        /**
         * @param lexer 词法分析器引用
         */
        explicit SyntaxAnalyzer(Lexer& lexer);

        /// 入口：解析整个命令行，返回 AST 根节点
        std::unique_ptr<ASTNode> parse();

    private:
        Lexer& lexer_;
        Token  lookahead_;

        /// 消耗 lookahead_ 并加载下一个 Token
        void advance();

        /// 断言 lookahead_ 类型为 expected 并消耗之；不匹配则报错
        void match(TokenType expected);

        // ─── 递归下降解析函数（一一对应 LL(1) 文法的每个非终结符）───

        std::unique_ptr<ASTNode> parse_cmdline();
        std::unique_ptr<ASTNode> parse_cmdline_rest(std::unique_ptr<ASTNode> left);
        std::unique_ptr<ASTNode> parse_pipeline();
        std::unique_ptr<ASTNode> parse_pipeline_rest(std::unique_ptr<ASTNode> left);
        std::unique_ptr<ASTNode> parse_command();
        std::string              parse_redir_in();
        std::pair<std::string, bool> parse_redir_out();  // {filename, is_append}
        std::vector<std::string> parse_args();
        bool                     parse_bg();
    };

    // ========================================================================
    //  对外统一入口
    // ========================================================================

    /**
     * @brief 一站式解析
     *
     * @param input 原始命令行字符串
     * @return AST 语法树的根节点；输入为空时返回 nullptr
     *
     * 等价于：
     * @code
     *   Lexer lexer(input);
     *   SyntaxAnalyzer sa(lexer);
     *   return sa.parse();
     * @endcode
     */
    static std::unique_ptr<ASTNode> parse(const std::string& input);
};

} // namespace parser
