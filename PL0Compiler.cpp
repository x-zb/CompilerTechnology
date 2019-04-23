//Author:Zhongbin Xie   要修改的地方：  3.错误处理函数加上打印出错行源代码
#include<stdio.h>
#include<string.h>
#include"state.h"


char source[MAX_SOURCE_FILE + 1];
int pos = 0;
int line = 1;
int line_start = 0;


char sym[MAX_KEY_WORD_LEN];
char id[MAX_IDENT_LEN + 1] = { 0 };
int num;
char ch;
char str[MAX_STRING_LEN + 1];

char key[20][MAX_KEY_WORD_LEN] = { "const", "var", "array", "of", "integer",              //20个保留字
                                   "char", "procedure", "function", "for", "if",
                                   "then", "else", "do", "while", "to",
                                   "downto", "begin", "end", "read", "write" };
char keysym[20][MAX_SYM_LEN] = { "CONSTSYM", "VARSYM", "ARRAYSYM", "OFSYM", "INTSYM", //保留字的类型
                                      "CHARSYM", "PROCSYM", "FUNCSYM", "FORSYM", "IFSYM",
                                      "THENSYM", "ELSESYM", "DOSYM", "WHILESYM", "TOSYM",
                                      "DOWNTOSYM", "BEGINSYM", "ENDSYM", "READSYM", "WRITESYM" };
char singal[15] = { '.', ',', ';', '=', '+', '-', ':', '[', ']', '(', ')', '*', '/', '<', '>' };     //单个字符token的类型
char ssym[15][MAX_SYM_LEN] = { "PERIOD", "COMMA", "SEMICOLON", "EQU", "PLUS", "MINUS", "COLON", "LBRACK", "RBRACK",
                                    "LPAREN", "RPAREN", "TIMES", "SLASH", "LSS", "GTR" };
//其余token的类型 ":="     ,"<>" ,"<=" ,">=" ,"整数"  ,"字符"     ,"字符串","标识符"，"程序结束" 
//                "BECOMES","NEQ","LEQ","GEQ","NUMBER","CHARACTER","STRING","IDENT" ，"END"

extern QFRT code2[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE];
extern  int cx2[MAX_LEVEL + 1];


//==========================符号表定义与操作=======================

ITEM table[MAX_ITEM_NUM];     //符号表
int t_pos = -1;               //指向当前符号表项
int pre_t_pos[MAX_LEVEL+1];   //上一层分程序最后一个表项的索引，用于实现栈式符号表的出栈；第0层的pre_t_pos在program()中初始化为-1

int level = -1;               //当前分程序的层次
int para_num = 0;             //当前分程序的参数个数
int var_num = 0;              //当前分程序局部变量个数
int dx;                       //栈中下一个要分配给局部变量的相对地址（相对当前ebp），在vardeclaration()中初始化为para_num+12
int px;                       //栈中下一个要分配给参数的相对地址，在procdeclaration()中初始化为2
int xt;                       //栈中上一个分配给临时变量的相对地址，在sub_program()内composition_state()前初始化为bpara_num+bvar_num+11

b_ITEM btab[MAX_ITEM_NUM];      //过程表（存过程和函数信息）
int bt_pos = 0;                 //指向当前过程,0层过程也占一项          
int bpx;         //当前过程的参数表（bp_list）的索引，在parameter_list()中初始化为-1

int lookup(char *str){        //逆序查表，若查到返回索引，否则返回-1
	int i = t_pos;
	while (strcmp(table[i].name, str) != 0 && i >= 0){
		i--;
	}
	return i;
}

void set_item(KIND kind, TYPE type){
	t_pos++;
	strcpy(table[t_pos].name, id);
	table[t_pos].kind = kind;
	table[t_pos].type = type;
	table[t_pos].lev = level;
	switch (kind){
	case CONST:if (type == INT)
				   table[t_pos].val = num;
			   else if (type == CHAR)
				   table[t_pos].val = ch;
		break;
	case VAR:table[t_pos].val = dx; break;
	case PROC:
	case FUNC:table[t_pos].val = bt_pos+1; break;
	case PARA:
	case VPARA:table[t_pos].val = px; table[t_pos].lev++; break;
	}
}

void copy_item(ITEM &x1,ITEM x2){
	strcpy(x1.name , x2.name);
	x1.kind = x2.kind;
	x1.type = x2.type;
	x1.lev = x2.lev;
	x1.val = x2.val;
	x1.array_type = x2.array_type;
	x1.array_length = x2.array_length;
}



void set_bitem(){
	bt_pos++;
	strcpy(btab[bt_pos].proc_name, id);
	btab[bt_pos].blev = level + 1;
	//btab[bt_pos].bpara_num = para_num;
}

char stab[50][MAX_STRING_LEN + 1];  //字符串表（存字符串和标号）
int st_pos = -1;                    //指向当前字符串
char lab[7] = "LABEL`";             //最多允许26个标号a-z

void set_sitem(char *strr){
	st_pos++;
	strcpy(stab[st_pos], strr);
}

//=======================四元式定义与操作================================



QFRT code[MAX_LEVEL+1][2 * MAX_SOURCE_FILE];        //code用来存放各层次的中间代码
int cx[MAX_LEVEL+1];                                //cx[i]为各层次中间代码的当前位置
int max_l;                                         //生成四元式的最大层次，code和code2的最大层次是一样的，故只需一个变量

int tempx = 0;                           //记录当前临时变量个数（同时也是临时变量下标，从1开始），在composition()中清零



void gen_code(int prog_level,OPERATOR opt,ITEM op1,ITEM op2,ITEM op3){
	//printf("level:%d\n", prog_level);
	cx[prog_level]++;
	code[prog_level][cx[prog_level]].opt = opt;

	code[prog_level][cx[prog_level]].operand[0].kind = op1.kind;
	code[prog_level][cx[prog_level]].operand[0].type = op1.type;
	code[prog_level][cx[prog_level]].operand[0].lev = op1.lev;
	code[prog_level][cx[prog_level]].operand[0].val = op1.val;
	
	code[prog_level][cx[prog_level]].operand[1].kind = op2.kind;
	code[prog_level][cx[prog_level]].operand[1].type = op2.type;
	code[prog_level][cx[prog_level]].operand[1].lev = op2.lev;
	code[prog_level][cx[prog_level]].operand[1].val = op2.val;

	code[prog_level][cx[prog_level]].operand[2].kind = op3.kind;
	code[prog_level][cx[prog_level]].operand[2].type = op3.type;
	code[prog_level][cx[prog_level]].operand[2].lev = op3.lev;
	code[prog_level][cx[prog_level]].operand[2].val = op3.val;
}

