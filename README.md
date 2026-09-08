# 自底向上的 LALR(1) 语法分析器与四元式解释器

本项目是个人自学《编译原理》过程中完成的一个小项目，实现了一个**自底向上的 LALR(1) 语法分析器**，并附带一个独立的**四元式解释器**。编译器将类 PL/0 源程序翻译为四元式中间代码；解释器读取这些四元式并执行，以验证中间代码的正确性。

通过这个项目，可以完整地体验从源代码到中间代码再到执行结果的全流程：

1. 用 LALR(1) 自底向上分析器进行语法分析；
2. 在分析过程中嵌入语义动作，完成类型检查、符号管理和中间代码生成；
3. 用独立的解释器执行生成的四元式，验证语义正确性。

整个项目采用 C/C++ 实现，不依赖第三方编译器框架，适合用来理解编译器前端的核心机制。

---

## 背景知识

### 自底向上语法分析

自底向上（Bottom-Up）分析是从输入串出发，逐步归约到文法开始符号的过程。它对应于从语法树的叶子节点向上构造整棵树，因此也被称为**移进-归约分析**。LR 分析器是自底向上分析中最强大、最常用的一类，能够处理大部分上下文无关文法。

### LR 分析家族

LR 分析器按能力从强到弱可分为：

- **LR(1)**：能力最强，每个项目带有 lookahead 符号，状态数较多；
- **LALR(1)**（Look-Ahead LR）：合并 LR(1) 中 **同心** 的项目集，状态数与 SLR(1) 相当，能力接近 LR(1)，是实际编译器（如 Yacc/Bison）中最常用的选择；
- **SLR(1)**：使用 Follow 集合解决冲突，能力最弱。

本项目实现的是 **LALR(1)**：先构造完整的 LR(1) 项目集族，再合并同心项目集，最后生成 ACTION/GOTO 分析表。

### 翻译模式与中间代码

语法分析只判断程序结构是否合法，要真正理解程序含义，还需要**语义分析**和**中间代码生成**。本项目在产生式中插入语义动作（通过 `M`、`N`、`P` 等空产生式标记位置），在归约时触发动作，生成**四元式**中间代码。

四元式是常见的三地址码表示，每条指令形如 `(op, arg1, arg2, result)`，例如 `(+, a, b, t0)` 表示 `t0 = a + b`。四元式结构规整，便于后续的代码优化和目标代码生成。

### 解释器的作用

四元式只是中间表示，无法直接运行。`QuadInterpreter/main.c` 是一个小型虚拟机：

- 维护程序计数器 `PC` 和变量表；
- 顺序执行四元式，遇到跳转或过程调用时修改 `PC`；
- 最终输出所有变量的值，帮助验证编译器生成的中间代码是否正确。

---

## 目录

