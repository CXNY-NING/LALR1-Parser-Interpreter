#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<limits.h>
#include<ctype.h>
#include<stdbool.h>
#include <windows.h>

#define Symbol_name_lenth 20
#define Production_right_Max_lenth 20
#define Symbol_Max_lenth 100
#define Prods_Max_length 100
#define Prods_line_Max_lenth 100
#define address_lenth 100
#define Expect_len 200
#define Quad_Max 1000
#define Sem_Stack_Max 200

//调试开关
bool need_log;

// 符号结构体定义
typedef struct symbol
{
	char name[Symbol_name_lenth];
	bool is_terminal;
}Symbol;

// 产生式结构体定义
typedef struct production
{
	int left;
	int right[Production_right_Max_lenth];
	int right_lenth;
}Production;

// 符号表数组
Symbol symbol[Symbol_Max_lenth];
int symbol_count = 0;

// 产生式数组
Production prods[Prods_Max_length];
int prod_count = 0;

// 查找或添加符号
int find_symbol(const char str[], bool cmp_terminal);

// 特殊符号位置
int start_pos = find_symbol("Start", false);
int eps_pos = find_symbol("eps", true);
int end_pos = find_symbol("$", true);

// First和Follow集合数组
bool first[Symbol_Max_lenth][Symbol_Max_lenth] = { false };
bool follow[Symbol_Max_lenth][Symbol_Max_lenth] = { false };

// 显示产生式
void show_prods()
{
	printf("产生式:\n");
	for (int i = 0; i < prod_count; i++)
	{
		int left_pos = prods[i].left;
		printf("%s", symbol[left_pos].name);
		printf(" -> ");
		for (int j = 0; j < prods[i].right_lenth; j++)
		{
			int right_pos = prods[i].right[j];
			printf("%s ", symbol[right_pos].name);
		}
		printf("\n");
	}
}

// 显示单个产生式
void show_prod(int prod_num)
{
	int left = prods[prod_num].left;
	printf("%s->", symbol[left].name);
	int p = 0;
	for (int i = 0; i < prods[prod_num].right_lenth; i++)
	{
		int right = prods[prod_num].right[i];
		if (p == 0)
		{
			printf("%s", symbol[right].name);
			p = 1;
		}
		else
		{
			printf(" %s", symbol[right].name);
		}
	}
	printf("\n");
}

// 显示First集合
void show_first()
{
	printf("First集合:\n\n");
	for (int i = 0; i < symbol_count; i++)
	{
		// 跳过终结符
		if (symbol[i].is_terminal)
			continue;

		printf("%s: {", symbol[i].name);
		// 控制逗号输出格式
		int flag = 0;
		for (int j = 0; j < symbol_count; j++)
		{
			if (first[i][j])
			{
				if (flag == 1)
				{
					printf(",");
				}
				printf("%s", symbol[j].name);
				flag = 1;
			}
		}
		printf("}\n");
	}
}

// 显示Follow集合
void show_follow()
{
	printf("Follow集合:\n\n");
	for (int i = 0; i < symbol_count; i++)
	{
		// 跳过终结符
		if (symbol[i].is_terminal)
			continue;

		printf("%s: {", symbol[i].name);
		// 控制逗号输出格式
		int flag = 0;
		for (int j = 0; j < symbol_count; j++)
		{
			if (follow[i][j])
			{
				if (flag == 1)
				{
					printf(",");
				}
				printf("%s", symbol[j].name);
				flag = 1;
			}
		}
		printf("}\n");
	}
}

// 存储符号信息
int find_symbol(const char str[], bool cmp_terminal)
{
	for (int i = 0; i < symbol_count; i++)
	{
		if (strcmp(symbol[i].name, str) == 0)
		{
			return i;
		}
	}
	strcpy(symbol[symbol_count].name, str);
	symbol[symbol_count].is_terminal = cmp_terminal;
	symbol_count++;
	return symbol_count - 1;
}

// 加载产生式文件
void load_Prods(const char address[])
{
	FILE* fp = fopen(address, "r");
	if (fp == NULL)
	{
		printf("打开产生式文件失败\n");
		exit(1);
	}
	char bom[3];
	if (fread(bom, 1, 3, fp) == 3)
	{
		if (!(bom[0] == (char)0xEF &&
			bom[1] == (char)0xBB &&
			bom[2] == (char)0xBF))
		{
			fseek(fp, 0, SEEK_SET); // Skip BOM if present
		}
	}
	prod_count = 1;
	char Prods_line[Prods_line_Max_lenth];
	while (fgets(Prods_line, sizeof(Prods_line), fp))
	{
		// 读取文件内容
		Prods_line[strcspn(Prods_line, "\n")] = '\0';
		int len = strlen(Prods_line);

		// 去除行尾空白字符
		while (len > 0 && isspace(Prods_line[len - 1]))
		{
			len--;
			Prods_line[len] = '\0';
		}
		if (len == 0)
		{
			continue;
		}

		// 查找产生式箭头
		char* arrow = strstr(Prods_line, "->");
		if (!arrow)
		{
			continue;
		}
		*arrow = '\0';

		// 去除左部尾部空白
		char* left = Prods_line;
		int left_len = strlen(left);
		while (left_len > 0 && isspace(left[left_len - 1]))
		{
			left_len--;
			left[left_len] = '\0';
		}
		// 左部为空则跳过
		if (left_len == 0)
		{
			continue;
		}
		int left_pos = find_symbol(left, false);


		// 处理产生式右部
		char* right = arrow + 2;
		char* part = strtok(right, "|");
		while (part)
		{
			char token[Symbol_name_lenth];
			int token_pos = 0;
			int right_arr[Symbol_name_lenth];
			int right_count = 0;

			int part_line = strlen(part);
			for (int i = 0; i < part_line; i++)
			{
				if (isspace(part[i]))
				{
					if (token_pos > 0)
					{
						token[token_pos] = '\0';
						bool cmp_terminal = !isupper(token[0]);
						right_arr[right_count] = find_symbol(token, cmp_terminal);
						right_count++;
						token_pos = 0;
					}
					continue;
				}

				if (token_pos < Symbol_name_lenth - 1)
				{
					token[token_pos] = part[i];
					token_pos++;
				}
			}

			if (token_pos > 0)
			{
				token[token_pos] = '\0';
				bool cmp_terminal = !isupper(token[0]);
				right_arr[right_count] = find_symbol(token, cmp_terminal);
				right_count++;
				token_pos = 0;
			}

			prods[prod_count].left = left_pos;
			memcpy(prods[prod_count].right, right_arr, sizeof(right_arr[0]) * right_count);
			prods[prod_count].right_lenth = right_count;
			prod_count++;

			part = strtok(NULL, "|");
		}
	}

	// 设置增广产生式
	prods[0].left = start_pos;
	prods[0].right[0] = prods[1].left;
	prods[0].right_lenth = 1;

	if (need_log)
	{
		show_prods();
	}
	printf("产生式加载成功\n");
	fclose(fp);
}

