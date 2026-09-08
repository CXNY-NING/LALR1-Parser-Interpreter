#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_QUADS 1000
#define MAX_NAME_LEN 32
#define MAX_VARS 500
#define MAX_PROC 100
#define MAX_CALL_STACK 100
#define LINE_LEN 256

// 四元式结构体
typedef struct {
    char op[16];
    char arg1[MAX_NAME_LEN];
    char arg2[MAX_NAME_LEN];
    char result[MAX_NAME_LEN];
} Quad;

// 变量结构体
typedef struct {
    char name[MAX_NAME_LEN];
    int value;
} Var;

// 过程结构体
typedef struct {
    char name[MAX_NAME_LEN];
    int pc;
} Proc;

Quad quads[MAX_QUADS];
int quad_count = 0;

Var vars[MAX_VARS];
int var_count = 0;

Proc procs[MAX_PROC];
int proc_count = 0;

int call_stack[MAX_CALL_STACK];
int call_top = -1;

bool trace_mode = false;
bool step_mode = false;

// 去除字符串首尾空白
void trim(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    int len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

// 解析一行四元式
// 输入格式: "index : (op, arg1, arg2, result)"
bool parse_line(char *line, Quad *q) {
    char *lparen = strchr(line, '(');
    char *rparen = strchr(line, ')');
    if (lparen == NULL || rparen == NULL || rparen < lparen) {
        return false;
    }
    *rparen = '\0';
    lparen++;

    char *parts[4];
    int idx = 0;
    char *token = strtok(lparen, ",");
    while (token != NULL && idx < 4) {
        parts[idx++] = token;
        token = strtok(NULL, ",");
    }
    if (idx != 4) {
        return false;
    }

    trim(parts[0]);
    trim(parts[1]);
    trim(parts[2]);
    trim(parts[3]);

    strncpy(q->op, parts[0], 15); q->op[15] = '\0';
    strncpy(q->arg1, parts[1], MAX_NAME_LEN - 1); q->arg1[MAX_NAME_LEN - 1] = '\0';
    strncpy(q->arg2, parts[2], MAX_NAME_LEN - 1); q->arg2[MAX_NAME_LEN - 1] = '\0';
    strncpy(q->result, parts[3], MAX_NAME_LEN - 1); q->result[MAX_NAME_LEN - 1] = '\0';

    return true;
}

// 加载四元式文件
void load_quads(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "错误：无法打开文件 '%s'\n", filename);
        exit(1);
    }

    char line[LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        // 去除换行符
        line[strcspn(line, "\n")] = '\0';
        // 跳过空行
        bool blank = true;
        for (int i = 0; line[i]; i++) {
            if (!isspace((unsigned char)line[i])) {
                blank = false;
                break;
            }
        }
        if (blank) continue;

        Quad q;
        if (!parse_line(line, &q)) {
            fprintf(stderr, "错误：无法解析第 %d 行：%s\n", quad_count + 1, line);
            exit(1);
        }

        if (quad_count >= MAX_QUADS) {
            fprintf(stderr, "错误：四元式数量超过上限 %d\n", MAX_QUADS);
            exit(1);
        }
        quads[quad_count++] = q;
    }

    fclose(fp);
    printf("已加载 %d 条四元式。\n", quad_count);
}

// 判断字符串是否为整数常量
bool is_number(const char *s) {
    if (s == NULL || *s == '\0') return false;
    int i = 0;
    if (s[0] == '-' || s[0] == '+') i++;
    bool has_digit = false;
    for (; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return false;
        has_digit = true;
    }
    return has_digit;
}

// 查找变量，返回下标，未找到返回 -1
int find_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// 设置变量值，不存在则创建
void set_var(const char *name, int value) {
    int idx = find_var(name);
    if (idx >= 0) {
        vars[idx].value = value;
        return;
    }

    if (var_count >= MAX_VARS) {
        fprintf(stderr, "错误：变量数量超过上限 %d\n", MAX_VARS);
        exit(1);
    }
    strncpy(vars[var_count].name, name, MAX_NAME_LEN - 1);
    vars[var_count].name[MAX_NAME_LEN - 1] = '\0';
    vars[var_count].value = value;
    var_count++;
}

// 获取变量值，未定义则报错退出
int get_var_value(const char *name) {
    int idx = find_var(name);
    if (idx < 0) {
        fprintf(stderr, "错误：读取未定义变量 '%s'\n", name);
        exit(1);
    }
    return vars[idx].value;
}

