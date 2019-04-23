//Author:Zhongbin Xie   Ҫ�޸ĵĵط���  3.�����������ϴ�ӡ������Դ����
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

char key[20][MAX_KEY_WORD_LEN] = { "const", "var", "array", "of", "integer",              //20��������
                                   "char", "procedure", "function", "for", "if",
                                   "then", "else", "do", "while", "to",
                                   "downto", "begin", "end", "read", "write" };
char keysym[20][MAX_SYM_LEN] = { "CONSTSYM", "VARSYM", "ARRAYSYM", "OFSYM", "INTSYM", //�����ֵ�����
                                      "CHARSYM", "PROCSYM", "FUNCSYM", "FORSYM", "IFSYM",
                                      "THENSYM", "ELSESYM", "DOSYM", "WHILESYM", "TOSYM",
                                      "DOWNTOSYM", "BEGINSYM", "ENDSYM", "READSYM", "WRITESYM" };
char singal[15] = { '.', ',', ';', '=', '+', '-', ':', '[', ']', '(', ')', '*', '/', '<', '>' };     //�����ַ�token������
char ssym[15][MAX_SYM_LEN] = { "PERIOD", "COMMA", "SEMICOLON", "EQU", "PLUS", "MINUS", "COLON", "LBRACK", "RBRACK",
                                    "LPAREN", "RPAREN", "TIMES", "SLASH", "LSS", "GTR" };
//����token������ ":="     ,"<>" ,"<=" ,">=" ,"����"  ,"�ַ�"     ,"�ַ���","��ʶ��"��"�������" 
//                "BECOMES","NEQ","LEQ","GEQ","NUMBER","CHARACTER","STRING","IDENT" ��"END"

extern QFRT code2[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE];
extern  int cx2[MAX_LEVEL + 1];


//==========================���ű��������=======================

ITEM table[MAX_ITEM_NUM];     //���ű�
int t_pos = -1;               //ָ��ǰ���ű���
int pre_t_pos[MAX_LEVEL+1];   //��һ��ֳ������һ�����������������ʵ��ջʽ���ű�ĳ�ջ����0���pre_t_pos��program()�г�ʼ��Ϊ-1

int level = -1;               //��ǰ�ֳ���Ĳ��
int para_num = 0;             //��ǰ�ֳ���Ĳ�������
int var_num = 0;              //��ǰ�ֳ���ֲ���������
int dx;                       //ջ����һ��Ҫ������ֲ���������Ե�ַ����Ե�ǰebp������vardeclaration()�г�ʼ��Ϊpara_num+12
int px;                       //ջ����һ��Ҫ�������������Ե�ַ����procdeclaration()�г�ʼ��Ϊ2
int xt;                       //ջ����һ���������ʱ��������Ե�ַ����sub_program()��composition_state()ǰ��ʼ��Ϊbpara_num+bvar_num+11

b_ITEM btab[MAX_ITEM_NUM];      //���̱�����̺ͺ�����Ϣ��
int bt_pos = 0;                 //ָ��ǰ����,0�����Ҳռһ��          
int bpx;         //��ǰ���̵Ĳ�����bp_list������������parameter_list()�г�ʼ��Ϊ-1