// First集合计算
void Build_First()
{
	// 初始化终结符的First集合
	for (int i = 0; i < symbol_count; i++)
	{
		if (symbol[i].is_terminal == true)
		{
			first[i][i] = true;
		}
	}

	// 迭代计算直到不再变化
	// 遍历所有产生式
	bool change;

	do
	{
		change = false;

		// 遍历产生式右部
		for (int i = 0; i < prod_count; i++)
		{
			// 将右部First加入左部
			int left_pos = prods[i].left;

			// 检查是否能推导出空串
			bool eps_cmp = true;

			// 不能推出空串则终止
			for (int j = 0; j < prods[i].right_lenth; j++)
			{
				int right_pos = prods[i].right[j];
				// 遍历所有符号
				for (int k = 0; k < symbol_count; k++)
				{
					if (first[right_pos][k] && k != eps_pos && !first[left_pos][k])
					{
						first[left_pos][k] = true;
						change = true;
					}
				}

				// 右部都能推出空串
				if (!first[right_pos][eps_pos])
				{
					eps_cmp = false;
					break;
				}
			}

			// 将空串加入左部First集合
			if (eps_cmp && !first[left_pos][eps_pos])
			{
				first[left_pos][eps_pos] = true;
				change = true;
			}
		}
	} while (change);

	if (need_log)
	{
		show_first();
	}
	printf("First集合计算完成\n");
}

// Follow集合计算
void Build_Follow()
{
	int start = prods[0].left;

	// 开始符号
	follow[start][end_pos] = true;

	// 迭代计算直到不再变化
	bool change;

	do
	{
		change = false;
		// 遍历所有产生式
		for (int i = 0; i < prod_count; i++)
		{
			int left_pos = prods[i].left;

			// 遍历产生式右部
			for (int j = 0; j < prods[i].right_lenth; j++)
			{
				int right_pos = prods[i].right[j];

				// 跳过终结符
				if (symbol[right_pos].is_terminal)
				{
					continue;
				}

				// 检查是否能推导出空串
				bool eps_cmp = true;

				// 遍历后续符号
				for (int k = j + 1; k < prods[i].right_lenth; k++)
				{
					int post_pos = prods[i].right[k];
					for (int l = 0; l < symbol_count; l++)
					{
						if (first[post_pos][l] && l != eps_pos && !follow[right_pos][l])
						{
							follow[right_pos][l] = true;
							change = true;
						}
					}
					// 将后续First加入Follow
					if (!first[post_pos][eps_pos])
					{
						eps_cmp = false;
						break;
					}
				}

				// 后续都能推出空串
				if (eps_cmp)
				{
					for (int k = 0; k < symbol_count; k++)
					{
						if (follow[left_pos][k] && !follow[right_pos][k])
						{
							follow[right_pos][k] = true;
							change = true;
						}
					}
				}
			}
		}
	} while (change);
	if (need_log)
	{
		show_follow();
	}
	printf("Follow集合计算完成\n");
}

// Token结构体
// Token结构体定义
typedef struct
{
	int pos;
	int sym;
	char name[Symbol_name_lenth];
	int  value;
}Token;

FILE* token_fp = NULL;
Token tok;


// 打开token文件
void test_token_fp(const char address[])
{
	token_fp = fopen(address, "r");
	if (token_fp == NULL)
	{
		printf("打开token文件失败\n");
		exit(1);
	}
}

// 符号表项结构体
typedef struct
{
	char name[Symbol_name_lenth];
	int type;// 类型
	int value;// 值
	int is_const;// 常量
}Symbol_table;

#define sym_table_lenth 100
Symbol_table sym_table[sym_table_lenth];
int sym_table_count = 0;

// 初始化符号表
void init_sym_table()
{
	sym_table_count = 0;
}

// 查找或插入符号表
int is_exist_sym_table(const char* name, int type, int value)
{
	for (int i = 0; i < sym_table_count; i++)
	{
		if (strcmp(sym_table[i].name, name) == 0)
		{
			return i;// 空
		}
	}
	Symbol_table st;
	strcpy(st.name, name);
	st.type = type;
	st.value = value;
	st.is_const = (type == 1);
	sym_table[sym_table_count] = st;
	sym_table_count++;
	return sym_table_count - 1;
}

// 在符号表中查找
int find_in_symbol_table(const char* name)
{
	for (int i = 0; i < sym_table_count; i++)
	{
		if (strcmp(sym_table[i].name, name) == 0)
		{
			return i;// 空
		}
	}
	return -1;
}

// 四元式结构体
typedef struct Quad
{
	char op[20];//操作
	char arg1[Symbol_name_lenth];//1操作数
	char arg2[Symbol_name_lenth];//2操作数
	char result[Symbol_name_lenth];//结果
} Quad;

Quad quad_table[Quad_Max];
int nextquad = 0;

// 链表节点结构体
typedef struct ListNode
{
	int quad_index;
	struct ListNode* next;
} ListNode;

// 语义记录结构体
typedef struct SemRec
{
	char addr[Symbol_name_lenth];   // 地址/名字
	ListNode* truelist;             // 真值链
	ListNode* falselist;            // 假值链
	ListNode* nextlist;             // 下一条链
	int quad;                       // 四元式编号
} SemRec;

// 生成四元式
SemRec sem_Stack[Sem_Stack_Max];
int sem_pos = -1;

int temp_count = 0;

void emit(const char* op, const char* arg1, const char* arg2, const char* result)
{
	if (nextquad >= Quad_Max)
	{
		printf("四元式表溢出\n");
		exit(1);
	}
	strcpy(quad_table[nextquad].op, op);
	strcpy(quad_table[nextquad].arg1, arg1 ? arg1 : "-");
	strcpy(quad_table[nextquad].arg2, arg2 ? arg2 : "-");
	strcpy(quad_table[nextquad].result, result ? result : "-");
	nextquad++;
}

ListNode* makelist(int quad)
{
	ListNode* node = (ListNode*)malloc(sizeof(ListNode));
	if (node == NULL)
	{
		printf("内存分配失败\n");
		exit(1);
	}
	node->quad_index = quad;
	node->next = NULL;
	return node;
}

ListNode* merge(ListNode* p1, ListNode* p2)
{
	if (p1 == NULL) return p2;
	if (p2 == NULL) return p1;
	ListNode* p = p1;
	while (p->next != NULL)
	{
		p = p->next;
	}
	p->next = p2;
	return p1;
}