// 获取操作数的值：数字直接返回，变量查表
int get_value(const char *s) {
    if (strcmp(s, "-") == 0) {
        return 0;
    }
    if (is_number(s)) {
        return atoi(s);
    }
    return get_var_value(s);
}

// 注册过程入口
void register_proc(const char *name, int pc) {
    for (int i = 0; i < proc_count; i++) {
        if (strcmp(procs[i].name, name) == 0) {
            fprintf(stderr, "错误：过程 '%s' 重复定义\n", name);
            exit(1);
        }
    }
    if (proc_count >= MAX_PROC) {
        fprintf(stderr, "错误：过程数量超过上限 %d\n", MAX_PROC);
        exit(1);
    }
    strncpy(procs[proc_count].name, name, MAX_NAME_LEN - 1);
    procs[proc_count].name[MAX_NAME_LEN - 1] = '\0';
    procs[proc_count].pc = pc;
    proc_count++;
}

// 查找过程入口
int find_proc_pc(const char *name) {
    for (int i = 0; i < proc_count; i++) {
        if (strcmp(procs[i].name, name) == 0) {
            return procs[i].pc;
        }
    }
    return -1;
}

// 调用栈操作
void push_call(int return_pc) {
    if (call_top >= MAX_CALL_STACK - 1) {
        fprintf(stderr, "错误：调用栈溢出\n");
        exit(1);
    }
    call_stack[++call_top] = return_pc;
}

int pop_call() {
    if (call_top < 0) {
        fprintf(stderr, "错误：调用栈下溢\n");
        exit(1);
    }
    return call_stack[call_top--];
}

// 打印所有变量
void print_vars() {
    printf("\n最终变量值:\n");
    if (var_count == 0) {
        printf("  (无变量)\n");
        return;
    }
    for (int i = 0; i < var_count; i++) {
        printf("  %-10s = %d\n", vars[i].name, vars[i].value);
    }
}

// 获取条件跳转目标，并检查合法性
// 允许 target == quad_count，表示"跳转到程序末尾"（即正常结束）
int get_jump_target(const char *s) {
    if (!is_number(s)) {
        fprintf(stderr, "错误：跳转目标 '%s' 不是有效行号\n", s);
        exit(1);
    }
    int target = atoi(s);
    if (target < 0 || target > quad_count) {
        fprintf(stderr, "错误：跳转目标 %d 超出四元式范围 [0, %d]\n", target, quad_count);
        exit(1);
    }
    return target;
}

