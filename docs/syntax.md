# syntax

## 1. Token 定义

词法分析器将输入字符串切分为以下 Token 类型：

| Token 类型 | 词素 (lexeme) | 说明 |
|-----------|--------------|------|
| `STRING` | 任意不含空白和特殊符号的字符串 | 命令名、参数、文件名 |
| `PIPE` | `\|` | 管道，连接两个命令 |
| `REDIR_IN` | `<` | 输入重定向 |
| `REDIR_OUT` | `>` | 输出重定向（覆盖） |
| `REDIR_APPEND` | `>>` | 输出重定向（追加） |
| `AND` | `&&` | 逻辑与：前一个命令成功才执行后一个 |
| `OR` | `\|\|` | 逻辑或：前一个命令失败才执行后一个 |
| `BG` | `&` | 后台运行：将该命令/管道放到后台执行 |
| `EOF` | （输入结束） | 输入结束标记 |

---

## 2. 简化约束

为降低实现复杂度，做以下简化约定：

1. **重定向紧跟在命令名之后**：如果命令有重定向，`<` / `>` / `>>` 必须直接出现在命令名后面、参数前面。
   - ✅ `cmd < input > output arg1 arg2`
   - ❌ `cmd arg1 < input`

2. **输入重定向先于输出重定向**：如果同时有输入和输出重定向，必须先写输入重定向再写输出重定向。
   - ✅ `cmd < in > out`
   - ❌ `cmd > out < in`

3. **重定向仅支持三种**：`<`（输入）、`>`（覆盖输出）、`>>`（追加输出）。

4. **后台运行符必须在末尾**：`&` 只能出现在整个命令行的末尾，或管道中某个命令的末尾。
   - ✅ `cmd arg1 arg2 &`
   - ✅ `cmd1 | cmd2 &`
   - ❌ `cmd & arg1`

---

## 3. 带歧义的自然文法（BNF）

> 以下文法反映了"人脑直觉"的写法，存在左递归和二义性，**不可直接用于 LL(1) 解析**。

```bnf
<cmdline>  ::= <cmdline> "&&" <cmdline>
             | <cmdline> "||" <cmdline>
             | <pipeline>

<pipeline> ::= <pipeline> "|" <command>
             | <command>

<command>  ::= STRING <redirects> <args> <bg>
             | STRING <args> <bg>

<redirects> ::= <redirect> <redirects>
              | <redirect>

<redirect> ::= "<" STRING        /* 输入重定向 */
             | ">" STRING        /* 输出重定向（覆盖） */
             | ">>" STRING       /* 输出重定向（追加） */

<args>     ::= STRING <args>
             | ε

<bg>       ::= "&"
             | ε
```

### 歧义来源分析

| 问题 | 说明 |
|------|------|
| **左递归** | `<cmdline>` 和 `<pipeline>` 的直接左递归使递归下降解析器无限循环 |
| **运算符优先级未定** | `&&` 和 `||` 的优先级/结合性未显式指定 |
| **`<redirect>` 与 `<args>` 的交替** | `<redirects>` 中使用重复而不是尾递归，与 `<args>` 交替时产生歧义 |
| **OPTIONAL 的 ε 选择点** | `<bg>` 等 ε-产生式在选择时依赖 FOLLOW 集判断 |

---

## 4. LL(1) 文法

### 4.1 消除左递归 & 左因子化后的文法

```bnf
/* ===== 顶层：命令行（逻辑连接符 && ||） ===== */
<cmdline>       ::= <pipeline> <cmdline_rest>

<cmdline_rest>  ::= "&&" <pipeline> <cmdline_rest>
                  | "||" <pipeline> <cmdline_rest>
                  | ε


/* ===== 第二层：管道 ===== */
<pipeline>      ::= <command> <pipeline_rest>

<pipeline_rest> ::= "|" <command> <pipeline_rest>
                  | ε


/* ===== 第三层：简单命令 ===== */
<command>       ::= STRING <redir_in> <redir_out> <args> <bg>

<redir_in>      ::= "<" STRING
                  | ε

<redir_out>     ::= ">" STRING
                  | ">>" STRING
                  | ε

<args>          ::= STRING <args>
                  | ε

<bg>            ::= "&"
                  | ε
```