void gen_code(int prog_level, OPERATOR opt, ITEM op1, ITEM op2){
	//printf("level:%d\n", prog_level);
	cx[prog_level]++;
	code[prog_level][cx[prog_level]].opt = opt;

	code[prog_level][cx[prog_level]].operand[0].kind = op1.kind;
	code[prog_level][cx[prog_level]].operand[0].type = op1.type;
	code[prog_level][cx[prog_level]].operand[0].lev = op1.lev;
	code[prog_level][cx[prog_level]].operand[0].val = op1.val;

	code[prog_level][cx[prog_level]].operand[1].kind = op2.kind;
	code[prog_level][cx[prog_level]].operand[1].type = op2.type;
	code[prog_level][cx[prog_level]].operand[1].lev = op2.lev;
	code[prog_level][cx[prog_level]].operand[1].val = op2.val;

	code[prog_level][cx[prog_level]].operand[2].kind = K_NULL;
}

void gen_code(int prog_level, OPERATOR opt, ITEM op1){
	//printf("level:%d\n", prog_level);
	cx[prog_level]++;
	code[prog_level][cx[prog_level]].opt = opt;

	code[prog_level][cx[prog_level]].operand[0].kind = op1.kind;
	code[prog_level][cx[prog_level]].operand[0].type = op1.type;
	code[prog_level][cx[prog_level]].operand[0].lev = op1.lev;
	code[prog_level][cx[prog_level]].operand[0].val = op1.val;

	code[prog_level][cx[prog_level]].operand[1].kind = K_NULL;
	code[prog_level][cx[prog_level]].operand[2].kind = K_NULL;	
}

void initial_cx(){                       //在prgram()中将各层的cx初始化为-1
	for (int i = 0; i <= MAX_LEVEL; i++){
		cx[i] = -1;
	}
}

void get_maxl(){                        //program()运行结束，生成中间代码后调用，得到所生成中间代码的最大层次max_l
	max_l = 0;
	while (cx[max_l] != -1 && max_l <= MAX_LEVEL){
		max_l++;
	}
	max_l--;   //最大层次
	//printf("最大层次：%d\n", max_l);//用于调试
}

/*char *num2str(int x){
	char str[MAX_INTEGER_DIGIT + 1] = {0};
	int a = x;
	int i = 0;
	while (a > 0){
		a = a / 10;
		i++;
	}
	while (i > 0){
		str[i - 1] = x % 10;
		x = x / 10;
		i--;
	}
	return str;
}*/

//=================================================================================

int errnum = 0;

void error(char* errmsg){
	printf("\n");
	printf("LEN:%d COL:%d %s\n", line, pos - line_start + 1, errmsg);
	printf("\n");
	errnum++;
}



//=================================================================================
void getsym(){
	int i;
	if (source[pos] == '\0'){
		strcpy(sym, "END");
	}
	else if (source[pos] == '\n'){
		line++;
		pos++;
		line_start = pos;
		getsym();
	}
	else if (source[pos] == ' '||source[pos]=='\t'){
		do
		pos++;
		while (source[pos] == ' '||source[pos]=='\t');
		getsym();
	}
	else if ((source[pos] >= 'a'&&source[pos] <= 'z') || (source[pos] >= 'A'&&source[pos] <= 'Z')){
		i = 0;
		do{
			id[i] = source[pos];
			i++;
			pos++;
		} while (((source[pos] >= 'a'&&source[pos] <= 'z') ||
			(source[pos] >= 'A'&&source[pos] <= 'Z') ||
			(source[pos] >= '0'&&source[pos] <= '9')) && i < MAX_IDENT_LEN);
		if ((source[pos] >= 'a'&&source[pos] <= 'z') ||
			(source[pos] >= 'A'&&source[pos] <= 'Z') ||
			(source[pos] >= '0'&&source[pos] <= '9')){
			error("Identifier is too long!");                    //若标识符超出最大长度，只截取最大长度以内部分作为标识符，跳至下一个不是字母或数字的字符
			do{
				pos++;
			} while ((source[pos] >= 'a'&&source[pos] <= 'z') ||
				(source[pos] >= 'A'&&source[pos] <= 'Z') ||
				(source[pos] >= '0'&&source[pos] <= '9'));
		}
		id[i] = '\0';
		for (i = 0; i < 20; i++){
			if (strcmp(id, key[i]) == 0){
				break;
			}
		}
		if (i == 20){
			strcpy(sym, "IDENT");
		}
		else {
			strcpy(sym, keysym[i]);
		}
	}
	else if (source[pos] >= '0'&&source[pos] <= '9'){
		num = 0;
		i = 1;
		do{
			num = num * 10 + source[pos] - '0';
			i++;
			pos++;
		} while (source[pos] >= '0'&&source[pos] <= '9'&& i <= MAX_INTEGER_DIGIT);
		if (source[pos] >= '0'&&source[pos] <= '9'){
			error("Constant integer is too long!");              //若整数超出最大长度，只截取最大长度以内的部分作为整数，跳至下一个不是数字的字符
			do{
				pos++;
			} while (source[pos] >= '0'&&source[pos] <= '9');
		}
		strcpy(sym, "NUMBER");
	}
	else if (source[pos] == '\''){
		if (source[pos + 2] == '\''){
			if ((source[pos + 1] >= 'a'&&source[pos + 1] <= 'z') ||
				(source[pos + 1] >= 'A'&&source[pos + 1] <= 'Z') ||
				(source[pos + 1] >= '0'&&source[pos + 1] <= '9')){
				strcpy(sym, "CHARACTER");
				ch = source[pos + 1];
				pos = pos + 3;
			}
			else{
				pos++;
				error("Illegal constant character!");
				pos += 2;
			}
		}
		else{
			pos += 2;
			error("More than one character or missing singal quotation marks !");
			do{
				pos++;
			} while (!(source[pos] == '\'' || source[pos] == ',' || source[pos] == ';'));
			pos++;
			getsym();
		}
	}
	else if (source[pos] == '\"'){
		i = 0;
		pos++;
		while ((source[pos] == ' ' || source[pos] == '!' || (source[pos] >= 35 && source[pos] <= 126)) && i < MAX_STRING_LEN){
			str[i] = source[pos];
			i++;
			pos++;
		}
		if (source[pos] == '\"'){
			strcpy(sym, "STRING");
			str[i] = '\0';
			pos++;
		}
		else {
			if (i == MAX_STRING_LEN)//若出现非法字符或字符串超长，跳至下一个';'或'"'后
				error("Constant string is too long!");
			else
				error("Illegal character in constant string!");
			do{
				pos++;
			} while (!(source[pos] == '"' || source[pos] == ';'));
			pos++;
			getsym();
		}
	}
	else if (source[pos] == ':'&&source[pos + 1] == '='){
		strcpy(sym, "BECOMES");
		pos += 2;
	}
	else if (source[pos] == '<'&&source[pos + 1] == '>'){
		strcpy(sym, "NEQ");
		pos += 2;
	}
	else if (source[pos] == '<'&&source[pos + 1] == '='){
		strcpy(sym, "LEQ");
		pos += 2;
	}
	else if (source[pos] == '>'&&source[pos + 1] == '='){
		strcpy(sym, "GEQ");
		pos += 2;
	}
	else {
		for (i = 0; i < 15; i++){
			if (source[pos] == singal[i])
				break;
		}
		if (i != 15){
			strcpy(sym, ssym[i]);
			pos++;
		}
		else{
			error("Illegal character in source file!");
			pos++;
			getsym();
		}
	}
}


