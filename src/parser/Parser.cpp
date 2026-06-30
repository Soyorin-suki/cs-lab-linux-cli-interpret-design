#include "Parser.hpp"
#include <iostream>
#include <cctype>

namespace parser {

// ============================================================================
//  ASTNode 工厂方法
// ============================================================================

std::unique_ptr<ASTNode> ASTNode::make_cmd(
    const std::string& name,
    const std::string& in_file,
    const std::string& out_file,
    bool append,
    const std::vector<std::string>& arg_list,
    bool bg)
{
    auto node = std::make_unique<ASTNode>();
    node->type         = ASTType::CMD;
    node->cmd_name     = name;
    node->redirect_in  = in_file;
    node->redirect_out = out_file;
    node->append_out   = append;
    node->args         = arg_list;
    node->background   = bg;
    return node;
}

std::unique_ptr<ASTNode> ASTNode::make_binary(
    ASTType t,
    std::unique_ptr<ASTNode> l,
    std::unique_ptr<ASTNode> r)
{
    auto node = std::make_unique<ASTNode>();
    node->type  = t;
    node->left  = std::move(l);
    node->right = std::move(r);
    return node;
}

// ============================================================================
//  Parser::Lexer 实现
// ============================================================================

Parser::Lexer::Lexer(const std::string& input)
    : input_(input)
    , pos_(0)
    , consumed_(true)        // 标记为已消费，迫使首次 next/peek 时加载
{
    // 如果输入非空，预读第一个 Token
    if (!input_.empty()) {
        load_next();
        consumed_ = false;
    } else {
        current_ = Token{TokenType::END, ""};
    }
}

Token Parser::Lexer::peek() const {
    return current_;
}

Token Parser::Lexer::next() {
    Token result = current_;
    consumed_ = true;
    load_next();
    consumed_ = false;
    return result;
}

bool Parser::Lexer::has_next() const {
    return current_.type != TokenType::END;
}

char Parser::Lexer::cur_char() const {
    if (pos_ < input_.size()) {
        return input_[pos_];
    }
    return '\0';
}

void Parser::Lexer::skip_whitespace() {
    while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
        ++pos_;
    }
}

void Parser::Lexer::load_next() {
    if (!consumed_) return;   // 还没消费，缓存仍有效
    skip_whitespace();

    if (pos_ >= input_.size()) {
        current_ = Token{TokenType::END, ""};
        return;
    }

    char c = input_[pos_];

    // ── 多字符操作符（需前瞻 1 字符） ──
    if (c == '>') {
        if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '>') {
            current_ = Token{TokenType::REDIR_APPEND, ">>"};
            pos_ += 2;
            return;
        }
        current_ = Token{TokenType::REDIR_OUT, ">"};
        ++pos_;
        return;
    }

    if (c == '|') {
        if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '|') {
            current_ = Token{TokenType::OR, "||"};
            pos_ += 2;
            return;
        }
        current_ = Token{TokenType::PIPE, "|"};
        ++pos_;
        return;
    }

    if (c == '&') {
        if (pos_ + 1 < input_.size() && input_[pos_ + 1] == '&') {
            current_ = Token{TokenType::AND, "&&"};
            pos_ += 2;
            return;
        }
        current_ = Token{TokenType::BG, "&"};
        ++pos_;
        return;
    }

    // ── 单字符操作符 ──
    if (c == '<') {
        current_ = Token{TokenType::REDIR_IN, "<"};
        ++pos_;
        return;
    }

    // ── 普通字符串 ──
    current_ = read_string();
}

Token Parser::Lexer::read_string() {
    std::string value;
    while (pos_ < input_.size()) {
        char c = input_[pos_];
        // 遇到空白或操作符则停止
        if (std::isspace(static_cast<unsigned char>(c))
            || c == '|' || c == '<' || c == '>' || c == '&')
        {
            break;
        }
        value += c;
        ++pos_;
    }
    return Token{TokenType::STRING, value};
}

// ============================================================================
//  Parser::SyntaxAnalyzer 实现
// ============================================================================

Parser::SyntaxAnalyzer::SyntaxAnalyzer(Parser::Lexer& lexer)
    : lexer_(lexer)
{
    // 预读第一个 Token
    lookahead_ = lexer_.next();
}

void Parser::SyntaxAnalyzer::advance() {
    lookahead_ = lexer_.next();
}

void Parser::SyntaxAnalyzer::match(TokenType expected) {
    if (lookahead_.type == expected) {
        advance();
    } else {
        std::cerr << "[语法错误] 期望 Token 类型 " << static_cast<int>(expected)
                  << "，实际为 " << static_cast<int>(lookahead_.type)
                  << " (\"" << lookahead_.value << "\")\n";
        // 容错：跳过当前 Token 继续
        advance();
    }
}

// ─── 入口 ─────────────────────────────────────────────────────────────────

