# parser 模块设计文档

## 模块职责

将用户输入的命令行字符串解析为 AST 语法树，分为两个阶段：

```
输入字符串  ──[Lexer]──▶  Token 序列  ──[SyntaxAnalyzer]──▶  AST 根节点
```

---

## 文件结构

```
src/parser/
├── Token.hpp      Token 类型定义
├── AST.hpp        AST 节点定义
├── Parser.hpp     Parser 类声明（含 Lexer 与 SyntaxAnalyzer）
└── Parser.cpp     实现
```

---

## 类关系

```
┌─────────────────────────────────────────┐
│               Parser                     │
│  ┌─────────────────────────────────────┐│
│  │           Lexer                      ││
│  │  - input_: string                    ││
│  │  - pos_: size_t                      ││
│  │  + peek() → Token                    ││
│  │  + next() → Token                    ││
│  └──────────────┬──────────────────────┘│
│                 │  Token 序列             │
│  ┌──────────────▼──────────────────────┐│
│  │        SyntaxAnalyzer                ││
│  │  - lexer_: Lexer&                   ││
│  │  + parse() → unique_ptr<ASTNode>     ││
│  │  - parse_cmdline()                   ││
│  │  - parse_pipeline()                  ││
│  │  - parse_command()                   ││
│  │  - ...                               ││
│  └─────────────────────────────────────┘│
│                                          │
│  + parse(input) → unique_ptr<ASTNode>    │
└─────────────────────────────────────────┘
```

- **`Parser`** 是外观类，对外只暴露一个静态方法 `parse(input)`。
- **`Lexer`** 是 `Parser` 的内部类，负责词法分析。
- **`SyntaxAnalyzer`** 是 `Parser` 的内部类，负责语法分析（递归下降）。

---

## 词法分析（Lexer）

### 扫描策略

从头到尾扫描输入字符串，识别多字符操作符优先级高于单字符：

| 扫描到的字符 | 前瞻 1 字符 | Token 类型 |
|-------------|------------|-----------|
| `>` + `>` | → | `REDIR_APPEND` `>>` |
| `>` | | `REDIR_OUT` `>` |
| `<` | | `REDIR_IN` `<` |
| `\|` + `\|` | → | `OR` `\|\|` |
| `\|` | | `PIPE` `\|` |
| `&` + `&` | → | `AND` `&&` |
| `&` | | `BG` `&` |
| 其他字符 | | 连续读取直到空白/操作符 → `STRING` |

### 空白处理

空格、制表符 `\t` 被跳过，不作为 Token 输出。

### Token 缓存

`Lexer` 内部维护一个 `current_` Token 作为单 Token 前瞻缓存。`peek()` 返回当前 Token 不消耗，`next()` 返回并加载下一个。`consumed_` 标志防止重复加载。

---

## 语法分析（SyntaxAnalyzer）

### 文法依据

严格遵循 `docs/syntax.md` 中定义的 LL(1) 文法，每个非终结符对应一个解析函数：

| 非终结符 | 解析函数 | 对应文法 |
|---------|---------|---------|
| `<cmdline>` | `parse_cmdline()` | `pipeline cmdline_rest` |
| `<cmdline_rest>` | `parse_cmdline_rest(left)` | `&& pipeline ...` / `\|\| pipeline ...` / `ε` |
| `<pipeline>` | `parse_pipeline()` | `command pipeline_rest` |
| `<pipeline_rest>` | `parse_pipeline_rest(left)` | `\| command ...` / `ε` |
| `<command>` | `parse_command()` | `STRING redir_in redir_out args bg` |
| `<redir_in>` | `parse_redir_in()` | `< STRING` / `ε` |
| `<redir_out>` | `parse_redir_out()` | `> STRING` / `>> STRING` / `ε` |
| `<args>` | `parse_args()` | `STRING args` / `ε` |
| `<bg>` | `parse_bg()` | `&` / `ε` |

### 递归下降模式

`cmdline_rest`、`pipeline_rest` 采用**传递左子树**的模式：

```
cmdline_rest(AST left):
    if lookahead == AND:
        right = pipeline()
        node = AND(left, right)
        return cmdline_rest(node)   // 左结合
    elif lookahead == OR:
        ...同理...
    else:
        return left                  // ε
```

这样 `A && B && C` 被解析为 `AND(AND(A, B), C)`（左结合）。

### 错误处理

当前为简单实现：遇到 Token 类型不匹配时打印错误信息到 `stderr`，并跳过当前 Token 继续解析（容错模式），不会崩溃。

---

## AST 节点

### 节点类型

```
CMD 节点（叶子）              二元节点（内部节点）
┌──────────────────┐         ┌─────────────┐
│ type = CMD       │         │ type = PIPE  │
│ cmd_name         │         │     / AND    │
│ redirect_in      │         │     / OR     │
│ redirect_out     │         │ left  ───────┼──▶ ASTNode*
│ append_out       │         │ right ───────┼──▶ ASTNode*
│ args[]           │         └─────────────┘
│ background       │
└──────────────────┘
```

### 示例

输入 `ls -la | grep foo`：

```
         PIPE
        /    \
     CMD      CMD
    (ls)     (grep)
   [-la]     [foo]
```

输入 `gcc main.c && ./a.out`：

```
         AND
        /    \
     CMD      CMD
    (gcc)   (./a.out)
  [main.c]    []
```

---

## 对外使用方式

```cpp
#include "parser/Parser.hpp"

std::string line = "ls -la | grep foo";
auto ast = parser::Parser::parse(line);

if (ast) {
    // ast 是 AST 语法树的根节点
    // 可通过 ast->type, ast->left, ast->right 遍历
}
```

---

## 当前限制

1. 不支持引号（`"..."` 或 `'...'`），所有内容按空白和运算符分割
2. 重定向必须紧跟在命令名后、参数前（文法约定的简化约束）
3. 输入 `<` 必须在输出 `>` 之前
4. 后台 `&` 必须在命令末尾
5. 转义字符（如 `\ `）暂不支持
6. 错误恢复仅做简单跳过，不保证后续解析完全正确