void backpatch(ListNode* p, int quad)
{
	char target[20];
	sprintf(target, "%d", quad);
	ListNode* cur = p;
	while (cur != NULL)
	{
		if (cur->quad_index >= 0 && cur->quad_index < Quad_Max)
		{
			strcpy(quad_table[cur->quad_index].result, target);
		}
		cur = cur->next;
	}
}

void newtemp(char* out)
{
	sprintf(out, "t%d", temp_count);
	temp_count++;
}

void push_sem(SemRec rec)
{
	if (sem_pos == Sem_Stack_Max - 1)
	{
		printf("语义栈溢出\n");
		exit(1);
	}
	sem_pos++;
	sem_Stack[sem_pos] = rec;
}

SemRec pop_sem()
{
	if (sem_pos == -1)
	{
		printf("语义栈下溢\n");
		exit(1);
	}
	SemRec rec = sem_Stack[sem_pos];
	sem_pos--;
	return rec;
}

SemRec peek_sem()
{
	if (sem_pos == -1)
	{
		printf("语义栈下溢\n");
		exit(1);
	}
	return sem_Stack[sem_pos];
}

// 创建空语义记录
SemRec empty_rec()
{
	SemRec rec;
	memset(&rec, 0, sizeof(SemRec));
	rec.addr[0] = '\0';
	rec.truelist = NULL;
	rec.falselist = NULL;
	rec.nextlist = NULL;
	rec.quad = -1;
	return rec;
}

// 四元式索引转字符串
const char* quad_op_str(int idx)
{
	static char buf[10];
	if (idx < 0 || idx >= nextquad) return "-";
	sprintf(buf, "%d", idx);
	return buf;
}

// 打印四元式
void print_quads()
{
	printf("\n \t 中间代码\t \n");
	for (int i = 0; i < nextquad; i++)
	{
		printf("%-4d: (%-4s, %-6s, %-6s, %s)\n", i,
			quad_table[i].op,
			quad_table[i].arg1,
			quad_table[i].arg2,
			quad_table[i].result);
	}
	printf("\n");
}

// 写入四元式到文件
void write_quads(const char* address)
{
	FILE* fp = fopen(address, "w");
	if (fp == NULL)
	{
		printf("写入quads.txt: %s\n", address);
		return;
	}
	for (int i = 0; i < nextquad; i++)
	{
		fprintf(fp, "%-4d: (%-4s, %-6s, %-6s, %s)\n", i,
			quad_table[i].op,
			quad_table[i].arg1,
			quad_table[i].arg2,
			quad_table[i].result);
	}
	fclose(fp);
}