- [背景知识](#背景知识)
- [项目概述](#项目概述)
- [仓库结构](#仓库结构)
- [支持的语法](#支持的语法)
- [快速开始](#快速开始)
- [LALR(1) 分析器](#lalr1-分析器)
- [四元式解释器](#四元式解释器)
- [示例](#示例)
- [注意事项](#注意事项)
- [关于本项目](#关于本项目)

---

## 项目概述

- **主程序**：`自底向上的语法分析（LALR(1)）.cpp`
  - 从 `prod.txt` 读取文法产生式；
  - 从 `test.txt` 读取待分析的源程序；
  - 构造 LALR(1) 分析表；
  - 一遍扫描完成语法分析与中间代码生成；
  - 输出四元式序列到 `quads.txt`。

- **解释器**：`QuadInterpreter/main.c`
  - 读取 `quads.txt` 中的四元式；
  - 逐条解释执行，支持算术运算、赋值、条件/无条件跳转、过程调用；
  - 可选追踪模式和单步模式；
  - 程序结束时打印所有变量的最终值。

---

## 仓库结构

```
.
├── 自底向上的语法分析（LALR(1)）.cpp   # 主 LALR(1) 编译器
├── 自底向上的语法分析（LALR(1)）.sln   # Visual Studio 解决方案
├── 自底向上的语法分析（LALR(1)）.vcxproj
├── 自底向上的语法分析（LALR(1)）.vcxproj.filters
├── prod.txt                            # 文法产生式
├── test.txt                            # 源程序样例 1
├── test_new.txt                        # 源程序样例 2
├── quads.txt                           # 生成的四元式（运行后生成）
├── log.txt                             # 运行日志
├── .vscode/                            # VS Code 调试配置
├── QuadInterpreter/                    # 四元式解释器（独立目录）
│   ├── main.c
│   ├── quads.txt
│   ├── requirements.md
│   └── README.md
└── README.md                           # 本文件
```

---

## 支持的语法

本项目采用类 PL/0 的文法，支持常量声明、变量声明、过程声明、赋值语句、条件语句、循环语句、过程调用以及算术/关系表达式。

主要产生式如下：

```
Start      -> Program
Program    -> Block
Block      -> ConstDecl VarDecl ProcDeclList Stmt

ConstDecl  -> const ConstDefList ;
ConstDecl  -> eps
ConstDef   -> ident = number

VarDecl    -> var IdentList ;
VarDecl    -> eps

ProcDecl   -> procedure ident ; N P Block
ProcDeclList -> ProcDecl ; ProcDeclList | eps

Stmt       -> ident := Exp
           |  call ident
           |  begin StmtList end
           |  if Cond then M Stmt
           |  while M Cond do M Stmt
           |  eps

Cond       -> Exp = Exp | Exp <> Exp | Exp < Exp | Exp > Exp
           |  Exp <= Exp | Exp >= Exp | odd ident

Exp        -> Exp + Term | Exp - Term | Term
Term       -> Term * Factor | Term / Factor | Factor
Factor     -> ident | number | ( Exp )
```

说明：
- `M`、`N`、`P` 是为翻译模式插入的语义动作标记（空产生式）；
- 过程无参数、无返回值，通过全局变量通信；
- 所有变量和常量均为整数类型；
- `odd` 用于判断奇数。

---

## 快速开始

### 1. 编译并运行 LALR(1) 分析器

使用 Visual Studio 打开 `自底向上的语法分析（LALR(1)）.sln`，编译运行；

或使用 g++：

```bash
g++ "自底向上的语法分析（LALR(1)）.cpp" -o lalr1.exe
./lalr1.exe
```

程序默认读取：
- `prod.txt`：文法产生式
- `test.txt`：源程序

运行后生成：
- `quads.txt`：四元式序列
- `log.txt`：分析过程日志

### 2. 编译并运行四元式解释器

进入 `QuadInterpreter` 目录：

```bash
cd QuadInterpreter

gcc main.c -o quad_interp.exe
./quad_interp.exe
```

解释器默认读取当前目录下的 `quads.txt`。也可以指定其他文件：

```bash
./quad_interp.exe my_quads.txt
```

---

## LALR(1) 分析器

### 主要功能

1. **读入文法**：从 `prod.txt` 读取产生式，自动识别终结符与非终结符；
2. **计算 First/Follow 集合**：用于构造 LALR(1) 项目集和分析表；
3. **构造 LALR(1) 项目集族**：合并同心 LR(1) 项目集，形成 LALR(1) 状态；
4. **生成分析表**：包括 `ACTION` 表（移进、归约、接受、报错）和 `GOTO` 表；
5. **一遍扫描分析**：边分析边执行语义动作，生成四元式；
6. **输出结果**：将生成的四元式写入 `quads.txt`。

### 核心数据结构

- **符号表**：用结构体数组记录所有终结符与非终结符；
- **产生式表**：记录每条产生式的左部符号和右部符号序列；
- **First/Follow 集合**：用布尔矩阵表示，便于快速查询；
- **LR(1) 项目集**：每个项目形如 `[A -> α·β, a]`，记录归约展望符；
- **LALR(1) 分析表**：`ACTION[state][terminal]` 和 `GOTO[state][non-terminal]`；
- **状态栈/符号栈**：标准 LR 分析的双栈结构；
- **语义栈**：在归约时传递属性值，用于计算表达式和生成四元式。

### 分析流程

```
读入产生式 -> 计算 First/Follow -> 构造 LR(1) 项目集
    -> 合并同心项目集得到 LALR(1) 状态 -> 生成 ACTION/GOTO 表
    -> 扫描源程序 -> 移进/归约 -> 触发语义动作 -> 输出 quads.txt
```

在归约过程中，程序会根据当前归约的产生式执行对应的语义子程序，例如：

- 常量声明：生成 `(:=, value, -, ident)` 初始化四元式；
- 表达式运算：生成 `(+, a, b, t)` 等算术四元式；
- 条件语句：生成条件跳转和回填目标地址；
- 循环语句：生成循环入口、条件判断和回跳四元式；
- 过程调用：生成 `proc`/`call`/`endproc` 相关四元式。

### 文法输入格式

`prod.txt` 中每行一条产生式，格式为：

```
左部 -> 右部符号1 右部符号2 ... 右部符号n
```

例如：

```
Start -> Program
Program -> Block
Block -> ConstDecl VarDecl ProcDeclList Stmt
Stmt -> ident := Exp
```

空产生式用 `eps` 表示：

```
ConstDecl -> eps
```

### 源程序输入格式

`test.txt` 为待编译的类 PL-0 源程序，例如：

```pascal
const m = 15 , n = 35 , k = 8 ;
var u , v , w , t ;
procedure s ;
begin
u := m - n / k ;
if u <> v then
v := ( u + t ) * n
end ;
procedure r ;
begin
while w <= m do
begin
w := w * 2 ;
t := t - w + m
end ;
call s ;
if odd t then
w := t / ( n - k )
end ;
```

---

## 四元式解释器

### 支持的中间代码

四元式格式为：

```
index : (op, arg1, arg2, result)
```

例如：

```
0   : (goto, -     , -     , 11)
1   : (proc, s     , -     , -)
2   : (/   , n     , k     , t0)
```

### 操作码说明

| 操作码 | 含义 | 四元式形式 | 语义 |
|--------|------|-----------|------|
| `+` `-` `*` `/` | 算术运算 | `(+, a, b, t)` | `t = a + b` |
| `:=` | 赋值 | `(:=, src, -, dst)` | `dst = src` |
| `goto` | 无条件跳转 | `(goto, -, -, L)` | `PC = L` |
| `j=` `j<>` `j<` `j>` `j<=` `j>=` | 条件跳转 | `(j<, a, b, L)` | 若 `a < b`，则 `PC = L` |
| `jodd` | 奇数跳转 | `(jodd, a, -, L)` | 若 `a` 为奇数，则 `PC = L` |
| `proc` | 过程入口 | `(proc, name, -, -)` | 标记过程开始 |
| `endproc` | 过程出口 | `(endproc, -, -, -)` | 返回调用者 |
| `call` | 调用过程 | `(call, name, -, -)` | 跳转至 `name` 过程 |
| `print` | 打印 | `(print, x, -, -)` | 输出 `x = 值` |

### 运行交互

启动后依次询问：

1. **是否开启追踪模式？** 每执行一条四元式打印详细信息；
2. **是否开启单步模式？** 每执行一条后暂停，按回车继续。

程序结束时自动打印所有变量的最终值。

---

## 示例

以 `test_new.txt` 为例：

```pascal
const a = 10 , b = 3 ;
var x , y , z ;
procedure p ;
begin
    x := a + b ;
    if x > 10 then
        y := x * 2
end ;
begin
    call p ;
    z := y - b ;
    while z > 0 do
    begin
        z := z - 1 ;
        x := x + 1
    end
end
```

运行主分析器后，`quads.txt` 会生成对应的四元式序列。再运行解释器即可观察变量变化过程与最终结果。

---

## 注意事项

1. **字符编码**：产生式文件 `prod.txt` 支持 UTF-8（含 BOM 或无 BOM），程序内部会跳过 BOM；
2. **整型运算**：解释器只支持整数运算，`/` 为整除；
3. **变量作用域**：当前实现中所有变量视为全局变量，过程之间通过全局变量通信；
4. **过程调用**：不支持递归调用；
5. **调试输出**：主分析器支持 `need_log` 调试开关（在源代码中控制），可打印分析过程细节。

---

## 关于本项目

本项目是个人自学《编译原理》过程中的练习作品，目标是通过手写代码深入理解编译器前端的几个核心环节：

- 自底向上的 LR/LALR 分析表构造；
- 语法制导的翻译模式；
- 中间代码（四元式）生成；
- 中间代码的解释执行。

项目代码和文档均为学习整理所用，欢迎交流指正。