int lookup(char *str){        //���������鵽�������������򷵻�-1
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

char stab[50][MAX_STRING_LEN + 1];  //�ַ��������ַ����ͱ�ţ�
int st_pos = -1;                    //ָ��ǰ�ַ���
char lab[7] = "LABEL`";             //�������26�����a-z

void set_sitem(char *strr){
	st_pos++;
	strcpy(stab[st_pos], strr);
}

//=======================��Ԫʽ���������================================



QFRT code[MAX_LEVEL+1][2 * MAX_SOURCE_FILE];        //code������Ÿ���ε��м����
int cx[MAX_LEVEL+1];                                //cx[i]Ϊ������м����ĵ�ǰλ��
int max_l;                                         //������Ԫʽ������Σ�code��code2���������һ���ģ���ֻ��һ������

int tempx = 0;                           //��¼��ǰ��ʱ����������ͬʱҲ����ʱ�����±꣬��1��ʼ������composition()������



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

void initial_cx(){                       //��prgram()�н������cx��ʼ��Ϊ-1
	for (int i = 0; i <= MAX_LEVEL; i++){
		cx[i] = -1;
	}
}

void get_maxl(){                        //program()���н����������м�������ã��õ��������м����������max_l
	max_l = 0;
	while (cx[max_l] != -1 && max_l <= MAX_LEVEL){
		max_l++;
	}
	max_l--;   //�����
	//printf("����Σ�%d\n", max_l);//���ڵ���
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
			error("Identifier is too long!");                    //����ʶ��������󳤶ȣ�ֻ��ȡ��󳤶����ڲ�����Ϊ��ʶ����������һ��������ĸ�����ֵ��ַ�
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
			error("Constant integer is too long!");              //������������󳤶ȣ�ֻ��ȡ��󳤶����ڵĲ�����Ϊ������������һ���������ֵ��ַ�
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
			if (i == MAX_STRING_LEN)//�����ַǷ��ַ����ַ���������������һ��';'��'"'��
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


void jump(){   //�����ֺš����Ż�'\0'(�������)
	while (strcmp(sym, "SEMICOLON") != 0 && strcmp(sym, "COMMA") != 0 && strcmp(sym, "END") != 0 && strcmp(sym, "PERIOD") != 0){
		getsym();
	}
}



//=========================������������==================================
void constant(){                                                        //ʶ��<��������>
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
				if (lookup(id) != -1 && table[lookup(id)].lev == level){  //���ʱ������ͬ���ظ�����
					error("ͬ�㳣����ʶ���ظ����壡");
					jump();
				}
				else{
					set_item(CONST, INT);//
					getsym();
				}
			}
			else if (strcmp(sym, "CHARACTER") == 0){
				if (lookup(id) != -1 && table[lookup(id)].lev == level){  //���ʱ������ͬ���ظ�����
					error("ͬ�㳣����ʶ���ظ����壡");
					jump();
				}
				else{
					set_item(CONST, CHAR);//
					getsym();
				}
			}
			else {
				error("���������С�=����ӦΪ������");
				jump();
			}
		}
		else {
			error("���������б�ʶ����ӦΪ '=' ��");
			jump();
		}
	}
	else {
		error("������ʶ���Ƿ���");
		jump();
	}

}

void constdeclaration(){                             //ʶ��<������������>
	int errnum_in = errnum;
	if (strcmp(sym, "CONSTSYM") == 0){
		do{
			do{
				getsym();
				constant();
			} while (strcmp(sym, "COMMA") == 0);
			if (strcmp(sym, "IDENT") == 0){
				error("��������֮��©���ţ�");
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
			error("������������ĩβ©�ֺţ�");
		}
	}
}

//=======================������������=============================

void basic_type(TYPE &type){
	if (strcmp(sym, "INTSYM") == 0){
		type = INT;//
	}
	else if (strcmp(sym, "CHARSYM") == 0){
		type = CHAR;//
	}
	else{
		error("�Ƿ��������ͣ�");
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
							error("�����Ͻ粻��Ϊ������");
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
				error("�����Ͻ�ӦΪ�޷���������");
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
			if (lookup(id) != -1 && table[lookup(id)].lev == level){  //���ʱ������ͬ���ظ�����
				error("ͬ�������ʶ���ظ����壡");
				jump();//��ʱsym="COMMA"��"COLON"
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
				error("����֮��©�ֺţ�");
			}
		} while (strcmp(sym, "IDENT") == 0);
		if (strcmp(sym, "COLON") == 0){
			getsym();
			var_type(tx);//		
		}
		else {
			error("��������ȱ�����ͣ�");
			jump();
			getsym();
		}
	}
	else {
		error("ӦΪ��ʶ����");
		jump();
		getsym();
	}
}


void vardeclaration(){
	int errnum_in = errnum;
	var_num = 0;//
	dx = btab[bt_pos].bpara_num+ 12;   //ÿ�����̣��ֳ��򣩵ĵ�һ��������para_num+12����ʼ�洢������para_numΪ�����������
	if (strcmp(sym, "VARSYM") == 0){
		getsym();
		while (strcmp(sym, "IDENT") == 0){
			variable();
			if (strcmp(sym, "SEMICOLON") == 0)
				getsym();
			else{
				error("����˵����©�ֺţ�");
			}
		}
		var_num = dx - 12 - para_num;//
		btab[bt_pos].bvar_num = var_num;//
		if (errnum_in == errnum){
			printf("This is a variable declaration!\n");
		}
	}
}