void jump(){   //跳至分号、逗号或'\0'(程序结束)
	while (strcmp(sym, "SEMICOLON") != 0 && strcmp(sym, "COMMA") != 0 && strcmp(sym, "END") != 0 && strcmp(sym, "PERIOD") != 0){
		getsym();
	}
}



//=========================常量声明部分==================================
void constant(){                                                        //识别<常量定义>
	char sign[MAX_SYM_LEN+1] = {0};//
	if (strcmp(sym, "IDENT") == 0){
		getsym();
		if (strcmp(sym, "EQU") == 0){
			getsym();
			if (strcmp(sym, "PLUS") == 0 || strcmp(sym, "MINUS") == 0){
				strcpy(sign, sym); //
				getsym();
			}
			if (strcmp(sym, "NUMBER") == 0){
				if (strcmp(sign, "MINUS") == 0)//
					num = -num;       //
				if (lookup(id) != -1 && table[lookup(id)].lev == level){  //填表时查表，检查同层重复定义
					error("同层常量标识符重复定义！");
					jump();
				}
				else{
					set_item(CONST, INT);//
					getsym();
				}
			}
			else if (strcmp(sym, "CHARACTER") == 0){
				if (lookup(id) != -1 && table[lookup(id)].lev == level){  //填表时查表，检查同层重复定义
					error("同层常量标识符重复定义！");
					jump();
				}
				else{
					set_item(CONST, CHAR);//
					getsym();
				}
			}
			else {
				error("常量定义中“=”右应为常量！");
				jump();
			}
		}
		else {
			error("常量定义中标识符后应为 '=' ！");
			jump();
		}
	}
	else {
		error("常量标识符非法！");
		jump();
	}

}

void constdeclaration(){                             //识别<常量声明部分>
	int errnum_in = errnum;
	if (strcmp(sym, "CONSTSYM") == 0){
		do{
			do{
				getsym();
				constant();
			} while (strcmp(sym, "COMMA") == 0);
			if (strcmp(sym, "IDENT") == 0){
				error("常量定义之间漏逗号！");
				constant();
			}
		} while (strcmp(sym, "COMMA") == 0);
		if (strcmp(sym, "SEMICOLON") == 0){
			getsym();
			if (errnum == errnum_in){
				printf("This is a constant declaration!\n");
			}
		}
		else {
			error("常量声明部分末尾漏分号！");
		}
	}
}

//=======================变量声明部分=============================

void basic_type(TYPE &type){
	if (strcmp(sym, "INTSYM") == 0){
		type = INT;//
	}
	else if (strcmp(sym, "CHARSYM") == 0){
		type = CHAR;//
	}
	else{
		error("非法基本类型！");
		jump();
	}
}


void var_type(int tx){
	TYPE type;
	if (strcmp(sym, "ARRAYSYM") == 0){
		getsym();
		if (strcmp(sym, "LBRACK") == 0){
			getsym();
			if (strcmp(sym, "NUMBER") == 0){
				getsym();
				if (strcmp(sym, "RBRACK") == 0){
					getsym();
					if (strcmp(sym, "OFSYM") == 0){
						getsym();
						basic_type(type);
						dx -= t_pos - tx ;
						/*if (num < 0){
							error("数组上界不能为负数！");
							jump();
							return;
						}*/
						for (int i = tx+1; i <= t_pos; i++){
							table[i].type = ARRAY;
							table[i].val = dx;
							dx += num;
							table[i].array_type = type;
							table[i].array_length = num;
						}
						getsym();
					}
				}
			}
			else {
				error("数组上界应为无符号整数！");
				jump();
			}
		}
	}
	else{
		basic_type(type);
		for (int i = tx+1; i <= t_pos; i++){
			table[i].type = type;
		}
		getsym();
	}
}