### 4.2 FIRST 集

| 非终结符 | FIRST 集 |
|---------|---------|
| `<cmdline>` | `{ STRING }` |
| `<cmdline_rest>` | `{ AND, OR, ε }` |
| `<pipeline>` | `{ STRING }` |
| `<pipeline_rest>` | `{ PIPE, ε }` |
| `<command>` | `{ STRING }` |
| `<redir_in>` | `{ REDIR_IN, ε }` |
| `<redir_out>` | `{ REDIR_OUT, REDIR_APPEND, ε }` |
| `<args>` | `{ STRING, ε }` |
| `<bg>` | `{ BG, ε }` |

### 4.3 FOLLOW 集

| 非终结符 | FOLLOW 集 |
|---------|----------|
| `<cmdline>` | `{ EOF }` |
| `<cmdline_rest>` | `{ EOF }` |
| `<pipeline>` | `{ AND, OR, EOF }` |
| `<pipeline_rest>` | `{ AND, OR, EOF }` |
| `<command>` | `{ PIPE, AND, OR, EOF }` |
| `<redir_in>` | `{ REDIR_OUT, REDIR_APPEND, STRING, BG, PIPE, AND, OR, EOF }` |
| `<redir_out>` | `{ STRING, BG, PIPE, AND, OR, EOF }` |
| `<args>` | `{ BG, PIPE, AND, OR, EOF }` |
| `<bg>` | `{ PIPE, AND, OR, EOF }` |

### 4.4 LL(1) 性质验证

对每个非终结符，检查其各候选式的 FIRST 集是否两两不相交，以及若存在 ε-产生式，FIRST ∩ FOLLOW = ∅：

| 非终结符 | 候选式 | 验证 |
|---------|--------|------|
| `<cmdline_rest>` | `&& ...` / `\|\| ...` / `ε` | `{AND} ∩ {OR} ∩ {ε} = ∅`；`{AND, OR} ∩ {EOF} = ∅` ✅ |
| `<pipeline_rest>` | `\| ...` / `ε` | `{PIPE} ∩ {ε} = ∅`；`{PIPE} ∩ {AND, OR, EOF} = ∅` ✅ |
| `<redir_in>` | `< ...` / `ε` | `{REDIR_IN} ∩ {ε} = ∅`；`{REDIR_IN} ∩ {REDIR_OUT, REDIR_APPEND, STRING, BG, PIPE, AND, OR, EOF} = ∅` ✅ |
| `<redir_out>` | `> ...` / `>> ...` / `ε` | `{REDIR_OUT} ∩ {REDIR_APPEND} ∩ {ε} = ∅`；`{REDIR_OUT, REDIR_APPEND} ∩ {STRING, BG, PIPE, AND, OR, EOF} = ∅` ✅ |
| `<args>` | `STRING ...` / `ε` | `{STRING} ∩ {ε} = ∅`；`{STRING} ∩ {BG, PIPE, AND, OR, EOF} = ∅` ✅ |
| `<bg>` | `&` / `ε` | `{BG} ∩ {ε} = ∅`；`{BG} ∩ {PIPE, AND, OR, EOF} = ∅` ✅ |

> ✅ **所有非终结符满足 LL(1) 条件，文法为 LL(1)。**

### 4.5 运算符优先级与结合性

文法隐式编码了以下优先级（从低到高）：

```
优先级（低 -> 高）      运算符        结合性
─────────────────────────────────────────
  1 (最低)           &&  ||        左结合
  2                  |             左结合
  3 (最高)           < > >> &       右结合（重定向/后台仅作用于单个命令）
```

优先级示例解析：

```
echo hello && cat file | grep foo || wc -l
```
解析为：
```
{ echo hello } && { { cat file | grep foo } || { wc -l } }
```

即：`|` 优先于 `&&`/`||`，`&&`/`||` 同级左结合。