//====================���̺ͺ�����������==============================
void  parameter_seg(){
	TYPE type_1;                   //���������
	int tx = t_pos ;               //��¼��ǰ���ű����������ڷ�����ű��в�������
	KIND PARA_KIND=PARA;           //��¼�������ࣺPARAorVPARA
	if (strcmp(sym, "VARSYM") == 0){
		PARA_KIND=VPARA;
		getsym();
	}
	if (strcmp(sym, "IDENT") == 0){
		do{
			if (lookup(id) != -1 && table[lookup(id)].lev == level+1){  //���ʱ������ͬ���ظ�����
				error("ͬ��������ظ����壡");
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
				error("����֮��©�ֺţ�");
			}
		} while (strcmp(sym, "IDENT") == 0);
		if (strcmp(sym, "COLON") == 0){
			getsym();
			basic_type(type_1);//
			for (int i = tx + 1; i <= t_pos; i++){//������ű�
				table[i].type = type_1;
			}
			for (int i=bpx+1; i <= bpx+para_num; i++){//����̲�����
				btab[bt_pos].bp_list[i].pkind = PARA_KIND;
				btab[bt_pos].bp_list[i].ptype = type_1;
			}
			bpx = bpx + para_num;//����bpx
			getsym();
		}
		else {
			error("ȱ�ٲ������ͣ�");
			jump();
			getsym();
		}
	}
}

void parameter_list(){
	para_num = 0;//
	px = 2;//
	bpx=-1;//
	if (strcmp(sym, "LPAREN") == 0){             //����û���βα�Ĺ��̣��ú���ֱ������
		getsym();
		if (strcmp(sym, "RPAREN") == 0){
			error("�βα�������û�в�����");
			jump();
			return;;
		}
		do{
			parameter_seg();
			if (strcmp(sym, "SEMICOLON") == 0){
				getsym();
			}
			else if (strcmp(sym, "IDENT") == 0 || strcmp(sym, "VARSYM") == 0){
				error("������֮��©�ֺţ�");
			}
		} while (strcmp(sym, "IDENT") == 0 || strcmp(sym, "VARSYM") == 0);
		if (strcmp(sym, "RPAREN") == 0){
			getsym();
		}
		else {
			error("�β��б�ȱ�������ţ�");
			jump();
		}
	}
	btab[bt_pos].bpara_num = para_num;//
}

void proc_head(){                    //strcmp(sym, "PROCSYM") == 0
	int errnum_in = errnum;
	getsym();
	if (strcmp(sym, "IDENT") == 0){
		if (lookup(id) != -1 && table[lookup(id)].lev == level){  //���ʱ������ͬ���ظ�����
			error("ͬ��������ظ����壡");
		}
		else{
			set_item(PROC, NUL);//
			gen_code(level + 1, proc, table[t_pos]); //
			set_bitem();//bt_posָ��ǰ����
			pre_t_pos[level + 1] = t_pos;//
		}
		getsym();
		parameter_list();           //parameter_list()���н�����bpara_numΪ��ʽ���������βθ���	
	}
	else{
		error("�������Ƿ���");
		jump();
	}
	if (strcmp(sym, "SEMICOLON") == 0){
		getsym();
	}
	else{
		error("�����ײ���©�ֺţ�");
	}
	if (errnum_in == errnum){
		printf("This is a procedure head!\n");
	}
}

void procdeclaration(){
	int errnum_in = errnum;
	void sub_program();                  //����������������ӵݹ����
	if (strcmp(sym, "PROCSYM") == 0){
		do{
			proc_head();                 //proc_head()���к�bpara_numΪ��ǰ�ֳ����������
			sub_program();               //������ǰ�ֳ��򣬴�ʱbpara_numΪ��ǰ�ֳ����������
			if (strcmp(sym, "SEMICOLON") == 0){
				getsym();
			}
			else if (strcmp(sym, "PROCSYM") == 0){
				error("����˵������֮��©�ֺţ�");
			}
			else {
				error("����˵�����ֺ�©�ֺţ�");
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
		if (lookup(id) != -1 && table[lookup(id)].lev == level){  //���ʱ������ͬ���ظ�����
			error("ͬ�㺯�����ظ����壡");
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
		error("����������");
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
			error("�����ײ���©�ֺţ�");
			jump();
			getsym();
		}
	}
	else{
		error("�����ײ�ȱ������˵����");
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
				error("����˵������֮��©�ֺţ�");
			}
			else {
				error("����˵�����ֺ�©�ֺţ�");
			}
		} while (strcmp(sym, "FUNCSYM") == 0);
		if (errnum_in == errnum){
			printf("This is a function declaration !\n");
		}
	}
}

//========================��估�������================================

//==========��ֵ���==========