void variable(){
	int tx = t_pos;//
	if (strcmp(sym, "IDENT") == 0){
		do{
			if (lookup(id) != -1 && table[lookup(id)].lev == level){  //填表时查表，检查同层重复定义
				error("同层变量标识符重复定义！");
				jump();//此时sym="COMMA"或"COLON"
			}
			else{
				set_item(VAR, NUL);//
				dx++;//
				getsym();
			}
			if (strcmp(sym, "COMMA") == 0){
				getsym();
			}
			else if (strcmp(sym, "IDENT") == 0){
				error("变量之间漏分号！");
			}
		} while (strcmp(sym, "IDENT") == 0);
		if (strcmp(sym, "COLON") == 0){
			getsym();
			var_type(tx);//		
		}
		else {
			error("变量定义缺少类型！");
			jump();
			getsym();
		}
	}
	else {
		error("应为标识符！");
		jump();
		getsym();
	}
}


void vardeclaration(){
	int errnum_in = errnum;
	var_num = 0;//
	dx = btab[bt_pos].bpara_num+ 12;   //每个过程（分程序）的第一个变量从para_num+12处开始存储，其中para_num为本层参数个数
	if (strcmp(sym, "VARSYM") == 0){
		getsym();
		while (strcmp(sym, "IDENT") == 0){
			variable();
			if (strcmp(sym, "SEMICOLON") == 0)
				getsym();
			else{
				error("变量说明后漏分号！");
			}
		}
		var_num = dx - 12 - para_num;//
		btab[bt_pos].bvar_num = var_num;//
		if (errnum_in == errnum){
			printf("This is a variable declaration!\n");
		}
	}
}

//====================过程和函数声明部分==============================
void  parameter_seg(){
	TYPE type_1;                   //存参数类型
	int tx = t_pos ;               //记录当前符号表索引，用于返填符号表中参数类型
	KIND PARA_KIND=PARA;           //记录参数种类：PARAorVPARA
	if (strcmp(sym, "VARSYM") == 0){
		PARA_KIND=VPARA;
		getsym();
	}
	if (strcmp(sym, "IDENT") == 0){
		do{
			if (lookup(id) != -1 && table[lookup(id)].lev == level+1){  //填表时查表，检查同层重复定义
				error("同层参数名重复定义！");
				jump();
			}
			else{
				set_item(PARA_KIND, NUL);//
				px++;//
				para_num++;//
				getsym();
			}
			if (strcmp(sym, "COMMA") == 0){
				getsym();
			}
			else if (strcmp(sym, "IDENT") == 0){
				error("参数之间漏分号！");
			}
		} while (strcmp(sym, "IDENT") == 0);
		if (strcmp(sym, "COLON") == 0){
			getsym();
			basic_type(type_1);//
			for (int i = tx + 1; i <= t_pos; i++){//反填符号表
				table[i].type = type_1;
			}
			for (int i=bpx+1; i <= bpx+para_num; i++){//填过程参数表
				btab[bt_pos].bp_list[i].pkind = PARA_KIND;
				btab[bt_pos].bp_list[i].ptype = type_1;
			}
			bpx = bpx + para_num;//更新bpx
			getsym();
		}
		else {
			error("缺少参数类型！");
			jump();
			getsym();
		}
	}
}

void parameter_list(){
	para_num = 0;//
	px = 2;//
	bpx=-1;//
	if (strcmp(sym, "LPAREN") == 0){             //对于没有形参表的过程，该函数直接跳过
		getsym();
		if (strcmp(sym, "RPAREN") == 0){
			error("形参表括号内没有参数！");
			jump();
			return;;
		}
		do{
			parameter_seg();
			if (strcmp(sym, "SEMICOLON") == 0){
				getsym();
			}
			else if (strcmp(sym, "IDENT") == 0 || strcmp(sym, "VARSYM") == 0){
				error("参数段之间漏分号！");
			}
		} while (strcmp(sym, "IDENT") == 0 || strcmp(sym, "VARSYM") == 0);
		if (strcmp(sym, "RPAREN") == 0){
			getsym();
		}
		else {
			error("形参列表缺少右括号！");
			jump();
		}
	}
	btab[bt_pos].bpara_num = para_num;//
}

void proc_head(){                    //strcmp(sym, "PROCSYM") == 0
	int errnum_in = errnum;
	getsym();
	if (strcmp(sym, "IDENT") == 0){
		if (lookup(id) != -1 && table[lookup(id)].lev == level){  //填表时查表，检查同层重复定义
			error("同层过程名重复定义！");
		}
		else{
			set_item(PROC, NUL);//
			gen_code(level + 1, proc, table[t_pos]); //
			set_bitem();//bt_pos指向当前过程
			pre_t_pos[level + 1] = t_pos;//
		}
		getsym();
		parameter_list();           //parameter_list()运行结束后，bpara_num为形式参数表中形参个数	
	}
	else{
		error("过程名非法！");
		jump();
	}
	if (strcmp(sym, "SEMICOLON") == 0){
		getsym();
	}
	else{
		error("过程首部后漏分号！");
	}
	if (errnum_in == errnum){
		printf("This is a procedure head!\n");
	}
}

void procdeclaration(){
	int errnum_in = errnum;
	void sub_program();                  //函数声明，产生间接递归调用
	if (strcmp(sym, "PROCSYM") == 0){
		do{
			proc_head();                 //proc_head()运行后，bpara_num为当前分程序参数个数
			sub_program();               //分析当前分程序，此时bpara_num为当前分程序参数个数
			if (strcmp(sym, "SEMICOLON") == 0){
				getsym();
			}
			else if (strcmp(sym, "PROCSYM") == 0){
				error("过程说明部分之间漏分号！");
			}
			else {
				error("过程说明部分后漏分号！");
			}
		} while (strcmp(sym, "PROCSYM") == 0);
		if (errnum_in == errnum){
			printf("This is a procedure declaration !\n");
		}
	}
}

void func_head(){                    //strcmp(sym, "FUNCSYM") == 0
	int errnum_in = errnum;
	TYPE type_2;   //
	int tx_1 = t_pos;
	getsym();
	if (strcmp(sym, "IDENT") == 0){
		if (lookup(id) != -1 && table[lookup(id)].lev == level){  //填表时查表，检查同层重复定义
			error("同层函数名重复定义！");
		}
		else{
			set_item(FUNC, NUL);     //
			gen_code(level + 1, proc, table[t_pos]);//
			set_bitem();      //
			pre_t_pos[level + 1] = t_pos;//
		}
		getsym();
		parameter_list();
	}
	else{
		error("过程名错误！");
		jump();
	}
	if (strcmp(sym, "COLON") == 0){
		getsym();
		basic_type(type_2);//
		table[tx_1+1].type = type_2;//
		getsym();
		if (strcmp(sym, "SEMICOLON") == 0){
			getsym();
		}
		else{
			error("过程首部后漏分号！");
			jump();
			getsym();
		}
	}
	else{
		error("过程首部缺少类型说明！");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a function head!\n");
	}
}

