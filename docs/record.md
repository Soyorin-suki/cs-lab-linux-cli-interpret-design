# 记录
> 参考[write a shell in c](https://brennan.io/2015/01/16/write-a-shell-in-c/)


## basic lifetime of shell

- initialize
- interpret
- terminate


## how shells start processes
在类 Unix 系统中，有两种方法启动进程：
1. 在机器启动时自动启动的 `Init`, 它会一直运行直到机器关闭
2. 通过 `fork()` 系统调用

前者略过

后者在系统调用该函数时，操作系统会复制它们运行。原始进程称为“父进程”，`fork()` 会向子进程返回0，并向父进程返回子进程的pid

从本质上讲，这意味着新进程的唯一启动方式是通过现有流程的自我复制来启动

### `fork()`
`fork()` 会复制一份完全一样的进程，并返回一个 `pid`
- 如果启动失败：返回 `pid<0`
- 如果启动成功: 
	- 对于父进程: 返回子进程的 `pid`
	- 对于子进程: 返回 `pid==0`

### `exevp()`
`exec`有着许多变体，这里使用的`execvp()`中的`v`表示其接受一个参数数组，或者说参数向量，`p`表示不要提供程序完整路径，而是给出名称，让os在路径中搜索

如果`exec`返回`-1`或任何非零值，就表示其出现了错误，我们调用`perror()`打印错误信息，并附上程序名称，之后我们退出程序，让 shell 继续运行

## shell builtins(shell 内置命令)



## version2
每个进程会默认打开 3 个文件描述符(file descriptor, fd)
- `0`: 标准输入(stdin)
- `1`: 标准输出(stdout)
- `2`: 标准错误(stderr)



### 重定向
重定向是将输入输出设置为具体的某一磁盘文件

### 管道
管道是将输入输出设置为某一进程的输出

### 26.6.29
注意到如果想要写重定向和管道功能的话，最好将其进行简单的语法处理

将其分成三种语法元素:`Command`, `Pip`, `Redirect`

