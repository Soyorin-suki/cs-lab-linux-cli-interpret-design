# 系统调用详解

> 本文档详细解释 user-sh 项目中使用的所有 Linux 系统调用，包括函数签名、工作原理、项目中的使用位置以及关键细节。

---

## 目录

- [系统调用详解](#系统调用详解)
	- [目录](#目录)
	- [1. 概览](#1-概览)
	- [2. 进程管理](#2-进程管理)
		- [2.1 fork() — 创建子进程](#21-fork--创建子进程)
			- [功能](#功能)
			- [返回值](#返回值)
			- [核心特性](#核心特性)
			- [项目中的使用](#项目中的使用)
		- [2.2 execvp() — 执行新程序](#22-execvp--执行新程序)
			- [功能](#功能-1)
			- [命名规则](#命名规则)
			- [关键细节](#关键细节)
			- [项目中的使用](#项目中的使用-1)
		- [2.3 waitpid() — 等待子进程](#23-waitpid--等待子进程)
			- [功能](#功能-2)
			- [参数](#参数)
			- [返回值](#返回值-1)
			- [为什么必须 waitpid？](#为什么必须-waitpid)
			- [项目中的使用](#项目中的使用-2)
		- [2.4 exit() — 终止进程](#24-exit--终止进程)
			- [功能](#功能-3)
			- [关键细节](#关键细节-1)
			- [项目中的使用](#项目中的使用-3)
	- [3. 文件与目录](#3-文件与目录)
		- [3.1 chdir() — 切换工作目录](#31-chdir--切换工作目录)
			- [功能](#功能-4)
			- [返回值](#返回值-2)
			- [为什么 cd 必须是内置命令？](#为什么-cd-必须是内置命令)
			- [项目中的使用](#项目中的使用-4)
		- [3.2 getcwd() — 获取当前目录](#32-getcwd--获取当前目录)
			- [功能](#功能-5)
			- [参数](#参数-1)
			- [返回值](#返回值-3)
			- [项目中的使用](#项目中的使用-5)
	- [4. 管道与 I/O 重定向](#4-管道与-io-重定向)
		- [4.1 pipe() — 创建管道](#41-pipe--创建管道)
			- [功能](#功能-6)
			- [管道 EOF 条件](#管道-eof-条件)
			- [项目中的使用](#项目中的使用-6)
		- [4.2 dup2() — 复制文件描述符](#42-dup2--复制文件描述符)
			- [功能](#功能-7)
			- [典型用法](#典型用法)
			- [管道中的使用](#管道中的使用)
			- [项目中的使用](#项目中的使用-7)
		- [4.3 close() — 关闭文件描述符](#43-close--关闭文件描述符)
			- [功能](#功能-8)
			- [为什么管道中 close 至关重要？](#为什么管道中-close-至关重要)
			- [项目中的使用](#项目中的使用-8)
	- [5. 辅助宏与函数](#5-辅助宏与函数)
		- [5.1 WIFEXITED / WEXITSTATUS / WIFSIGNALED / WTERMSIG](#51-wifexited--wexitstatus--wifsignaled--wtermsig)
			- [使用示例](#使用示例)
			- [项目中的使用](#项目中的使用-9)
		- [5.2 perror() — 打印系统错误](#52-perror--打印系统错误)
			- [功能](#功能-9)
			- [示例](#示例)
			- [项目中所有使用](#项目中所有使用)
	- [6. 系统调用速查表](#6-系统调用速查表)
		- [调用关系图](#调用关系图)
		- [后续扩展将引入的系统调用](#后续扩展将引入的系统调用)

---

## 1. 概览

user-sh 共使用 **9 个系统调用**，按功能分为三类：

```
进程管理：  fork() → execvp() → waitpid() → exit()
文件目录：  chdir()   getcwd()
管道 I/O：  pipe()    dup2()    close()
```

他们在 Shell 中的调用关系如下（以 `ls -la | grep foo` 为例）：

```
Shell (父进程)
  │
  ├─ getcwd()         获取当前目录 → 生成提示符
  │
  ├─ pipe(fd)         创建管道
  │
  ├─ fork() ──────▶ 左子进程                 ├─ fork() ──────▶ 右子进程
  │   dup2(fd[1],1)                            │   dup2(fd[0],0)
  │   close(fd)                                │   close(fd)
  │   execvp("ls",...)                         │   execvp("grep",...)
  │   exit(...)                                │   exit(...)
  │                                             │
  ├─ close(fd)       父进程关闭管道两端          │
  ├─ waitpid(left)   等待左子进程               │
  └─ waitpid(right)  等待右子进程               │
```

---

## 2. 进程管理

### 2.1 fork() — 创建子进程

```c
#include <unistd.h>
pid_t fork(void);
```

#### 功能

创建一个新进程（子进程），它是调用进程（父进程）的**几乎完全相同的副本**。

#### 返回值

| 返回值 | 含义 |
|--------|------|
| `< 0` | 创建失败（进程数达到上限、内存不足） |
| `== 0` | **这是子进程** |
| `> 0` | **这是父进程**，返回值是子进程的 PID |

#### 核心特性

```
调用 fork() 之前：
  ┌──────────┐
  │  父进程   │  (PID=100)
  └──────────┘

调用 fork() 之后——一次调用，两个进程：
  ┌──────────┐     ┌──────────┐
  │  父进程   │     │  子进程   │
  │ PID=100  │     │ PID=200  │
  │ fork→200 │     │ fork→0   │
  └──────────┘     └──────────┘
```

**写时复制（Copy-On-Write, COW）**：现代 Linux 中，fork 后父子共享内存页，只有当任一方**写入**时才真正复制。这大幅降低了 fork 的开销。

**文件描述符继承**：子进程获得父进程 fd 表的副本，但父子进程的 fd 指向**相同的内核文件表项**（共享文件偏移量）。这是管道实现的基础——父子进程通过同一个管道的 fd 通信。

#### 项目中的使用

**位置 1 — 外部命令执行**（`src/MyShell.cpp:execute_cmd()`）：

```cpp
pid_t pid = fork();

if (pid < 0) {
    perror("MyShell: fork 失败");
    return 1;
}

if (pid == 0) {
    // ===== 子进程 =====
    execvp(argv[0], argv.data());   // 用外部程序替换自己
    perror("MyShell");
    exit(EXIT_FAILURE);             // exec 失败才执行到这里
}

// ===== 父进程 =====
int wstatus;
do {
    waitpid(pid, &wstatus, WUNTRACED);
} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
```

**位置 2 — 管道左侧**（`src/MyShell.cpp:execute_ast()` PIPE 分支）：

```cpp
pid_t left_pid = fork();
if (left_pid == 0) {
    dup2(pipe_fd[1], STDOUT_FILENO);  // 重定向 stdout
    close(pipe_fd[0]); close(pipe_fd[1]);
    int rc = execute_ast(node->left.get());
    exit(rc == -1 ? 1 : rc);
}
```

**位置 3 — 管道右侧**：同上，fork + dup2 stdin。

---

### 2.2 execvp() — 执行新程序

```c
#include <unistd.h>
int execvp(const char *file, char *const argv[]);
```

#### 功能

用指定程序**替换**当前进程的地址空间。当前进程的代码、数据、堆栈全部被新程序覆盖，**PID 不变**。

- **成功**：函数不返回（当前进程"变成"了新程序）
- **失败**：返回 `-1`，设置 `errno`

#### 命名规则

```
exec   +  l/v    +  p/e
          ││        ││
          ││        │└── e: 显式传递环境变量 (extern char **environ)
          ││        └─── p: 在 PATH 环境变量中搜索可执行文件
          ││
          │└── v: 参数以 vector 传递 (char *argv[]，末尾 nullptr)
          └─── l: 参数以 list 传递 (arg0, arg1, ..., NULL)
```

选择 `execvp` 的原因：

- `v`：`vector<string>` 转 `char*[]` 方便
- `p`：用户输入 `ls` 而非 `/bin/ls`，由 exec 自动在 PATH 中搜索

#### 关键细节

1. **exec 不改变 fd 表**：当前进程打开的 fd（包括通过 dup2 重定向的 stdin/stdout）**保持不变**。这是管道和重定向工作的前提。
2. **exec 后代码不执行**：exec 成功则当前进程"消失"，后面的代码（如 `exit(EXIT_FAILURE)`）只在 exec 失败时才运行。
3. **argv 约定**：`argv[0]` 是程序名（惯例，可以是任意值），数组以 `nullptr` 结尾。

#### 项目中的使用

```cpp
// src/MyShell.cpp:execute_cmd()

vector<char*> argv;
for (auto& arg : full_args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
}
argv.push_back(nullptr);  // execvp 的终止标记

if (execvp(argv[0], argv.data()) == -1) {
    perror("MyShell");     // 打印失败原因
}
exit(EXIT_FAILURE);        // exec 成功则不会执行到这里
```

---

### 2.3 waitpid() — 等待子进程

```c
#include <sys/wait.h>
pid_t waitpid(pid_t pid, int *wstatus, int options);
```

#### 功能

父进程调用 waitpid 等待指定子进程的状态变化（终止、暂停、恢复）。

#### 参数

| 参数 | 说明 |
|------|------|
| `pid` | 要等待的子进程 PID。`-1` 表示等待任意子进程 |
| `wstatus` | 输出参数，存储子进程的退出状态 |
| `options` | 选项标志。`0` = 阻塞等待；`WNOHANG` = 非阻塞；`WUNTRACED` = 子进程暂停也返回 |

#### 返回值

| 返回值 | 含义 |
|--------|------|
| `> 0` | 状态变化的子进程 PID |
| `== 0` | （仅 WNOHANG 时）没有子进程状态变化 |
| `< 0` | 错误（如没有子进程） |

#### 为什么必须 waitpid？

如果父进程不 waitpid，子进程结束后：
- 内核保留子进程的 PCB（进程控制块），占用 PID 和少量内存
- 子进程变成**僵尸进程**（状态 `Z`，`ps` 看到 `<defunct>`）
- 大量僵尸堆积 → PID 耗尽 → 无法创建新进程

#### 项目中的使用

**位置 1 — 外部命令等待**（`src/MyShell.cpp:execute_cmd()`）：

```cpp
int wstatus;
do {
    waitpid(pid, &wstatus, WUNTRACED);
} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
// 循环等待直到子进程正常退出或被信号杀死
```

**位置 2 — 管道等待**（`src/MyShell.cpp:execute_ast()` PIPE 分支）：

```cpp
int left_status, right_status;
waitpid(left_pid,  &left_status,  0);
waitpid(right_pid, &right_status, 0);
// 必须等待两个子进程都结束
```

> **注意**：管道中 waitpid 的顺序**不重要**，因为管道缓冲区允许左右进程并发执行，父进程只需等两者都结束。

---

### 2.4 exit() — 终止进程

```c
#include <stdlib.h>
void exit(int status);
```

#### 功能

正常终止当前进程，将 `status` 的低 8 位作为退出码返回给父进程。

#### 关键细节

- `exit()` 会执行清理工作：调用 atexit 注册的函数、刷新 stdio 缓冲区、关闭 fd
- `_exit()` 是原始系统调用，**不执行**上述清理
- 子进程 `exec` 失败后应调用 `exit(EXIT_FAILURE)` 而非 `return`

#### 项目中的使用

```cpp
// 子进程中 exec 失败后
exit(EXIT_FAILURE);

// 管道子进程中
int rc = execute_ast(node->left.get());
exit(rc == -1 ? 1 : rc);  // exit 在管道中转为普通退出码
```

---

## 3. 文件与目录

### 3.1 chdir() — 切换工作目录

```c
#include <unistd.h>
int chdir(const char *path);
```

#### 功能

将调用进程的**当前工作目录**（CWD）改为指定路径。

每个进程在内核 PCB 中维护一个 `cwd` 字段，chdir 修改的就是它。

#### 返回值

| 返回值 | 含义 |
|--------|------|
| `0` | 成功 |
| `-1` | 失败，设置 errno（ENOENT=路径不存在, EACCES=权限不足, ENOTDIR=路径是文件） |

#### 为什么 cd 必须是内置命令？

```
【错误做法】cd 作为外部命令
  Shell ──fork()──▶ 子进程 ──chdir("/tmp")──▶ 子进程的 cwd 改为 /tmp
     │                                           │
     │                                           └── exit()，子进程销毁
     │
     └── 父进程的 cwd 不变！（仍然在原目录）


【正确做法】cd 作为内置命令
  Shell ──chdir("/tmp")──▶ Shell 自己的 cwd 改为 /tmp  ✅
```

#### 项目中的使用

```cpp
// src/Builtin.cpp:builtin_cd()

if (args.size() < 2) {
    std::cerr << "cd: 参数过少!\n";
    return 2;
}
if (chdir(args[1].c_str()) != 0) {
    perror("cd");   // 打印 errno 描述
    return 2;
}
return 0;
```

---

### 3.2 getcwd() — 获取当前目录

```c
#include <unistd.h>
char *getcwd(char *buf, size_t size);
```

#### 功能

获取当前进程的绝对路径工作目录，写入 `buf`。

#### 参数

| 参数 | 说明 |
|------|------|
| `buf` | 存放路径的缓冲区 |
| `size` | 缓冲区大小（字节） |

#### 返回值

| 返回值 | 含义 |
|--------|------|
| `buf`（非空指针） | 成功 |
| `NULL` | 失败（缓冲区太小、权限不足等） |

#### 项目中的使用

```cpp
// src/MyShell.cpp:get_pwd()

char buf[4096];
getcwd(buf, sizeof(buf));   // 获取当前目录路径
string pwd(buf);
return pwd;
```

> 缓冲区 4096 字节足以容纳 Linux 支持的最长路径（PATH_MAX = 4096）。

---

## 4. 管道与 I/O 重定向

### 4.1 pipe() — 创建管道

```c
#include <unistd.h>
int pipe(int pipefd[2]);
```

#### 功能

创建一个**单向**数据通道，用于进程间通信（IPC）。内核维护一个环形缓冲区：

```
写端 fd[1] ──▶ [内核管道缓冲区] ──▶ 读端 fd[0]
```

- 写入 `fd[1]` 的数据可以从 `fd[0]` 读出（FIFO 顺序）
- 管道容量通常为 64KB（`pipe capacity`）

#### 管道 EOF 条件

**读端收到 EOF 的条件：所有写端都被关闭。**

这就是为什么管道中必须仔细管理 fd 的关闭——如果某个进程忘记 close 写端，读端将**永久阻塞**。

#### 项目中的使用

```cpp
// src/MyShell.cpp:execute_ast() PIPE 分支

int pipe_fd[2];
if (pipe(pipe_fd) == -1) {
    perror("MyShell: pipe 创建失败");
    return 1;
}

// pipe_fd[0] = 读端 (fd number like 3)
// pipe_fd[1] = 写端 (fd number like 4)
```

---

### 4.2 dup2() — 复制文件描述符

```c
#include <unistd.h>
int dup2(int oldfd, int newfd);
```

#### 功能

将 `newfd` 指向与 `oldfd` **相同的内核文件表项**。如果 `newfd` 已经打开，先自动关闭它。

这是实现重定向和管道的**核心系统调用**。

#### 典型用法

```
// 将标准输出重定向到文件
int fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
dup2(fd, STDOUT_FILENO);  // fd=1 现在指向 output.txt
close(fd);                  // 关闭原始 fd（1 号仍然指向文件）
```

#### 管道中的使用

```
// 左子进程：stdout → 管道写端
dup2(pipe_fd[1], STDOUT_FILENO);  // fd=1 指向管道写端
// 之后子进程的 printf/cout 都流入管道

// 右子进程：stdin → 管道读端
dup2(pipe_fd[0], STDIN_FILENO);   // fd=0 指向管道读端
// 之后子进程的 scanf/cin 都从管道读取
```

#### 项目中的使用

```cpp
// 管道左侧 — stdout 流入管道
dup2(pipe_fd[1], STDOUT_FILENO);
close(pipe_fd[0]);
close(pipe_fd[1]);

// 管道右侧 — stdin 取自管道
dup2(pipe_fd[0], STDIN_FILENO);
close(pipe_fd[0]);
close(pipe_fd[1]);
```

> **关键**：dup2 之后必须 close 原始管道 fd。因为 dup2 只是让 `STDOUT_FILENO`(1) 指向管道，原始的 `pipe_fd[0]`(3) 和 `pipe_fd[1]`(4) 依然存在。如果不关闭，读端永远不会遇到 EOF。

---

### 4.3 close() — 关闭文件描述符

```c
#include <unistd.h>
int close(int fd);
```

#### 功能

关闭文件描述符 `fd`，释放相关内核资源。

#### 为什么管道中 close 至关重要？

这是初学者最容易出错的点。以 `ls | grep foo` 为例，fork 后的 fd 分布：

```
fork 之后，父子进程的 fd 表：
  父: 0(stdin) 1(stdout) 2(stderr) 3(pipe_fd[0]) 4(pipe_fd[1])
  子: 0(stdin) 1(stdout) 2(stderr) 3(pipe_fd[0]) 4(pipe_fd[1])
  （继承关系，指向相同内核对象）

正确的 close 操作：
  ┌─────────────────────────────────────────────────────────────┐
  │ 左子进程: close(3) close(4)  ← 关闭原始管道 fd（已 dup2）   │
  │ 右子进程: close(3) close(4)  ← 同上                          │
  │ 父进程:   close(3) close(4)  ← 父进程不参与 I/O，必须关闭！  │
  └─────────────────────────────────────────────────────────────┘

如果父进程忘记 close(pipe_fd[1])（写端）：
  左子进程退出 → 左子的写端关闭
  但父进程仍持有写端 → 管道写端计数 > 0
  → 右子进程永远读不到 EOF → 死锁！
```

#### 项目中的使用

```cpp
// 管道中——每个涉及方都关闭

// 左子进程：
dup2(pipe_fd[1], STDOUT_FILENO);
close(pipe_fd[0]);  // 关闭不用的读端
close(pipe_fd[1]);  // 关闭已 dup2 的写端

// 右子进程：
dup2(pipe_fd[0], STDIN_FILENO);
close(pipe_fd[0]);  // 关闭已 dup2 的读端
close(pipe_fd[1]);  // 关闭不用的写端

// 父进程（关键！）：
close(pipe_fd[0]);  // 必须关闭
close(pipe_fd[1]);  // 必须关闭
```

---

## 5. 辅助宏与函数

### 5.1 WIFEXITED / WEXITSTATUS / WIFSIGNALED / WTERMSIG

```c
#include <sys/wait.h>
```

这些是**宏**，用于解析 `waitpid()` 返回的 `wstatus`：

| 宏 | 含义 |
|----|------|
| `WIFEXITED(status)` | 子进程是否**正常退出**（调用了 exit 或 main return） |
| `WEXITSTATUS(status)` | 提取正常退出的**退出码**（0~255） |
| `WIFSIGNALED(status)` | 子进程是否被**信号杀死** |
| `WTERMSIG(status)` | 获取杀死子进程的**信号编号** |

#### 使用示例

```cpp
int wstatus;
waitpid(pid, &wstatus, 0);

if (WIFEXITED(wstatus)) {
    int exit_code = WEXITSTATUS(wstatus);
    // exit_code == 0 → 成功
    // exit_code != 0 → 失败（&& 和 || 的判断依据）
}

if (WIFSIGNALED(wstatus)) {
    int sig = WTERMSIG(wstatus);
    // 惯例：返回 128 + 信号编号
    return 128 + sig;
}
```

#### 项目中的使用

```cpp
// src/MyShell.cpp:execute_cmd()
if (WIFEXITED(wstatus)) {
    return WEXITSTATUS(wstatus);   // 正常的退出码
}
return 128 + WTERMSIG(wstatus);    // 被信号杀死

// src/MyShell.cpp:PIPE 分支
if (WIFEXITED(right_status)) {
    return WEXITSTATUS(right_status);
}
if (WIFSIGNALED(right_status)) {
    return 128 + WTERMSIG(right_status);
}
```

---

### 5.2 perror() — 打印系统错误

```c
#include <stdio.h>
void perror(const char *s);
```

#### 功能

打印 `s` + `": "` + 当前 `errno` 对应的错误描述到 stderr。

虽然 `perror` 本身是 C 库函数，但它底层调用 `write(2, ...)` 系统调用，是对 `errno` 的最常用输出方式。

#### 示例

```cpp
chdir("/nonexistent");
// errno = ENOENT (2)

perror("cd");
// 输出到 stderr: cd: No such file or directory
```

#### 项目中所有使用

| 位置 | 调用 | 可能触发场景 |
|------|------|------------|
| `execute_cmd` | `perror("MyShell: fork 失败")` | 系统进程数达上限 |
| `execute_cmd` | `perror("MyShell")` | execvp 失败（命令不存在） |
| PIPE 分支 | `perror("MyShell: pipe 创建失败")` | 系统 fd 耗尽 |
| PIPE 分支 | `perror("MyShell: fork 失败")` | 系统进程数达上限 |
| `builtin_cd` | `perror("cd")` | chdir 失败（路径不存在等） |

---

## 6. 系统调用速查表

| 系统调用 | 头文件 | 作用 | 使用位置 | 行数 |
|---------|--------|------|---------|:--:|
| `fork()` | `<unistd.h>` | 创建子进程 | `execute_cmd()` / PIPE 分支×2 | ~10 |
| `execvp()` | `<unistd.h>` | 加载并执行程序 | `execute_cmd()` | 1 |
| `waitpid()` | `<sys/wait.h>` | 等待子进程结束 | `execute_cmd()` / PIPE 分支×3 | ~6 |
| `exit()` | `<stdlib.h>` | 终止进程 | `execute_cmd()` 子进程 / PIPE 子进程×2 | ~4 |
| `chdir()` | `<unistd.h>` | 切换工作目录 | `builtin_cd()` | 1 |
| `getcwd()` | `<unistd.h>` | 获取工作目录 | `get_pwd()` | 1 |
| `pipe()` | `<unistd.h>` | 创建管道 | PIPE 分支 | 1 |
| `dup2()` | `<unistd.h>` | 复制文件描述符 | PIPE 分支×2 | 2 |
| `close()` | `<unistd.h>` | 关闭文件描述符 | PIPE 分支×6 | 6 |

### 调用关系图

```
main()
  └─ MyShell::init()
      └─ Builtin::init()  注册内置命令表
      
MyShell::loop()
  ├─ get_pwd()
  │   └─ getcwd()         ★ 系统调用
  │
  └─ execute_ast()
      ├─ [CMD] execute_cmd()
      │   ├─ Builtin::execute()
      │   │   ├─ [cd]    chdir()     ★ 系统调用
      │   │   ├─ [exit]  （返回特殊值）
      │   │   └─ [history/help]（纯用户态）
      │   │
      │   └─ [外部命令]
      │       ├─ fork()               ★ 系统调用
      │       ├─ execvp()             ★ 系统调用
      │       ├─ waitpid()            ★ 系统调用
      │       └─ exit()               ★ 系统调用
      │
      ├─ [PIPE]
      │   ├─ pipe()                   ★ 系统调用
      │   ├─ fork() ×2                ★ 系统调用
      │   ├─ dup2() ×2               ★ 系统调用
      │   ├─ close() ×6              ★ 系统调用
      │   ├─ waitpid() ×2            ★ 系统调用
      │   └─ exit() ×2               ★ 系统调用
      │
      ├─ [AND]  execute_ast(left) → 短路判断 → execute_ast(right)
      └─ [OR]   execute_ast(left) → 短路判断 → execute_ast(right)
```

### 后续扩展将引入的系统调用

| 系统调用 | 用途 | 对应功能 |
|---------|------|---------|
| `open()` | 打开文件 | 重定向 `<` `>` `>>` |
| `sigaction()` / `signal()` | 注册信号处理 | `SIGCHLD` 回收后台进程、`SIGINT` Ctrl+C |

---

> 📖 参考：`man 2 fork`, `man 2 execvp`, `man 2 waitpid`, `man 2 pipe`, `man 2 dup2`