void funcdeclaration(){
	int errnum_in = errnum;
	void sub_program();
	if (strcmp(sym, "FUNCSYM") == 0){
		do{
			func_head();
			sub_program();
			if (strcmp(sym, "SEMICOLON") == 0){
				getsym();
			}
			else if (strcmp(sym, "PROCSYM") == 0){
				error("过程说明部分之间漏分号！");
			}
			else {
				error("过程说明部分后漏分号！");
			}
		} while (strcmp(sym, "FUNCSYM") == 0);
		if (errnum_in == errnum){
			printf("This is a function declaration !\n");
		}
	}
}

//========================语句及复合语句================================

//==========赋值语句==========

void argument_list(int index_0){
	void expression(ITEM &);
	ITEM x;
	int i = 0;
	if (strcmp(sym, "LPAREN") == 0){             //对于没有实参表的过程，该函数直接跳过
		getsym();
		if (strcmp(sym, "RPAREN") == 0){
			error("实参表括号内没有参数！");
			jump();
			return;
		}
		do{
			expression(x);//
			if (x.type != btab[table[index_0].val].bp_list[i++].ptype){
				if (i > btab[table[index_0].val].bpara_num){
					error("实参个数过多！");
				}
				else{
					error("实参与形参类型不一致！");
				}
				while (strcmp(sym, "SEMICOLON") != 0){
					getsym();
				}
				return;
			}
			gen_code(level, arg, x);//
			if (strcmp(sym, "COMMA") == 0){
				getsym();
			}
			else if (strcmp(sym, "PLUS") == 0 || strcmp(sym, "MINUS") == 0||
				     strcmp(sym, "IDENT") == 0 || strcmp(sym, "NUMBER") == 0 || strcmp(sym, "LPAREN") == 0){
				error("参数段之间漏分号！");
			}
		} while (strcmp(sym, "PLUS") == 0 || strcmp(sym, "MINUS") == 0 ||
			     strcmp(sym, "IDENT") == 0 || strcmp(sym, "NUMBER") == 0 || strcmp(sym, "LPAREN") == 0);
		if (strcmp(sym, "RPAREN") == 0){
			getsym();
			if (i < btab[table[index_0].val].bpara_num){
				error("实参个数过少！");
			}
		}
		else {
			error("缺少右括号！");
			jump();
		}
	}
}

void factor(ITEM &x){
	void expression(ITEM &);                          //函数声明，间接递归调用
	int iden_index;
	ITEM x1,op1;
	if (strcmp(sym, "IDENT") == 0){
		iden_index = lookup(id);//
		if (iden_index == -1){ //引用时查表，未定义报错
			error("标识符未定义（因子）！");
			jump();
			return;
		}
		getsym();
		if (strcmp(sym, "LBRACK") == 0){
			getsym();
			expression(x1);
			if ((x1.kind == IMM || x1.kind == CONST) && (x1.val >= table[iden_index].array_length || x1.val<0)){
				error("数组的整数或常量下标越界（数组元素为右值）！");
				jump();
				return;
			}
			if (strcmp(sym, "RBRACK") == 0){
				getsym();
			}
			else {
				error("数组下标缺少右方括号！");
			}
		}
		else if (strcmp(sym, "LPAREN") == 0){
			argument_list(iden_index);
		}
		//////////////////////////////////////////////////////////////////
		if (table[iden_index].kind == CONST){ //因子可以为字符常量！！！
			x.kind = CONST;//
			x.type = table[iden_index].type;//
			x.lev = level;//
			x.val = table[iden_index].val;//
		}
		else if (table[iden_index].type == ARRAY){
			op1.kind = TEMP;
			op1.type = table[iden_index].array_type;
			op1.lev = ++tempx;
			op1.val = xt + tempx;
			gen_code(level, arrayr, op1, table[iden_index], x1);//
			copy_item(x, op1);//
		}
		else if (table[iden_index].kind == VAR || table[iden_index].kind == PARA || table[iden_index].kind == VPARA){
			copy_item(x, table[iden_index]);//
		}
		else if (table[iden_index].kind == FUNC){
			op1.kind = TEMP;
			op1.type = table[iden_index].type;
			op1.lev = ++tempx;
			op1.val = xt + tempx;
			gen_code(level, call, table[iden_index],op1);//
			copy_item(x, op1);//
		}
		/////////////////////////////////////////////////////////////////////
	}
	else if (strcmp(sym, "LPAREN") == 0){
		getsym();
		expression(x); //
		if (strcmp(sym, "RPAREN") == 0){
			getsym();
		}
		else{
			error("因子表达式漏右括号！");
		}
	}
	else if (strcmp(sym, "NUMBER") == 0){
		x.kind=IMM;//
		x.type = INT;//
		x.lev = -1;//
		x.val = num;//
		getsym();
	}
	else if (strcmp(sym, "CHARACTER") == 0){//因子不能为字符！！！
		error("因子不能为字符！");
		jump();
	}
	else{
		error("因子非法！");
		jump();
	}
}

void term(ITEM &x_1){
	ITEM x_2,op1;
	factor(x_1);
	while (strcmp(sym, "TIMES") == 0 || strcmp(sym, "SLASH") == 0){
		char MULT[6];
		strcpy(MULT, sym);
		getsym();
		factor(x_2);
		op1.kind = TEMP;//
		op1.type = INT;//
		op1.lev = ++tempx;//
		op1.val = xt + tempx;//
		if (strcmp(MULT, "TIMES") == 0){
			gen_code(level, mul,op1, x_1, x_2);//
			copy_item(x_1,op1);//
		}
		else if (strcmp(MULT, "SLASH") == 0){
			gen_code(level, div, op1, x_1, x_2);//
			copy_item(x_1, op1);//
		}
	}
}