// 语义动作处理
void do_semantic_action(int prod_num, SemRec* rhs, int rhs_count, SemRec* result)
{
	Production* p = &prods[prod_num];
	int left = p->left;
	*result = empty_rec();

	const char* left_name = symbol[left].name;

	// M -> eps
	if (strcmp(left_name, "M") == 0)
	{
		result->quad = nextquad;
		return;
	}

	// N -> eps
	if (strcmp(left_name, "N") == 0)
	{
		result->nextlist = makelist(nextquad);
		emit("goto", "-", "-", "_");
		return;
	}

	// P -> eps (过程入口标记)
	if (strcmp(left_name, "P") == 0)
	{
		// ident在语义栈顶下方两个位置(在';'和N之前)
		if (sem_pos >= 2)
		{
			emit("proc", sem_Stack[sem_pos - 2].addr, "-", "-");
		}
		else
		{
			emit("proc", "-", "-", "-");
		}
		result->quad = nextquad - 1;
		return;
	}

	// ConstDef -> ident = number
	if (strcmp(left_name, "ConstDef") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[1]].name, "=") == 0)
	{
		emit(":=", rhs[2].addr, "-", rhs[0].addr);
		return;
	}

	// Program -> Block
	if (strcmp(left_name, "Program") == 0)
	{
		return;
	}

	// Block -> ConstDecl VarDecl ProcDeclList Stmt
	if (strcmp(left_name, "Block") == 0)
	{
		// 回填Stmt的nextlist
		if (rhs[3].nextlist != NULL)
		{
			backpatch(rhs[3].nextlist, nextquad);
		}
		return;
	}

	// StmtList -> Stmt
	if (strcmp(left_name, "StmtList") == 0 && p->right_lenth == 1)
	{
		result->nextlist = rhs[0].nextlist;
		return;
	}

	// StmtList -> StmtList ; M Stmt
	if (strcmp(left_name, "StmtList") == 0 && p->right_lenth == 4)
	{
		backpatch(rhs[0].nextlist, rhs[2].quad);
		result->nextlist = rhs[3].nextlist;
		return;
	}

	// Stmt -> ident := Exp
	if (strcmp(left_name, "Stmt") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[1]].name, ":=") == 0)
	{
		emit(":=", rhs[2].addr, "-", rhs[0].addr);
		result->nextlist = NULL;
		return;
	}

	// Stmt -> call ident
	if (strcmp(left_name, "Stmt") == 0 && p->right_lenth == 2 && strcmp(symbol[p->right[0]].name, "call") == 0)
	{
		emit("call", rhs[1].addr, "-", "-");
		result->nextlist = NULL;
		return;
	}

	// Stmt -> begin StmtList end
	if (strcmp(left_name, "Stmt") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[0]].name, "begin") == 0)
	{
		result->nextlist = rhs[1].nextlist;
		return;
	}

	// Stmt -> if Cond then M Stmt
	if (strcmp(left_name, "Stmt") == 0 && p->right_lenth == 5 && strcmp(symbol[p->right[0]].name, "if") == 0)
	{
		backpatch(rhs[1].truelist, rhs[3].quad);
		result->nextlist = merge(rhs[1].falselist, rhs[4].nextlist);
		return;
	}

	// Stmt -> while M Cond do M Stmt
	if (strcmp(left_name, "Stmt") == 0 && p->right_lenth == 6 && strcmp(symbol[p->right[0]].name, "while") == 0)
	{
		backpatch(rhs[2].truelist, rhs[4].quad);
		backpatch(rhs[5].nextlist, rhs[1].quad);
		emit("goto", "-", "-", "_");
		// 生成跳转指令
		// 回填目标地址
		char target[20];
		sprintf(target, "%d", rhs[1].quad);
		strcpy(quad_table[nextquad - 1].result, target);
		result->nextlist = rhs[2].falselist;
		return;
	}

	// Stmt -> eps
	if (strcmp(left_name, "Stmt") == 0 && p->right_lenth == 1 && p->right[0] == eps_pos)
	{
		result->nextlist = NULL;
		return;
	}

	// Cond -> Exp relop Exp
	if (strcmp(left_name, "Cond") == 0 && p->right_lenth == 3 && symbol[p->right[1]].is_terminal)
	{
		const char* op_name = symbol[p->right[1]].name;
		char jump_op[10];
		if (strcmp(op_name, "=") == 0) strcpy(jump_op, "j=");
		else if (strcmp(op_name, "<>") == 0) strcpy(jump_op, "j<>");
		else if (strcmp(op_name, "<") == 0) strcpy(jump_op, "j<");
		else if (strcmp(op_name, ">") == 0) strcpy(jump_op, "j>");
		else if (strcmp(op_name, "<=") == 0) strcpy(jump_op, "j<=");
		else if (strcmp(op_name, ">=") == 0) strcpy(jump_op, "j>=");
		else strcpy(jump_op, op_name);

		result->truelist = makelist(nextquad);
		emit(jump_op, rhs[0].addr, rhs[2].addr, "_");
		result->falselist = makelist(nextquad);
		emit("goto", "-", "-", "_");
		return;
	}

	// Cond -> odd ident
	if (strcmp(left_name, "Cond") == 0 && p->right_lenth == 2 && strcmp(symbol[p->right[0]].name, "odd") == 0)
	{
		result->truelist = makelist(nextquad);
		emit("jodd", rhs[1].addr, "-", "_");
		result->falselist = makelist(nextquad);
		emit("goto", "-", "-", "_");
		return;
	}

	// Exp -> Exp + Term
	if (strcmp(left_name, "Exp") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[1]].name, "+") == 0)
	{
		newtemp(result->addr);
		emit("+", rhs[0].addr, rhs[2].addr, result->addr);
		return;
	}

	// Exp -> Exp - Term
	if (strcmp(left_name, "Exp") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[1]].name, "-") == 0)
	{
		newtemp(result->addr);
		emit("-", rhs[0].addr, rhs[2].addr, result->addr);
		return;
	}

	// Exp -> Term
	if (strcmp(left_name, "Exp") == 0 && p->right_lenth == 1)
	{
		strcpy(result->addr, rhs[0].addr);
		return;
	}

	// Term -> Term * Factor
	if (strcmp(left_name, "Term") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[1]].name, "*") == 0)
	{
		newtemp(result->addr);
		emit("*", rhs[0].addr, rhs[2].addr, result->addr);
		return;
	}

	// Term -> Term / Factor
	if (strcmp(left_name, "Term") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[1]].name, "/") == 0)
	{
		newtemp(result->addr);
		emit("/", rhs[0].addr, rhs[2].addr, result->addr);
		return;
	}

	// Term -> Factor
	if (strcmp(left_name, "Term") == 0 && p->right_lenth == 1)
	{
		strcpy(result->addr, rhs[0].addr);
		return;
	}

	// Factor -> ident
	if (strcmp(left_name, "Factor") == 0 && p->right_lenth == 1 && strcmp(symbol[p->right[0]].name, "ident") == 0)
	{
		strcpy(result->addr, rhs[0].addr);
		return;
	}

	// Factor -> number
	if (strcmp(left_name, "Factor") == 0 && p->right_lenth == 1 && strcmp(symbol[p->right[0]].name, "number") == 0)
	{
		strcpy(result->addr, rhs[0].addr);
		return;
	}

	// Factor -> ( Exp )
	if (strcmp(left_name, "Factor") == 0 && p->right_lenth == 3 && strcmp(symbol[p->right[0]].name, "(") == 0)
	{
		strcpy(result->addr, rhs[1].addr);
		return;
	}

	// ProcDecl -> procedure ident ; N P Block
	if (strcmp(left_name, "ProcDecl") == 0 && p->right_lenth == 6)
	{
		emit("endproc", "-", "-", "-");
		// 回填N的goto到endproc之后的四元式
		backpatch(rhs[3].nextlist, nextquad);
		return;
	}

	// ProcDeclList -> ProcDecl ; ProcDeclList
	if (strcmp(left_name, "ProcDeclList") == 0 && p->right_lenth == 3)
	{
		return;
	}

	// ProcDeclList -> eps
	if (strcmp(left_name, "ProcDeclList") == 0 && p->right_lenth == 1 && p->right[0] == eps_pos)
	{
		return;
	}

	// ConstDecl / ConstDefList / ConstDef / VarDecl / IdentList: 无四元式
	// 其他产生式不生成四元式
}

// 读取token
int get_next_token()
{
	char token[Symbol_name_lenth];
	// 读取token字符串
	if (fscanf(token_fp, "%s", token) == EOF)
	{
		printf("状态栈\n\n");
		// 文件结束返回结束符
		return end_pos;
	}
	// 查找匹配的符号
	for (int i = 0; i < symbol_count; i++)
	{
		if (strcmp(symbol[i].name, token) == 0)
		{
			tok.pos++;
			tok.sym = i;
			if (need_log)
			{
				printf("匹配到的token: %s\n", symbol[i].name);
			}
			return i;
		}
	}
	// 处理标识符token
	// 处理数字token
	if (isalpha(token[0]))
	{
		is_exist_sym_table(token, 0, 0);
		for (int i = 0; i < symbol_count; i++)
		{
			if (strcmp(symbol[i].name, "ident") == 0)
			{
				tok.pos++;
				tok.sym = i;
				strcpy(tok.name, token);
				if (need_log)
				{
					printf("匹配到的token: %s %s\n", symbol[i].name, token);
				}
				return i;
			}
		}
	}
	else
		// 未知token错误处理
		if (isdigit(token[0]))
		{
			int value = atoi(token);
			is_exist_sym_table(token, 2, value);
			for (int i = 0; i < symbol_count; i++)
			{
				if (strcmp(symbol[i].name, "number") == 0)
				{
					tok.pos++;
					tok.sym = i;
					tok.value = value;
					if (need_log)
					{
						printf("匹配到的token: %s %d\n", symbol[i].name, value);
					}

					return i;
				}
			}
		}

	// 返回错误
	printf("未知token '%s'\n", token);
	return -1;
}

// LR(1)项结构体
#define Item_Max 100
#define Itemset_Max 500

// 项目集结构体
typedef struct
{
	int prod_num;
	int point_pos;
	bool lookahead[Symbol_Max_lenth];
}Item;

// 项目集数组
typedef struct
{
	Item item[Item_Max];
	int item_count;
}Itemset;

// Goto表
Itemset itemset[Itemset_Max];
int itemset_count = 0;

// 检查项是否已在闭包中
int Goto_table[Itemset_Max][Symbol_Max_lenth];