std::unique_ptr<ASTNode> Parser::SyntaxAnalyzer::parse() {
    if (lookahead_.type == TokenType::END) {
        return nullptr;   // 空输入
    }
    return parse_cmdline();
}

// ─── cmdline ::= pipeline cmdline_rest ─────────────────────────────────────

std::unique_ptr<ASTNode> Parser::SyntaxAnalyzer::parse_cmdline() {
    auto left = parse_pipeline();
    return parse_cmdline_rest(std::move(left));
}

// ─── cmdline_rest ::= "&&" pipeline cmdline_rest
//                    | "||" pipeline cmdline_rest
//                    | ε ────────────────────────────────────────────────────

std::unique_ptr<ASTNode> Parser::SyntaxAnalyzer::parse_cmdline_rest(std::unique_ptr<ASTNode> left) {
    if (lookahead_.type == TokenType::AND) {
        match(TokenType::AND);
        auto right = parse_pipeline();
        auto node = ASTNode::make_binary(ASTType::AND, std::move(left), std::move(right));
        return parse_cmdline_rest(std::move(node));
    }

    if (lookahead_.type == TokenType::OR) {
        match(TokenType::OR);
        auto right = parse_pipeline();
        auto node = ASTNode::make_binary(ASTType::OR, std::move(left), std::move(right));
        return parse_cmdline_rest(std::move(node));
    }

    // ε: 没有更多逻辑连接符，直接返回左子树
    return left;
}

// ─── pipeline ::= command pipeline_rest ────────────────────────────────────

std::unique_ptr<ASTNode> Parser::SyntaxAnalyzer::parse_pipeline() {
    auto left = parse_command();
    return parse_pipeline_rest(std::move(left));
}

// ─── pipeline_rest ::= "|" command pipeline_rest
//                     | ε ───────────────────────────────────────────────────

std::unique_ptr<ASTNode> Parser::SyntaxAnalyzer::parse_pipeline_rest(std::unique_ptr<ASTNode> left) {
    if (lookahead_.type == TokenType::PIPE) {
        match(TokenType::PIPE);
        auto right = parse_command();
        auto node = ASTNode::make_binary(ASTType::PIPE, std::move(left), std::move(right));
        return parse_pipeline_rest(std::move(node));
    }

    return left;  // ε
}

// ─── command ::= STRING redir_in redir_out args bg ─────────────────────────

std::unique_ptr<ASTNode> Parser::SyntaxAnalyzer::parse_command() {
    // 命令名
    std::string cmd_name = lookahead_.value;
    match(TokenType::STRING);

    // 重定向（可选）
    std::string in_file   = parse_redir_in();
    auto [out_file, append] = parse_redir_out();

    // 参数（可选）
    std::vector<std::string> arg_list = parse_args();

    // 后台运行（可选）
    bool is_bg = parse_bg();

    return ASTNode::make_cmd(cmd_name, in_file, out_file, append, arg_list, is_bg);
}

// ─── redir_in ::= "<" STRING | ε ───────────────────────────────────────────

std::string Parser::SyntaxAnalyzer::parse_redir_in() {
    if (lookahead_.type == TokenType::REDIR_IN) {
        match(TokenType::REDIR_IN);
        std::string file = lookahead_.value;
        match(TokenType::STRING);
        return file;
    }
    return "";  // ε
}

// ─── redir_out ::= ">" STRING | ">>" STRING | ε ───────────────────────────
//     返回 {filename, is_append}

std::pair<std::string, bool> Parser::SyntaxAnalyzer::parse_redir_out() {
    if (lookahead_.type == TokenType::REDIR_OUT) {
        match(TokenType::REDIR_OUT);
        std::string file = lookahead_.value;
        match(TokenType::STRING);
        return {file, false};   // > 覆盖
    }

    if (lookahead_.type == TokenType::REDIR_APPEND) {
        match(TokenType::REDIR_APPEND);
        std::string file = lookahead_.value;
        match(TokenType::STRING);
        return {file, true};    // >> 追加
    }

    return {"", false};  // ε
}

// ─── args ::= STRING args | ε ──────────────────────────────────────────────

std::vector<std::string> Parser::SyntaxAnalyzer::parse_args() {
    std::vector<std::string> result;
    while (lookahead_.type == TokenType::STRING) {
        result.push_back(lookahead_.value);
        match(TokenType::STRING);
    }
    return result;
}

// ─── bg ::= "&" | ε ────────────────────────────────────────────────────────

bool Parser::SyntaxAnalyzer::parse_bg() {
    if (lookahead_.type == TokenType::BG) {
        match(TokenType::BG);
        return true;
    }
    return false;  // ε
}

// ============================================================================
//  Parser 外观方法
// ============================================================================

std::unique_ptr<ASTNode> Parser::parse(const std::string& input) {
    Parser::Lexer lexer(input);
    Parser::SyntaxAnalyzer sa(lexer);
    return sa.parse();
}

} // namespace parser