void expression(ITEM &x1){
	ITEM x2,op1;//
	char SIGN[6];
	if (strcmp(sym, "PLUS") == 0 || strcmp(sym, "MINUS") == 0){
		strcpy(SIGN, sym);
		getsym();
	}
	term(x1);
	while (strcmp(sym, "PLUS") == 0 || strcmp(sym, "MINUS") == 0){
		char PLUS[6];
		strcpy(PLUS, sym);//
		getsym();
		term(x2);//
		op1.kind = TEMP;//
		op1.type = INT;//
		op1.lev = ++tempx;//
		op1.val = xt + tempx;//
		if (strcmp(PLUS, "PLUS") == 0){
			gen_code(level, add, op1, x1,x2);//
			copy_item(x1, op1);//
		}
		else if (strcmp(PLUS, "MINUS") == 0){
			gen_code(level, sub,op1,x1,x2);//
			copy_item(x1, op1);//
		}
	}
	if (strcmp(SIGN, "MINUS") == 0){
		op1.kind = TEMP;//
		op1.type = INT;//
		op1.lev = ++tempx;//
		op1.val = xt + tempx;//
		gen_code(level, neg, op1, x1);//
		copy_item(x1, op1);//
	}
}

void assign_state(int ident_index_1){
	int errnum_in = errnum;
	ITEM x1,x2,op1,op2;
	//if (strcmp(sym, "IDENT") == 0){
	
		//getsym();
		if (strcmp(sym, "LBRACK") == 0){     
			getsym();
			expression(x1);//
			if (strcmp(sym, "RBRACK") == 0){
				getsym();
			}
			else{
				error("缺少右方括号！");
			}
		}
		if (strcmp(sym, "BECOMES") == 0){
			getsym();
			expression(x2);//
			///////////////////////////////////////////////////////////
			if (table[ident_index_1].kind == FUNC){
				op1.kind = VAR;
				op1.type = table[ident_index_1].type;
				op1.lev = level;
				op1.val = 0;//op1为函数返回值
				if (x2.kind == TEMP || x2.kind == CONST || x2.kind == IMM){
					gen_code(level, assign, op1, x2);
				}
				else{
					op2.kind = TEMP;
					op2.type = x2.type;
					op2.lev = ++tempx;
					op2.val = xt + tempx;
					gen_code(level, assign, op2, x2);
					gen_code(level, assign, op1, op2);
				}				
			}
			else if (table[ident_index_1].type == ARRAY){
				if ((x1.kind == IMM || x1.kind == CONST)&&(x1.val>=table[ident_index_1].array_length||x1.val<0)){
					error("数组的整数或常量下标越界（数组元素为左值）！");
					jump();
					return;
				}				
				if (x2.kind == TEMP || x2.kind == CONST || x2.kind == IMM){
					gen_code(level, arrayl, table[ident_index_1], x1, x2);
				}
				else{
					op2.kind = TEMP;
					op2.type = x2.type;
					op2.lev = ++tempx;
					op2.val = xt + tempx;
					gen_code(level, assign, op2, x2);
					gen_code(level, arrayl, table[ident_index_1], x1, op2);
				}
			}
			else if (table[ident_index_1].kind == VAR || table[ident_index_1].kind == PARA || table[ident_index_1].kind == VPARA){
				if (x2.kind == TEMP || x2.kind == CONST || x2.kind == IMM){
					gen_code(level, assign, table[ident_index_1], x2);
				}
				else{
					op2.kind = TEMP;
					op2.type = x2.type;
					op2.lev = ++tempx;
					op2.val = xt + tempx;
					gen_code(level, assign, op2, x2);
					gen_code(level, assign, table[ident_index_1], op2);
				}
			}
			else{
				error("左值错误！");
			}
			//////////////////////////////////////////////////////////
		}
		else{
			error("缺少赋值符号！");
			jump();
		}
	//}
	//else{
	//	error("赋值语句应以标识符开始！");
	//	jump();
	//	getsym();
    //}
	if (errnum_in == errnum){
		printf("This is an assignment statement!\n");
	}
}

//=========条件语句============

void condition(char *log_opr_1){
	int errnum_in = errnum;
	ITEM x1,x2;//
	expression(x1);
	if (strcmp(sym, "EQU") == 0 || strcmp(sym, "NEQ") == 0 || strcmp(sym, "LEQ") == 0 ||
		strcmp(sym, "GEQ") == 0 || strcmp(sym, "LSS") == 0 || strcmp(sym, "GTR") == 0){
		strcpy(log_opr_1, sym);//
		getsym();
	}
	else{
		error("比较运算符非法！");
		jump();
	}
	expression(x2);
	gen_code(level, cmp, x1, x2);//
	if (errnum_in == errnum){
		printf("This is a conditional !\n");
	}
}

void if_state(){
	int errnum_in = errnum;
	void statement();
	char log_opr[4];//
	ITEM op1,op2;//
	getsym();
	condition(log_opr);//
	/////////////////////////////////////////////////
	lab[5]++;
	set_sitem(lab);
	op1.kind = STRING;
	op1.type = NUL;
	op1.lev = -1;
	op1.val = st_pos;
	if (strcmp(log_opr, "EQU") == 0){
		gen_code(level, jne, op1);
	}
	else if (strcmp(log_opr, "NEQ") == 0){
		gen_code(level, je, op1);
	}
	else if (strcmp(log_opr, "LEQ") == 0){
		gen_code(level, jg, op1);
	}
	else if (strcmp(log_opr, "GEQ") == 0){
		gen_code(level, jl, op1);
	}
	else if (strcmp(log_opr, "LSS") == 0){
		gen_code(level, jge, op1);
	}
	else if (strcmp(log_opr, "GTR") == 0){
		gen_code(level, jle, op1);
	}
	///////////////////////////////////////////////////////
	if (strcmp(sym, "THENSYM") == 0){
		getsym();
		statement();
		if (strcmp(sym, "ELSESYM") == 0){
			//////////////////////////////////////
			lab[5]++;
			set_sitem(lab);
			op2.kind = STRING;
			op2.type = NUL;
			op2.lev = -1;
			op2.val = st_pos;
			gen_code(level, jmp, op2);
			gen_code(level, label, op1);
			/////////////////////////////////////
			getsym();
			statement();
			gen_code(level, label, op2);//
		}
		else{
			gen_code(level, label, op1);//
		}
	}
	else{
		error("缺少then！");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a if_conditional statement!\n");
	}
}