// 计算项目集闭包
bool is_exit_item_closure(Itemset* set, Item* e)
{
	for (int i = 0; i < set->item_count; i++)
	{
		Item* cur = &set->item[i];

		// 遍历项目集中的项
		if (cur->prod_num == e->prod_num && cur->point_pos == e->point_pos)
		{
			// 点号已在末尾则跳过
			bool changed = false;
			for (int k = 0; k < symbol_count; k++)
			{
				if (e->lookahead[k] && !cur->lookahead[k])
				{
					cur->lookahead[k] = true;
					changed = true;
				}
			}
			return true;
		}
	}
	return false;
}


// 计算GOTO转移
void closure(Itemset* set)
{
	// 初始化新项目集
	// 遍历项目集中的项

	// 点号已到达末尾则跳过
	for (int i = 0; i < set->item_count; i++)
	{
		Item it = set->item[i];
		Production p = prods[it.prod_num];
		// 匹配转移符号
		if (it.point_pos == p.right_lenth)
		{
			continue;
		}
		// 点号前进并加入新项目集
		int sym = p.right[it.point_pos];
		// 计算新项目集的闭包
		// 收集点号后的符号序列
		// 初始化Beta的First集合
		int Beta[Production_right_Max_lenth];
		int Beta_count = 0;
		for (int j = it.point_pos + 1; j < p.right_lenth; j++)
		{
			Beta[Beta_count] = p.right[j];
			Beta_count++;
		}
		// 计算Beta的First集合
		bool Beta_first[Symbol_Max_lenth] = { false };
		// 检查Beta是否能推出空串
		bool is_empty = true;
		for (int j = 0; j < Beta_count; j++)
		{
			for (int k = 0; k < symbol_count; k++)
			{
				if (first[Beta[j]][k] && k != eps_pos)
				{
					Beta_first[k] = true;
				}
			}
			// Beta能推出空串则加入lookahead
			if (!first[Beta[j]][eps_pos])
			{
				is_empty = false;
				break;
			}
		}
		// 对非终结符添加新项
		if (is_empty)
		{
			for (int j = 0; j < symbol_count; j++)
			{
				if (it.lookahead[j])
				{
					Beta_first[j] = true;
				}
			}
		}

		if (!symbol[sym].is_terminal)
		{
			// 查找该非终结符的产生式
			for (int j = 0; j < prod_count; j++)
			{
				if (sym == prods[j].left)
				{
					Item new_it;
					for (int k = 0; k < symbol_count; k++)
					{
						new_it.lookahead[k] = Beta_first[k];
					}
					new_it.point_pos = 0;
					Production p1 = prods[j];
					// 空产生式特殊处理
					if (p1.right_lenth == 1 && p1.right[0] == eps_pos)
					{
						new_it.point_pos = p1.right_lenth;
					}
					new_it.prod_num = j;
					if (!is_exit_item_closure(set, &new_it))
					{
						// 加入闭包
						set->item[set->item_count] = new_it;
						set->item_count++;
					}
				}
			}
		}
	}
}

// 状态转移
Itemset GOTO(Itemset* set, int X)
{
	Itemset new_set;
	new_set.item_count = 0;
	for (int i = 0; i < Item_Max; i++)
	{
		memset(new_set.item[i].lookahead, 0, sizeof(new_set.item[i].lookahead));
	}
	for (int i = 0; i < set->item_count; i++)
	{
		Item it = set->item[i];
		Production p = prods[it.prod_num];
		if (it.point_pos >= p.right_lenth)
		{
			continue;
		}
		if (p.right[it.point_pos] == X)
		{
			it.point_pos++;
			new_set.item[new_set.item_count] = it;
			new_set.item_count++;
		}
	}
	closure(&new_set);
	return new_set;
}

// 比较两个项目集（含lookahead）
bool is_same_itemset_lookahead(Itemset* s1, Itemset* s2)
{
	if (s1->item_count != s2->item_count)
		return false;

	for (int i = 0; i < s1->item_count; i++)
	{
		Item* a = &s1->item[i];
		Item* b = &s2->item[i];
		// 比较核心部分
		if (a->prod_num != b->prod_num || a->point_pos != b->point_pos)
			return false;
		// 查找项目集（含lookahead）
		for (int k = 0; k < symbol_count; k++)
		{
			if (a->lookahead[k] != b->lookahead[k])
				return false;
		}
	}
	return true;
}

// 显示项目集
int find_itemset_lookahead(Itemset* set)
{
	for (int i = 0; i < itemset_count; i++)
	{
		if (is_same_itemset_lookahead(set, &itemset[i]))
		{
			return i;
		}
	}
	return -1;
}

// 显示项目集（含展望符）
void show_itemset(Itemset* set)
{
	if (set->item_count == 0)
	{
		printf("项目集为空\n");
		return;
	}
	printf("项目集项:");
	for (int i = 0; i < set->item_count; i++)
	{
		Item it = set->item[i];
		Production p = prods[it.prod_num];
		printf("%s ->", symbol[p.left].name);
		for (int j = 0; j < p.right_lenth; j++)
		{
			if (j == it.point_pos)
			{
				printf(".");
			}
			printf("%s ", symbol[p.right[j]].name);
		}
		if (it.point_pos == p.right_lenth)
		{
			printf(".");
		}
		printf("\n");
	}
}

// 构建所有LALR(1)项目集
void show_itemset_lookahead(Itemset* set)
{
	if (set->item_count == 0)
	{
		printf("项目集为空\n");
		return;
	}
	printf("项目集项:\n");
	for (int i = 0; i < set->item_count; i++)
	{
		Item it = set->item[i];
		Production p = prods[it.prod_num];
		printf("%s ->", symbol[p.left].name);
		for (int j = 0; j < p.right_lenth; j++)
		{
			if (j == it.point_pos)
			{
				printf(".");
			}
			printf("%s ", symbol[p.right[j]].name);
		}
		if (it.point_pos == p.right_lenth)
		{
			printf(".");
		}
		printf(" ");
		int pan = 0;
		for (int j = 0; j < symbol_count; j++)
		{
			if (pan == 0)
			{
				if (it.lookahead[j])
				{
					printf("%s", symbol[j].name);
				}
				pan = 1;
			}
			else
			{
				if (it.lookahead[j])
				{
					printf(",%s", symbol[j].name);
				}
			}
		}
		printf("\n");
	}
}