---

### 4.6 递归下降解析伪代码

```cpp
// cmdline ::= pipeline cmdline_rest
AST cmdline() {
    AST left = pipeline();
    return cmdline_rest(left);
}

// cmdline_rest ::= '&&' pipeline cmdline_rest | '||' pipeline cmdline_rest | ε
AST cmdline_rest(AST left) {
    if (lookahead == AND) {
        match(AND);
        AST right = pipeline();
        AST node = make_and(left, right);
        return cmdline_rest(node);
    } else if (lookahead == OR) {
        match(OR);
        AST right = pipeline();
        AST node = make_or(left, right);
        return cmdline_rest(node);
    } else {
        return left;  // ε
    }
}

// pipeline ::= command pipeline_rest
AST pipeline() {
    AST left = command();
    return pipeline_rest(left);
}

// pipeline_rest ::= '|' command pipeline_rest | ε
AST pipeline_rest(AST left) {
    if (lookahead == PIPE) {
        match(PIPE);
        AST right = command();
        AST node = make_pipe(left, right);
        return pipeline_rest(node);
    } else {
        return left;
    }
}

// command ::= STRING redir_in redir_out args bg
AST command() {
    string cmd_name = lookahead_value;
    match(STRING);
    string in_file  = redir_in();
    string out_file = redir_out();
    vector<string> arg_list = args();
    bool is_bg = bg();
    return make_command(cmd_name, in_file, out_file, arg_list, is_bg);
}

// redir_in ::= '<' STRING | ε
string redir_in() {
    if (lookahead == REDIR_IN) {
        match(REDIR_IN);
        string file = lookahead_value;
        match(STRING);
        return file;
    }
    return "";  // ε
}

// redir_out ::= '>' STRING | '>>' STRING | ε
string redir_out() {
    if (lookahead == REDIR_OUT) {
        match(REDIR_OUT);
        string file = lookahead_value;
        match(STRING);
        return file;
    } else if (lookahead == REDIR_APPEND) {
        match(REDIR_APPEND);
        string file = lookahead_value;
        match(STRING);
        return file;
    }
    return "";
}

// args ::= STRING args | ε
vector<string> args() {
    vector<string> result;
    while (lookahead == STRING) {
        result.push_back(lookahead_value);
        match(STRING);
    }
    return result;
}

// bg ::= '&' | ε
bool bg() {
    if (lookahead == BG) {
        match(BG);
        return true;
    }
    return false;
}
```

---

### 4.7 示例解析

| 输入 | 解析结果（AST 简写） |
|------|---------------------|
| `ls -la` | `CMD(ls, -, -, [-la], false)` |
| `ls -la \| grep foo` | `PIPE(CMD(ls, -, -, [-la], false), CMD(grep, -, -, [foo], false))` |
| `cat < in.txt > out.txt` | `CMD(cat, in.txt, out.txt, [], false)` |
| `gcc main.c && ./a.out` | `AND(CMD(gcc, -, -, [main.c], false), CMD(./a.out, -, -, [], false))` |
| `make \|\| echo fail &` | `OR(CMD(make, -, -, [], false), CMD(echo, -, -, [fail], true))` |
| `cmd < in >> out arg1 &` | `CMD(cmd, in, out(append), [arg1], true)` |

> `-` 表示未设置（空字符串/false）。

---

### 4.8 文法对应 AST 节点类型

```cpp
enum class ASTType {
    CMD,        // 简单命令
    PIPE,       // 管道 |
    AND,        // 逻辑与 &&
    OR,         // 逻辑或 ||
};

struct ASTNode {
    ASTType type;
    // --- CMD 节点 ---
    std::string cmd_name;
    std::string redirect_in;   // "" 表示无输入重定向
    std::string redirect_out;  // "" 表示无输出重定向
    bool append_out = false;   // true 表示 >> 追加
    std::vector<std::string> args;
    bool background = false;
    // --- PIPE / AND / OR 节点 ---
    ASTNode *left = nullptr;
    ASTNode *right = nullptr;
};
```