//=========当循环语句==========

void do_while_state(){	
	int errnum_in = errnum;
	void statement(); 
	///////////////////////////////////////	
	char log_opr[4];
	ITEM op1;
	lab[5]++;
	set_sitem(lab);
	op1.kind = STRING;
	op1.type = NUL;
	op1.lev = -1;
	op1.val = st_pos;
	gen_code(level, label, op1);
	//////////////////////////////////////
	getsym();
	statement();
	if (strcmp(sym, "WHILESYM") == 0){
		getsym();
		condition(log_opr);
		///////////////////////////////////////
		if (strcmp(log_opr, "EQU") == 0){
			gen_code(level, je, op1);
		}
		else if (strcmp(log_opr, "NEQ") == 0){
			gen_code(level, jne, op1);
		}
		else if (strcmp(log_opr, "LEQ") == 0){
			gen_code(level, jle, op1);
		}
		else if (strcmp(log_opr, "GEQ") == 0){
			gen_code(level, jge, op1);
		}
		else if (strcmp(log_opr, "LSS") == 0){
			gen_code(level, jl, op1);
		}
		else if (strcmp(log_opr, "GTR") == 0){
			gen_code(level, jg, op1);
		}
		///////////////////////////////////////
	}
	else{
		error("do_while语句中缺少关键词while!");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a when_loop statement!\n");
	}
}

//========for循环语句==========
void for_loop_state(){
	int errnum_in = errnum;
	void statement(); 
	int iden_index;//
	ITEM x1,x2,op1,op2;//
	char TO_DOWNTO[10];
	getsym();
	if (strcmp(sym, "IDENT") == 0){
		iden_index = lookup(id);//
		if (iden_index == -1){ //引用时查表，未定义报错
			error("标识符未定义（for循环变量）！");
			while (strcmp(sym, "SEMICOLON") != 0){
				getsym();
			}
			return;
		}
		getsym();
		if (strcmp(sym, "BECOMES") == 0){
			getsym();
			expression(x1);//
			if (strcmp(sym, "TOSYM") == 0 || strcmp(sym, "DOWNTOSYM") == 0){
				strcpy(TO_DOWNTO, sym);//
				getsym();
				expression(x2);
				////////////////////////////////////////
				gen_code(level, cmp, x1, x2);
				lab[5]++;
				set_sitem(lab);
				op1.kind = STRING;
				op1.type = NUL;
				op1.lev = -1;
				op1.val = st_pos;
				if (strcmp(TO_DOWNTO, "TOSYM") == 0){
					gen_code(level,jg , op1);
				}
				else{
					gen_code(level,jl , op1);
				}
				/////////////////////////////////////////
				if (strcmp(sym, "DOSYM") == 0){
					///////////////////////////////////
					gen_code(level, assign, table[iden_index], x1);
					lab[5]++;
					set_sitem(lab);
					op2.kind = STRING;
					op2.type = NUL;
					op2.lev = -1;
					op2.val = st_pos;
					gen_code(level, label, op2);
					///////////////////////////////////
					getsym();
					statement();
					//////////////////////////////////
					if (strcmp(TO_DOWNTO, "TOSYM") == 0){
						gen_code(level, loopup, table[iden_index], x2, op2);
					}
					else{
						gen_code(level, loopdown, table[iden_index], x2, op2);
					}
					gen_code(level, label, op1);
					//////////////////////////////////
				}
				else{
					error("for循环语句中缺少关键字do！");
					jump();
				}
			}
			else{
				error("for循环语句中缺少关键字to或者downto！");
				jump();
			}
		}
	}
	else{
		error("for循环语句中关键字for后应为标识符！");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a for_loop statement!\n");
	}
}

//========过程调用语句=========
//void call_state(){

//}

//==========读语句=============
void read_state(){
	int errnum_in = errnum;
	void statement();
	int index_3;//
	getsym();
	if (strcmp(sym, "LPAREN") == 0){
		getsym();
		if (strcmp(sym, "IDENT") == 0){
			do{
				index_3 = lookup(id);//
				if (index_3 == -1){ //引用时查表，未定义报错
					error("标识符未定义（读语句）！");
					while (strcmp(sym, "SEMICOLON") != 0){
						getsym();
					}
					return;
				}
				if(table[index_3].kind != VAR&&table[index_3].kind != PARA&&table[index_3].kind != VPARA){
					error("读语句参数只能为变量或参数！");
					while (strcmp(sym, "SEMICOLON") != 0){
						getsym();
					}
					return;
				}
				gen_code(level, read, table[index_3]);//
				getsym();
				if (strcmp(sym, "COMMA") == 0){
					getsym();
				}
				else if (strcmp(sym, "IDENT") == 0){
					error("读语句参数之间漏逗号！");
				}
			} while (strcmp(sym, "IDENT") == 0);
			if (strcmp(sym, "RPAREN") == 0){
				getsym();
			}
			else{
				error("读语句参数表缺少右括号！");
				jump();
			}
		}
		else{
			error("读语句参数表内不是标识符或读语句没有参数！");
			jump();
		}
	}
	else{
		error("读语句缺少参数表！");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a read statement!\n");
	}
}

//==========写语句=============
void write_state(){	
	int errnum_in = errnum;
	void statement(); 
	ITEM x,op1;
	getsym();
	if (strcmp(sym, "LPAREN") == 0){
		getsym();
		if (strcmp(sym, "STRING") == 0){
			st_pos++;//
			strcpy(stab[st_pos], str);//
			op1.kind = STRING;
			op1.type = NUL;
			op1.lev = -1;
			op1.val = st_pos;
			getsym();
			if (strcmp(sym, "COMMA") == 0){
				getsym();
				expression(x);//
				gen_code(level, write, op1, x);//
			}
			else{
				gen_code(level, write, op1);//
			}
		}
		else{
			expression(x);
			gen_code(level, write, x);//
		}
		if (strcmp(sym, "RPAREN") == 0){
			getsym();
		}
		else{
			error("写语句参数列表缺少右括号！");
			jump();
		}
	}
	else{
		error("写语句缺少参数列表！");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a write statement!\n");
	}
}