// 初始化初始项目集
void Build_all_itemset()
{
	// 设置初始项
	Itemset start_state;
	start_state.item[0].prod_num = 0;
	start_state.item[0].point_pos = 0;
	start_state.item_count = 1;
	memset(start_state.item[0].lookahead, 0, sizeof(start_state.item[0].lookahead));
	start_state.item[0].lookahead[end_pos] = true;
	// 计算初始闭包
	closure(&start_state);
	// 保存初始项目集
	itemset[0] = start_state;
	itemset_count = 1;
	if (need_log)
	{
		printf(".0:\n");
		show_itemset_lookahead(&itemset[0]);
	}
	// 遍历所有项目集
	for (int i = 0; i < itemset_count; i++)
	{
		Itemset set = itemset[i];
		// 遍历所有符号
		for (int X = 0; X < symbol_count; X++)
		{
			Itemset next = GOTO(&set, X);
			// 空项目集则跳过
			if (next.item_count == 0)
			{
				continue;
			}
			// 查找是否已存在
			int pos = find_itemset_lookahead(&next);
			// 不存在则添加新项目集
			if (pos == -1)
			{
				pos = itemset_count;
				itemset[pos] = next;
				itemset_count++;
				if (need_log)
				{
					printf(".%d:\n", pos);
					show_itemset_lookahead(&itemset[pos]);
				}
			}
		}
	}

	printf("LALR(1)项目集构建完成\n");
	printf("项目集总数: %d\n", itemset_count);
}

// 比较两个项目集（不含lookahead）
bool is_same_itemset(Itemset* s1, Itemset* s2)
{
	if (s1->item_count != s2->item_count)
	{
		return false;
	}
	for (int i = 0; i < s1->item_count; i++)
	{
		if (s1->item[i].prod_num != s2->item[i].prod_num || s1->item[i].point_pos != s2->item[i].point_pos)
		{
			return false;
		}
	}
	return true;
}

// 合并项目集的lookahead
void merge_itemset(Itemset* to, Itemset* from)
{
	for (int i = 0; i < to->item_count; i++)
	{
		for (int j = 0; j < symbol_count; j++)
		{
			if (from->item[i].lookahead[j])
			{
				to->item[i].lookahead[j] = true;
			}
		}
	}
}

// 合并同心项目集
void merge()
{
	for (int i = 0; i < itemset_count; i++)
	{
		Itemset* it1 = &itemset[i];
		for (int j = i + 1; j < itemset_count; j++)
		{
			Itemset* it2 = &itemset[j];
			if (is_same_itemset(it1, it2))
			{
				// 合并lookahead
				merge_itemset(it1, it2);
				// 删除重复项目集
				for (int k = j; k < itemset_count - 1; k++)
				{
					itemset[k] = itemset[k + 1];
				}
				// 调整数组
				j--;
				itemset_count--;
			}
		}
	}
	printf("状态栈\n\n");
	printf("合并为%d个项目集\n", itemset_count);
	if (need_log)
	{
		for (int i = 0; i < itemset_count; i++)
		{
			printf(".%d:\n", i);
			show_itemset_lookahead(&itemset[i]);
		}

	}
}

// Action表项结构体
#define ACC 0
#define SHIFT 1
#define REDUCE 2
#define ERROR 3

typedef struct
{
	int operation;
	int prod_num;
}Action;
Action Action_table[Itemset_Max][Symbol_Max_lenth];

// 初始化Action表
void init_Action_table()
{
	for (int i = 0; i < itemset_count; i++)
	{
		for (int j = 0; j < symbol_count; j++)
		{
			Action_table[i][j].operation = ERROR;
			Action_table[i][j].prod_num = -1;
		}
	}
}

// 初始化Goto表
void init_goto_table()
{
	for (int i = 0; i < Itemset_Max; i++)
	{
		for (int j = 0; j < symbol_count; j++)
		{
			Goto_table[i][j] = -1;
		}
	}
}

// 填充Goto表
void Build_Goto_table()
{
	// 遍历项目集和符号
	init_goto_table();
	for (int i = 0; i < itemset_count; i++)
	{
		for (int X = 0; X < symbol_count; X++)
		{
			Itemset temp = GOTO(&itemset[i], X);
			// 查找目标项目集
			int pos = -1;
			for (int j = 0; j < itemset_count; j++)
			{
				if (is_same_itemset(&itemset[j], &temp))
				{
					pos = j;
					break;
				}
			}
			Goto_table[i][X] = pos;
		}
	}
}

// Action冲突检测与报错
void error(int set_i, int symbol_X)
{
	if (Action_table[set_i][symbol_X].operation != ERROR)
	{
		printf("状态%d在符号%s上存在Action冲突\n", set_i, symbol[symbol_X].name);
		show_itemset_lookahead(&itemset[set_i]);
		printf("语义栈下溢\n");
		switch (Action_table[set_i][symbol_X].operation)
		{
		case SHIFT: printf("SHIFT(%d)", Action_table[set_i][symbol_X].prod_num); break;
		case REDUCE: printf("REDUCE(%d)", Action_table[set_i][symbol_X].prod_num); break;
		case ACC: printf("ACC"); break;
		}
		printf("检测到Action冲突\n");
		printf("LALR(1)表存在冲突\n");
		exit(1);
	}
}

// 构建Action表
void Build_Action_table()
{
	init_Action_table();
	// 遍历所有项目集
	for (int i = 0; i < itemset_count; i++)
	{
		Itemset set = itemset[i];
		// 遍历项目集中的项
		for (int j = 0; j < set.item_count; j++)
		{
			Item it = set.item[j];
			Production p = prods[it.prod_num];
			int dot = it.point_pos;
			if (dot < p.right_lenth)
			{
				// 点号未到达末尾：移入
				int X = p.right[dot];
				int next_state = Goto_table[i][X];
				if (next_state != -1)
				{
					// 终结符则填移入动作
					if (symbol[X].is_terminal)
					{
						error(i, X);
						Action_table[i][X].operation = SHIFT;
						Action_table[i][X].prod_num = next_state;
					}

				}
			}
			else
			{
				// 接受状态
				if (it.prod_num == 0 && it.lookahead[end_pos])
				{
					// 填归约动作
					error(i, end_pos);
					Action_table[i][end_pos].operation = ACC;
					Action_table[i][end_pos].prod_num = 0;
				}
				else
				{
					// 遍历lookahead填归约
					for (int k = 0; k < symbol_count; k++)
					{
						if (it.lookahead[k])
						{
							error(i, k);
							Action_table[i][k].operation = REDUCE;
							Action_table[i][k].prod_num = it.prod_num;
						}
					}
				}
			}
		}
	}
	printf("LALR(1) ACTION/GOTO表构建完成\n");
}

// 显示Action表
void show_action_table(int state) {
	printf("状态%d Action表:\n", state);
	for (int i = 0; i < symbol_count; i++) {
		if (!symbol[i].is_terminal) continue; // 日志文件
		Action act = Action_table[state][i];
		if (act.operation == ERROR) continue;
		char op[20];
		switch (act.operation) {
		case ACC: strcpy(op, "ACC"); break;
		case SHIFT: sprintf(op, "SHIFT(%d)", act.prod_num); break;
		case REDUCE: sprintf(op, "REDUCE(%d)", act.prod_num); break;
		default: strcpy(op, "ERROR");
		}
		printf("  %s -> %s\n", symbol[i].name, op);
	}
}