void argument_list(int index_0){
	void expression(ITEM &);
	ITEM x;
	int i = 0;
	if (strcmp(sym, "LPAREN") == 0){             //����û��ʵ�α�Ĺ��̣��ú���ֱ������
		getsym();
		if (strcmp(sym, "RPAREN") == 0){
			error("ʵ�α�������û�в�����");
			jump();
			return;
		}
		do{
			expression(x);//
			if (x.type != btab[table[index_0].val].bp_list[i++].ptype){
				if (i > btab[table[index_0].val].bpara_num){
					error("ʵ�θ������࣡");
				}
				else{
					error("ʵ�����β����Ͳ�һ�£�");
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
				error("������֮��©�ֺţ�");
			}
		} while (strcmp(sym, "PLUS") == 0 || strcmp(sym, "MINUS") == 0 ||
			     strcmp(sym, "IDENT") == 0 || strcmp(sym, "NUMBER") == 0 || strcmp(sym, "LPAREN") == 0);
		if (strcmp(sym, "RPAREN") == 0){
			getsym();
			if (i < btab[table[index_0].val].bpara_num){
				error("ʵ�θ������٣�");
			}
		}
		else {
			error("ȱ�������ţ�");
			jump();
		}
	}
}

void factor(ITEM &x){
	void expression(ITEM &);                          //������������ӵݹ����
	int iden_index;
	ITEM x1,op1;
	if (strcmp(sym, "IDENT") == 0){
		iden_index = lookup(id);//
		if (iden_index == -1){ //����ʱ���δ���屨��
			error("��ʶ��δ���壨���ӣ���");
			jump();
			return;
		}
		getsym();
		if (strcmp(sym, "LBRACK") == 0){
			getsym();
			expression(x1);
			if ((x1.kind == IMM || x1.kind == CONST) && (x1.val >= table[iden_index].array_length || x1.val<0)){
				error("��������������±�Խ�磨����Ԫ��Ϊ��ֵ����");
				jump();
				return;
			}
			if (strcmp(sym, "RBRACK") == 0){
				getsym();
			}
			else {
				error("�����±�ȱ���ҷ����ţ�");
			}
		}
		else if (strcmp(sym, "LPAREN") == 0){
			argument_list(iden_index);
		}
		//////////////////////////////////////////////////////////////////
		if (table[iden_index].kind == CONST){ //���ӿ���Ϊ�ַ�����������
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
			error("���ӱ��ʽ©�����ţ�");
		}
	}
	else if (strcmp(sym, "NUMBER") == 0){
		x.kind=IMM;//
		x.type = INT;//
		x.lev = -1;//
		x.val = num;//
		getsym();
	}
	else if (strcmp(sym, "CHARACTER") == 0){//���Ӳ���Ϊ�ַ�������
		error("���Ӳ���Ϊ�ַ���");
		jump();
	}
	else{
		error("���ӷǷ���");
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
				error("ȱ���ҷ����ţ�");
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
				op1.val = 0;//op1Ϊ��������ֵ
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
					error("��������������±�Խ�磨����Ԫ��Ϊ��ֵ����");
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
				error("��ֵ����");
			}
			//////////////////////////////////////////////////////////
		}
		else{
			error("ȱ�ٸ�ֵ���ţ�");
			jump();
		}
	//}
	//else{
	//	error("��ֵ���Ӧ�Ա�ʶ����ʼ��");
	//	jump();
	//	getsym();
    //}
	if (errnum_in == errnum){
		printf("This is an assignment statement!\n");
	}
}

//=========�������============

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
		error("�Ƚ�������Ƿ���");
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
		error("ȱ��then��");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a if_conditional statement!\n");
	}
}

//=========��ѭ�����==========

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
		error("do_while�����ȱ�ٹؼ���while!");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a when_loop statement!\n");
	}
}

//========forѭ�����==========
void for_loop_state(){
	int errnum_in = errnum;
	void statement(); 
	int iden_index;//
	ITEM x1,x2,op1,op2;//
	char TO_DOWNTO[10];
	getsym();
	if (strcmp(sym, "IDENT") == 0){
		iden_index = lookup(id);//
		if (iden_index == -1){ //����ʱ���δ���屨��
			error("��ʶ��δ���壨forѭ����������");
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
					error("forѭ�������ȱ�ٹؼ���do��");
					jump();
				}
			}
			else{
				error("forѭ�������ȱ�ٹؼ���to����downto��");
				jump();
			}
		}
	}
	else{
		error("forѭ������йؼ���for��ӦΪ��ʶ����");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a for_loop statement!\n");
	}
}

//========���̵������=========
//void call_state(){

//}