//==========复合语句===========
void composition_state(){
	void statement();
	int errnum_in = errnum;
	if (strcmp(sym, "BEGINSYM") != 0){
		error("复合语句缺少begin！");
	}
	getsym();
	statement();
	while (strcmp(sym, "SEMICOLON") == 0){
		getsym();
		if (strcmp(sym, "ENDSYM") == 0){
			//error("复合语句最后一个子句后多分号！");//复合语句最后一个子句可以有分号
			break;
		}
		statement();
	}
	//printf("%s\n", sym);  //用于调试
	if (strcmp(sym, "ENDSYM") == 0){
		getsym();
		if (errnum_in == errnum){
			printf("This is a composition statement!\n");
		}
	}
	else{
		error("复合语句后漏end！");
		jump();
		getsym();
	}
}

//==========语句===============
void statement(){
	int errum_in_call_state ;
	int ident_index;//
	if (strcmp(sym, "IDENT") == 0){
		ident_index = lookup(id);//
		if (ident_index == -1){ //引用时查表，未定义报错
			error("标识符未定义（赋值语句或过程调用语句）！");
			while (strcmp(sym, "SEMICOLON") != 0){
				getsym();
			}
			return;
		}
		if (table[ident_index].kind == CONST){
			error("不能改变常量的值！");
			while (strcmp(sym, "SEMICOLON") != 0){
				getsym();
			}
			return;
		}
		getsym();
		if (strcmp(sym, "LBRACK") == 0 || strcmp(sym, "BECOMES") == 0){
			assign_state(ident_index);//
		}
		else if (strcmp(sym, "EQU") == 0){
			error("赋值语句中赋值符号写成等号！");
			jump();
		}
		else {
			errum_in_call_state = errnum;
			if (strcmp(sym, "LPAREN") == 0){
				argument_list(ident_index);
			}
			gen_code(level, call, table[ident_index]);//过程调用
			
			if (errum_in_call_state == errnum){
				printf("This is a call statement!\n");
			}
		}
	}
	else if (strcmp(sym, "IFSYM") == 0){
		if_state();
	}
	else if (strcmp(sym, "DOSYM") == 0){
		do_while_state();
	}
	else if (strcmp(sym, "FORSYM") == 0){
		for_loop_state();
	}
	else if (strcmp(sym, "READSYM") == 0){
		read_state();
	}
	else if (strcmp(sym, "WRITESYM") == 0){
		write_state();
	}
	else if (strcmp(sym, "BEGINSYM") == 0){
		composition_state();
	}
	else{
		error("语句非法！");
		jump();
	}
}


//=============================================================================
void sub_program(){
	level++;//
	if(level > MAX_LEVEL){
		error("分程序层次超过最大允许层次！");
		jump();
		return;
	}
	int t_px = t_pos-btab[bt_pos].bpara_num;//本层分程序过程名在符号表中的索引
	int bt_px = bt_pos;//本层分程序在过程表中的索引
	constdeclaration();
	vardeclaration();
	while (strcmp(sym, "PROCSYM") == 0 || strcmp(sym, "FUNCSYM") == 0){
		if (strcmp(sym, "PROCSYM") == 0){
			procdeclaration();
		}
		else{
			funcdeclaration();
		}
	}
	tempx = 0;//
	xt = btab[bt_px].bpara_num + btab[bt_px].bvar_num + 11;//
	composition_state();
	btab[bt_px].bper_num = tempx;//
	if (level > 0){
		gen_code(level, ret, table[t_px]);//
	}
	//print_table();//打印本分程序所用的栈式符号表，用于调试
	t_pos = pre_t_pos[level];//
	level--;//
}

void program(char *filename){
	printf("Compiling %s ...\n", filename);
	btab[0].bpara_num = 0;//
	initial_cx();//
	pre_t_pos[0] = -1;//
	getsym();
	sub_program();
	//printf("%s\n", sym);  //用于调试
	if (strcmp(sym, "PERIOD") != 0){
		error("Missing '.' at tne end of the program!");
	}
	getsym();
	if (strcmp(sym, "END") != 0){
		error("Part of the program is afer '.'!");
	}
	if (errnum == 0){
		printf("This is a PL/0 program !\n\n\n");
	}
	else{
		printf("%d errors detected!\n\n\n",errnum);
	}
}

//===============================================

int main(int argc, const char *argv[]){
	char source_filename[MAX_FILENAME + 1];
	
	printf("请输入源文件路径和文件名(输入e退出程序)：\n");
	scanf("%s", source_filename);
	if (strcmp(source_filename, "e") == 0){
		return -1;
	}

	FILE *fpin;
	
	while ((fpin = fopen(source_filename, "r")) == NULL){
		printf("无法打开源文件%s!\n",source_filename);
		printf("请输入源文件路径和文件名(输入e退出程序)：\n");
		scanf("%s", source_filename);
		if (strcmp(source_filename, "e") == 0){
			return -1;
		}
	}

	

	int i = 0;                    //将源程序读入数组source[]
	while (!feof(fpin)){
		if (i <= MAX_SOURCE_FILE){
			source[i] = fgetc(fpin);
			i++;
		}
		else{
			printf("Source file is too large!\n");
			return -1;
		}
	}
	source[i - 1] = '\0';
	
	//print_token();//打印词法分析结果，用于调试
	program(source_filename);
	//print_btab();//打印过程表，用于调试
	//print_stab();//打印字符串和标号表，用于调试
	get_maxl();
	if (errnum == 0){
		//print_qfmt(code,cx);//打印四元式，用于调试
		optimize();
		//print_qfmt(code2,cx2);//打印优化后的四元式，用于调试
		gen_asmfile(code2,cx2);
	}

	fclose(fpin);

	return 0;
}