// 分析栈定义
#define Stack_Max 100

// 状态栈
int state_Stack[Stack_Max];
int state_pos = -1;

// 符号栈
int symbol_Stack[Stack_Max];
int symbol_pos = -1;

// 压入状态
// 压入符号
void push_state(int data)
{
	if (state_pos == Stack_Max - 1)
	{
		printf("状态栈溢出\n");
		exit(1);
	}
	state_pos++;
	state_Stack[state_pos] = data;
}

void push_symbol(int data)
{
	if (symbol_pos == Stack_Max - 1)
	{
		printf("状态栈溢出\n");
		exit(1);
	}
	symbol_pos++;
	symbol_Stack[symbol_pos] = data;
}

// 弹出状态
int pop_state()
{
	if (state_pos == -1)
	{
		printf("状态栈\n\n");
		exit(1);
	}
	int temp = state_Stack[state_pos];
	state_pos--;
	return temp;
}

int pop_symbol()
{
	if (symbol_pos == -1)
	{
		printf("符号栈溢出\n");
		exit(1);
	}
	int temp = symbol_Stack[symbol_pos];
	symbol_pos--;
	return temp;
}

// 查看状态栈顶
int peek_state()
{
	if (state_pos == -1)
	{
		printf("状态栈\n\n");
		exit(1);
	}
	return state_Stack[state_pos];
}

int peek_symbol()
{
	if (symbol_pos == -1)
	{
		printf("符号栈下溢\n");
		exit(1);
	}
	return symbol_Stack[symbol_pos];
}

// 获取期望的终结符集合
void get_expected_element(int state, char* expected, int len)
{
	expected[0] = '\0';
	int p = 0;
	// 显示分析栈内容
	for (int i = 0; i < symbol_count; i++)
	{
		// 打印状态栈
		if (!symbol[i].is_terminal)
		{
			continue;
		}
		Action act = Action_table[state][i];
		if (act.operation == ACC || act.operation == SHIFT || act.operation == REDUCE)
		{
			if (p)
			{
				strncat(expected, ",", len - strlen(expected) - 1);
			}
			strncat(expected, symbol[i].name, len - strlen(expected) - 1);
			p = 1;
		}
	}

	if (strlen(expected) == 0)
	{
		strncat(expected, "$", len - 1);
	}
}

// 打印符号栈
void all_Stack_show()
{
	printf("文法:\n");
	int p = 1;
	for (int i = 0; i <= state_pos; i++)
	{
		if (p)
		{
			printf("%d", state_Stack[i]);
			p = 0;
		}
		else
		{
			printf(" %d", state_Stack[i]);
		}
	}
	printf("\n符号:");
	p = 1;
	for (int i = 0; i <= symbol_pos; i++)
	{
		int sym = symbol_Stack[i];
		if (p)
		{
			printf("%s", symbol[sym].name);
			p = 0;
		}
		else
		{
			printf(" %s", symbol[sym].name);
		}
	}
	printf("\n");
}

// LALR分析
void LRLA_analyse()
{
	// 初始化分析栈
	symbol_pos = -1;
	state_pos = -1;
	sem_pos = -1;
	push_state(0);
	tok.pos = 0;
	// 初始化符号表
	init_sym_table();
	int a = get_next_token();
	while (1)
	{
		int s = peek_state();
		Action act = Action_table[s][a];// 符号
		switch (act.operation)
		{
		case ACC:
		{
			// 接受
			printf("解析成功\n");
			print_quads();
			write_quads("quads.txt");
			return;
		}
		case SHIFT:
		{
			// 移入
			push_state(act.prod_num);
			push_symbol(a);
			SemRec rec = empty_rec();
			if (strcmp(symbol[a].name, "ident") == 0)
			{
				strcpy(rec.addr, tok.name);
			}
			else if (strcmp(symbol[a].name, "number") == 0)
			{
				sprintf(rec.addr, "%d", tok.value);
			}
			push_sem(rec);
			if (need_log)
			{
				printf("按产生式%d归约, 符号: %s\n", act.prod_num, symbol[a].name);
			}
			a = get_next_token();
			break;
		}
		case REDUCE:
		{
			// 归约
			int prod_num = act.prod_num;
			Production p = prods[prod_num];
			SemRec rhs_sem[Production_right_Max_lenth];
			SemRec result;
			bool is_eps = (p.right_lenth == 1 && p.right[0] == eps_pos);
			if (!is_eps)
			{
				for (int i = p.right_lenth - 1; i >= 0; i--)
				{
					rhs_sem[i] = pop_sem();
				}
				for (int i = 0; i < p.right_lenth; i++)
				{
					pop_state();
					pop_symbol();
				}
			}
			do_semantic_action(prod_num, rhs_sem, p.right_lenth, &result);
			// 执行语义动作
			int A = p.left;
			push_symbol(A);
			push_sem(result);
			// 压入归约结果
			int new_state = Goto_table[peek_state()][A];
			push_state(new_state);
			if (need_log)
			{
				printf("意外的符号:");
				show_prod(prod_num);
			}
			break;
		}
		default:
		{
			// 语法错误处理
			printf("语法错误\n");
			char expect[Expect_len] = { 0 };
			get_expected_element(s, expect, strlen(expect));
			printf("Token位置 %d, token %s\n", tok.pos, symbol[tok.sym].name);
			printf("意外的: %s\n", symbol[a].name);
			printf("状态%d, 期望的token: %s", s, expect);
			return;
		}
		}
		if (need_log)
		{
			all_Stack_show();
		}
	}
}