//==========�����=============
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
				if (index_3 == -1){ //����ʱ���δ���屨��
					error("��ʶ��δ���壨����䣩��");
					while (strcmp(sym, "SEMICOLON") != 0){
						getsym();
					}
					return;
				}
				if(table[index_3].kind != VAR&&table[index_3].kind != PARA&&table[index_3].kind != VPARA){
					error("��������ֻ��Ϊ�����������");
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
					error("��������֮��©���ţ�");
				}
			} while (strcmp(sym, "IDENT") == 0);
			if (strcmp(sym, "RPAREN") == 0){
				getsym();
			}
			else{
				error("����������ȱ�������ţ�");
				jump();
			}
		}
		else{
			error("�����������ڲ��Ǳ�ʶ��������û�в�����");
			jump();
		}
	}
	else{
		error("�����ȱ�ٲ�����");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a read statement!\n");
	}
}

//==========д���=============
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
			error("д�������б�ȱ�������ţ�");
			jump();
		}
	}
	else{
		error("д���ȱ�ٲ����б�");
		jump();
	}
	if (errnum_in == errnum){
		printf("This is a write statement!\n");
	}
}

//==========�������===========
void composition_state(){
	void statement();
	int errnum_in = errnum;
	if (strcmp(sym, "BEGINSYM") != 0){
		error("�������ȱ��begin��");
	}
	getsym();
	statement();
	while (strcmp(sym, "SEMICOLON") == 0){
		getsym();
		if (strcmp(sym, "ENDSYM") == 0){
			//error("����������һ���Ӿ���ֺţ�");//����������һ���Ӿ�����зֺ�
			break;
		}
		statement();
	}
	//printf("%s\n", sym);  //���ڵ���
	if (strcmp(sym, "ENDSYM") == 0){
		getsym();
		if (errnum_in == errnum){
			printf("This is a composition statement!\n");
		}
	}
	else{
		error("��������©end��");
		jump();
		getsym();
	}
}

//==========���===============
void statement(){
	int errum_in_call_state ;
	int ident_index;//
	if (strcmp(sym, "IDENT") == 0){
		ident_index = lookup(id);//
		if (ident_index == -1){ //����ʱ���δ���屨��
			error("��ʶ��δ���壨��ֵ������̵�����䣩��");
			while (strcmp(sym, "SEMICOLON") != 0){
				getsym();
			}
			return;
		}
		if (table[ident_index].kind == CONST){
			error("���ܸı䳣����ֵ��");
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
			error("��ֵ����и�ֵ����д�ɵȺţ�");
			jump();
		}
		else {
			errum_in_call_state = errnum;
			if (strcmp(sym, "LPAREN") == 0){
				argument_list(ident_index);
			}
			gen_code(level, call, table[ident_index]);//���̵���
			
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
		error("���Ƿ���");
		jump();
	}
}


//=============================================================================
void sub_program(){
	level++;//
	if(level > MAX_LEVEL){
		error("�ֳ����γ�����������Σ�");
		jump();
		return;
	}
	int t_px = t_pos-btab[bt_pos].bpara_num;//����ֳ���������ڷ��ű��е�����
	int bt_px = bt_pos;//����ֳ����ڹ��̱��е�����
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
	//print_table();//��ӡ���ֳ������õ�ջʽ���ű����ڵ���
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
	//printf("%s\n", sym);  //���ڵ���
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
	
	printf("������Դ�ļ�·�����ļ���(����e�˳�����)��\n");
	scanf("%s", source_filename);
	if (strcmp(source_filename, "e") == 0){
		return -1;
	}

	FILE *fpin;
	
	while ((fpin = fopen(source_filename, "r")) == NULL){
		printf("�޷���Դ�ļ�%s!\n",source_filename);
		printf("������Դ�ļ�·�����ļ���(����e�˳�����)��\n");
		scanf("%s", source_filename);
		if (strcmp(source_filename, "e") == 0){
			return -1;
		}
	}

	

	int i = 0;                    //��Դ�����������source[]
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
	
	//print_token();//��ӡ�ʷ�������������ڵ���
	program(source_filename);
	//print_btab();//��ӡ���̱����ڵ���
	//print_stab();//��ӡ�ַ����ͱ�ű����ڵ���
	get_maxl();
	if (errnum == 0){
		//print_qfmt(code,cx);//��ӡ��Ԫʽ�����ڵ���
		optimize();
		//print_qfmt(code2,cx2);//��ӡ�Ż������Ԫʽ�����ڵ���
		gen_asmfile(code2,cx2);
	}

	fclose(fpin);

	return 0;
}