// 主执行循环
void run() {
    // 第一遍扫描，建立过程表
    for (int pc = 0; pc < quad_count; pc++) {
        if (strcmp(quads[pc].op, "proc") == 0) {
            register_proc(quads[pc].arg1, pc);
        }
    }

    int pc = 0;
    while (pc >= 0 && pc < quad_count) {
        Quad *q = &quads[pc];
        int next_pc = pc + 1;

        if (trace_mode) {
            printf("[PC=%d] %-6s %-6s %-6s -> %s\n", pc, q->op, q->arg1, q->arg2, q->result);
        }

        if (strcmp(q->op, "+") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            set_var(q->result, a + b);
            if (trace_mode) printf("        %s = %d + %d = %d\n", q->result, a, b, a + b);
        }
        else if (strcmp(q->op, "-") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            set_var(q->result, a - b);
            if (trace_mode) printf("        %s = %d - %d = %d\n", q->result, a, b, a - b);
        }
        else if (strcmp(q->op, "*") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            set_var(q->result, a * b);
            if (trace_mode) printf("        %s = %d * %d = %d\n", q->result, a, b, a * b);
        }
        else if (strcmp(q->op, "/") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            if (b == 0) {
                fprintf(stderr, "错误：除零（PC=%d）\n", pc);
                exit(1);
            }
            set_var(q->result, a / b);
            if (trace_mode) printf("        %s = %d / %d = %d\n", q->result, a, b, a / b);
        }
        else if (strcmp(q->op, ":=") == 0) {
            int a = get_value(q->arg1);
            set_var(q->result, a);
            if (trace_mode) printf("        %s = %d\n", q->result, a);
        }
        else if (strcmp(q->op, "goto") == 0) {
            next_pc = get_jump_target(q->result);
            if (trace_mode) printf("        -> 跳转到 %d\n", next_pc);
        }
        else if (strcmp(q->op, "j=") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            if (a == b) {
                next_pc = get_jump_target(q->result);
                if (trace_mode) printf("        %d == %d -> 跳转到 %d\n", a, b, next_pc);
            } else {
                if (trace_mode) printf("        %d != %d -> 不跳转\n", a, b);
            }
        }
        else if (strcmp(q->op, "j<>") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            if (a != b) {
                next_pc = get_jump_target(q->result);
                if (trace_mode) printf("        %d != %d -> 跳转到 %d\n", a, b, next_pc);
            } else {
                if (trace_mode) printf("        %d == %d -> 不跳转\n", a, b);
            }
        }
        else if (strcmp(q->op, "j<") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            if (a < b) {
                next_pc = get_jump_target(q->result);
                if (trace_mode) printf("        %d < %d -> 跳转到 %d\n", a, b, next_pc);
            } else {
                if (trace_mode) printf("        %d >= %d -> 不跳转\n", a, b);
            }
        }
        else if (strcmp(q->op, "j>") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            if (a > b) {
                next_pc = get_jump_target(q->result);
                if (trace_mode) printf("        %d > %d -> 跳转到 %d\n", a, b, next_pc);
            } else {
                if (trace_mode) printf("        %d <= %d -> 不跳转\n", a, b);
            }
        }
        else if (strcmp(q->op, "j<=") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            if (a <= b) {
                next_pc = get_jump_target(q->result);
                if (trace_mode) printf("        %d <= %d -> 跳转到 %d\n", a, b, next_pc);
            } else {
                if (trace_mode) printf("        %d > %d -> 不跳转\n", a, b);
            }
        }
        else if (strcmp(q->op, "j>=") == 0) {
            int a = get_value(q->arg1);
            int b = get_value(q->arg2);
            if (a >= b) {
                next_pc = get_jump_target(q->result);
                if (trace_mode) printf("        %d >= %d -> 跳转到 %d\n", a, b, next_pc);
            } else {
                if (trace_mode) printf("        %d < %d -> 不跳转\n", a, b);
            }
        }
        else if (strcmp(q->op, "jodd") == 0) {
            int a = get_value(q->arg1);
            if (a % 2 != 0) {
                next_pc = get_jump_target(q->result);
                if (trace_mode) printf("        %d 是奇数 -> 跳转到 %d\n", a, next_pc);
            } else {
                if (trace_mode) printf("        %d 是偶数 -> 不跳转\n", a);
            }
        }
        else if (strcmp(q->op, "proc") == 0) {
            // 过程入口标记，仅作为标签使用
            if (trace_mode) printf("        (过程 %s 入口)\n", q->arg1);
        }
        else if (strcmp(q->op, "endproc") == 0) {
            int ret = pop_call();
            if (trace_mode) printf("<-- endproc, 返回到 %d\n", ret);
            next_pc = ret;
        }
        else if (strcmp(q->op, "call") == 0) {
            int target = find_proc_pc(q->arg1);
            if (target < 0) {
                fprintf(stderr, "错误：调用未定义过程 '%s'（PC=%d）\n", q->arg1, pc);
                exit(1);
            }
            push_call(pc + 1);
            if (trace_mode) printf("--> call %s, 返回地址 %d\n", q->arg1, pc + 1);
            next_pc = target;
        }
        else if (strcmp(q->op, "print") == 0) {
            int a = get_value(q->arg1);
            printf("[print] %s = %d\n", q->arg1, a);
        }
        else {
            fprintf(stderr, "错误：未知操作码 '%s'（PC=%d）\n", q->op, pc);
            exit(1);
        }

        pc = next_pc;

        if (step_mode) {
            printf("按回车继续...");
            getchar();
        }
    }

    printf("\n程序执行结束。\n");
    print_vars();
}

// 询问用户是否开启某个模式
bool ask_mode(const char *prompt) {
    char buf[8];
    printf("%s (yes/no): ", prompt);
    fflush(stdout);
    if (scanf("%7s", buf) != 1) {
        return false;
    }
    return (strcmp(buf, "yes") == 0 || strcmp(buf, "Yes") == 0);
}

int main(int argc, char *argv[]) 
{

    SetConsoleOutputCP(CP_UTF8);
    const char *filename = "quads.txt";
    if (argc >= 2) {
        filename = argv[1];
    }

    printf("四元式解释器\n");
    printf("输入文件: %s\n\n", filename);

    trace_mode = ask_mode("是否开启追踪模式");
    step_mode = ask_mode("是否开启单步模式");

    // 消耗 scanf 留下的换行符，防止单步模式第一次直接跳过
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("\n");

    load_quads(filename);
    run();

    return 0;
}