// 写入日志
void write_log(const char* address)
{
	FILE* fp = fopen(address, "w");
	if (fp == NULL)
	{
		printf("Writing log: %s\n", address);
		return;
	}
	// 写入产生式到日志
	fprintf(fp, "文法:\n");
	for (int i = 0; i < prod_count; i++)
	{
		Production p = prods[i];
		fprintf(fp, "%s ->", symbol[p.left].name);
		for (int j = 0; j < p.right_lenth; j++)
		{
			fprintf(fp, " %s", symbol[p.right[j]].name);
		}
		fprintf(fp, "\n");
	}
	// First/Follow集合
	fprintf(fp, "First集合:\n");
	for (int i = 0; i < symbol_count; i++)
	{
		if (!symbol[i].is_terminal)
		{
			fprintf(fp, "%s = {", symbol[i].name);
			int p = 1;
			for (int j = 0; j < symbol_count; j++)
			{
				if (first[i][j])
				{
					if (p)
					{
						fprintf(fp, "%s", symbol[j].name);
						p = 0;
					}
					else
					{
						fprintf(fp, ",%s", symbol[j].name);
					}
				}
			}
			fprintf(fp, "}\n");
		}
	}
	fprintf(fp, "Follow集合:\n");
	for (int i = 0; i < symbol_count; i++)
	{
		if (!symbol[i].is_terminal)
		{
			fprintf(fp, "%s = {", symbol[i].name);
			int p = 1;
			for (int j = 0; j < symbol_count; j++)
			{
				if (follow[i][j])
				{
					if (p)
					{
						fprintf(fp, "%s", symbol[j].name);
						p = 0;
					}
					else
					{
						fprintf(fp, ",%s", symbol[j].name);
					}
				}
			}
			fprintf(fp, "}\n");
		}
	}
	// 写入项目集到日志
	fprintf(fp, "文法:\n");
	for (int i = 0; i < itemset_count; i++)
	{
		Itemset set = itemset[i];
		fprintf(fp, ".%d:\n", i);
		for (int j = 0; j < set.item_count; j++)
		{
			Item it = set.item[j];
			// 写入项目
			Production p = prods[it.prod_num];
			int point_pos = it.point_pos;
			fprintf(fp, "%s -> ", symbol[p.left].name);
			for (int k = 0; k < p.right_lenth; k++)
			{
				if (k == point_pos)
				{
					fprintf(fp, ". ");
				}
				fprintf(fp, "%s ", symbol[p.right[k]].name);
			}
			if (point_pos == p.right_lenth)
			{
				fprintf(fp, ".");
			}
			fprintf(fp, " | ");
			// 写入展望符
			int q = 1;
			for (int j = 0; j < symbol_count; j++)
			{
				if (it.lookahead[j])
				{
					if (q)
					{
						fprintf(fp, "%s", symbol[j].name);
						q = 0;
					}
					else
					{
						fprintf(fp, " %s", symbol[j].name);
					}
				}
			}
			fprintf(fp, "\n");
		}
		for (int j = 0; j < symbol_count; j++)
		{
			if (symbol[j].is_terminal)
			{
				Action act = Action_table[i][j];
				if (act.operation == ERROR)
				{
					continue;
				}
				char op[20];
				switch (act.operation)
				{
				case ACC: strcpy(op, "ACC"); break;
				case SHIFT: sprintf(op, "SHIFT(%d)", act.prod_num); break;
				case REDUCE: sprintf(op, "REDUCE(%d)", act.prod_num); break;
				default: strcpy(op, "ERROR");
				}
				fprintf(fp, "  %s -> %s\n", symbol[j].name, op);
			}
		}
		fprintf(fp, "\n");
	}
	// 分隔线
	fprintf(fp, " ACTION / GOTO 表 \n");

	// 分类终结符和非终结符
	int terms[100], t_cnt = 0;
	int nterms[100], nt_cnt = 0;
	for (int i = 0; i < symbol_count; i++) {
		if (symbol[i].is_terminal)
			terms[t_cnt++] = i;
		else
			nterms[nt_cnt++] = i;
	}

	// 打印表头
	fprintf(fp, "I | ");
	for (int i = 0; i < t_cnt; i++)
		fprintf(fp, "%-10s", symbol[terms[i]].name);

	fprintf(fp, " | ");
	for (int i = 0; i < nt_cnt; i++)
		fprintf(fp, "%-10s", symbol[nterms[i]].name);
	fprintf(fp, "\n");

	// 分隔线
	fprintf(fp, "-----|");
	for (int i = 0; i < t_cnt; i++)
		fprintf(fp, "----------");

	fprintf(fp, "|");
	for (int i = 0; i < nt_cnt; i++)
		fprintf(fp, "----------");
	fprintf(fp, "\n");

	// 遍历所有状态打印表
	for (int st = 0; st < itemset_count; st++) {
		fprintf(fp, " %-4d| ", st);

		// 打印Action部分
		for (int i = 0; i < t_cnt; i++) {
			Action a = Action_table[st][terms[i]];
			char buf[10] = "-";
			if (a.operation == SHIFT)
				sprintf(buf, "s%d", a.prod_num);
			else if (a.operation == REDUCE)
				sprintf(buf, "r%d", a.prod_num);
			else if (a.operation == ACC)
				strcpy(buf, "acc");
			fprintf(fp, "%-10s", buf);
		}

		fprintf(fp, " | ");

		// 打印Goto部分
		for (int i = 0; i < nt_cnt; i++) {
			int v = Goto_table[st][nterms[i]];
			char buf[10];
			if (v == -1)
				strcpy(buf, "-");
			else
				sprintf(buf, "%d", v);
			fprintf(fp, "%-10s", buf);
		}
		fprintf(fp, "\n");
	}

	// 关闭日志文件
	fclose(fp);
}

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);

	char Prod_address[address_lenth];
	char Token_address[address_lenth];
	char Log_address[] = "log.txt";
	Prod_address[0] = '\0';
	Token_address[0] = '\0';

	bool has_cmd_args = (argc >= 4);
	if (has_cmd_args)
	{
		if (strcmp(argv[1], "yes") == 0 || strcmp(argv[1], "Yes") == 0)
		{
			need_log = true;
		}
		else
		{
			need_log = false;
		}
		strcpy(Prod_address, argv[2]);
		strcpy(Token_address, argv[3]);
	}
	else
	{
		printf("需要日志输出吗? (yes/no): ");
		char str[4];
		scanf("%s", str);
		if (strcmp(str, "yes") == 0 || strcmp(str, "Yes") == 0)
		{
			need_log = true;
			printf("日志已启用\n");
		}
		else
		{
			need_log = false;
			printf("日志已关闭\n");
		}
		printf("请输入文法文件: ");
		scanf("%s", Prod_address);
		printf("请输入测试文件: ");
		scanf("%s", Token_address);
	}

	fprintf(stderr, "[步骤] 加载文法\n");
	load_Prods(Prod_address);
	fprintf(stderr, "[步骤] 构建First集合\n");
	Build_First();
	fprintf(stderr, "[步骤] 构建Follow集合\n");
	Build_Follow();
	fprintf(stderr, "[步骤] 构建项目集\n");
	Build_all_itemset();
	fprintf(stderr, "[步骤] 合并\n");
	merge();
	fprintf(stderr, "[步骤] 构建Goto表\n");
	Build_Goto_table();
	fprintf(stderr, "[步骤] 构建Action表\n");
	Build_Action_table();
	fprintf(stderr, "[步骤] 写入日志\n");
	write_log(Log_address);
	fprintf(stderr, "[步骤] 打开token文件\n");
	test_token_fp(Token_address);
	fprintf(stderr, "[步骤] 开始分析\n");
	printf("开始LALR分析\n\n");
	LRLA_analyse();
	if (token_fp != NULL)
	{
		fclose(token_fp);
	}

	return 0;
}
