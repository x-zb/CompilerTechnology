#include<stdio.h>
#include<string.h>
#include"state.h"


extern int pos ;
extern int line;
extern int line_start;
extern char sym[MAX_KEY_WORD_LEN];
extern char id[MAX_IDENT_LEN + 1];
extern int num;
extern char ch;
extern char str[MAX_STRING_LEN + 1];
extern void getsym();
extern int errnum;

void print_token(){
	int i = 1;
	printf("词法分析结果\n");
	printf("============\n");
	printf( "%-10s%-15s%s\n", "序号", "单词类别", "单词值");
	while (1){
		getsym();
		if (strcmp(sym, "END") == 0){
			printf("%d errors!\n",errnum);
			if (errnum == 0)
				printf("Token Analysis Successful!\n");
			break;
		}
		else if (strcmp(sym, "NUMBER") == 0){
			printf("%-10d%-15s%d\n", i, sym, num);
			i++;
		}
		else if (strcmp(sym, "CHARACTER") == 0){
			printf("%-10d%-15s\'%c\'\n", i, sym, ch);
			i++;
		}
		else if (strcmp(sym, "STRING") == 0){
			printf("%-10d%-15s\"%s\"\n", i, sym, str);
			i++;
		}
		else if (strcmp(sym, "IDENT") == 0){
			printf( "%-10d%-15s%s\n", i, sym, id);
			i++;
		}
		else {
			printf( "%-10d%-15sNULL\n", i, sym);
			i++;
		}
	}
	printf("===================================\n\n");
	pos = 0;
	line = 1;
	line_start = 0;
}


extern ITEM table[MAX_ITEM_NUM];
extern b_ITEM btab[MAX_ITEM_NUM];
extern char stab[50][MAX_STRING_LEN + 1];
extern int t_pos;
extern int bt_pos;
extern int st_pos;

void print_table(){
	printf("栈式符号表table\n");
	printf("===============\n");
	printf("%-10s%-10s%-10s%-5s%-5s%-10s%-5s\n","name","kind","type","lev","val","array_type","array_length");
	//printf("=====================================================\n");
	for (int i = 0; i <= t_pos; i++){
		char ss1[10];
		switch (table[i].kind){
		case CONST: strcpy(ss1, "CONST"); break;
		case VAR:   strcpy(ss1, "VAR"); break;
		case PROC:  strcpy(ss1, "PROC"); break;
		case FUNC:  strcpy(ss1, "FUNC"); break;
		case PARA:  strcpy(ss1, "PARA"); break;
		case VPARA: strcpy(ss1, "VPARA"); break;
		default:strcpy(ss1, NULL);
		}
		
		char ss2[10];
		switch (table[i].type){
		case INT: strcpy(ss2, "INT"); break;
		case CHAR:   strcpy(ss2, "CHAR"); break;
		case ARRAY:  strcpy(ss2, "ARRAY"); break;
		case NUL:  strcpy(ss2, "NUL"); break;
		default:strcpy(ss2, NULL);
		}

		char ss3[10];
		switch (table[i].array_type){
		case INT: strcpy(ss3, "INT"); break;
		case CHAR:   strcpy(ss3, "CHAR"); break;
		case ARRAY:  strcpy(ss3, "ARRAY"); break;
		case NUL:  strcpy(ss3, "NUL"); break;
		default:strcpy(ss3, NULL);
		}
		printf("%-10s%-10s%-10s%-5d%-5d%-10s%-5d\n",table[i].name,ss1,ss2,table[i].lev,table[i].val,ss3,table[i].array_length);
	}
	printf("===============================================================================\n\n");
}

void print_btab(){
	printf("过程表btab\n");
	printf("==============\n");
	printf("%-10s%-5s%-10s%-10s%-10s%-10s\n", "proc_name", "blev","bpara_num", "bvar_num", "bper_num", "para_list");
	//printf("=======================\n");
	for (int i = 0; i <= bt_pos; i++){
		printf("%-10s%-5d%-10d%-10d%-10d", btab[i].proc_name, btab[i].blev,btab[i].bpara_num,btab[i].bvar_num,btab[i].bper_num);
		for (int j = 0; j < btab[i].bpara_num; j++){
			char ss1[6];
			switch (btab[i].bp_list[j].pkind){
			case PARA:  strcpy(ss1, "PARA"); break;
			case VPARA: strcpy(ss1, "VPARA"); break;
			default:strcpy(ss1, NULL);
			}
			
			char ss2[5];
			switch (btab[i].bp_list[j].ptype){
			case INT: strcpy(ss2, "INT"); break;
			case CHAR:   strcpy(ss2, "CHAR"); break;
			default:strcpy(ss2, NULL);
			}
			printf("(%-6s,%-5s)  ", ss1, ss2);
		}
		printf("\n");
	}
	printf("================================================================================\n\n");
}


void print_stab(){
	printf("字符串及标号表stab\n");
	printf("======================\n");
	printf("%-6s%-s\n", "序号", "字符串");
	for (int i = 0; i <= st_pos; i++){
		printf("%-6d%-s\n", i,stab[i]);
	}
	printf("================================================================================\n\n");
}


extern int cx[MAX_LEVEL + 1];
extern QFRT code[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE];
extern int max_l;

void print_qfmt(QFRT code[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE], int cx[MAX_LEVEL + 1]){
	
	int sum = 0;
	for (int i = 0; i <= max_l; i++){//计算中间代码条数
		sum =sum+ cx[i] + 1;
	}
	
	printf("四元式中间代码\n");
	printf("======================================\n");
	for (int i = max_l; i >= 0; i--){
		for (int j = 0; j <= cx[i]; j++){
			if (code[i][j].opt == null){
				continue;
			}
			char ss[10];
			switch (code[i][j].opt){
			case assign: strcpy(ss, "assign"); break;
			case add:   strcpy(ss, "add"); break;
			case sub:  strcpy(ss, "sub"); break;
			case mul:  strcpy(ss, "mul"); break;
			case div:  strcpy(ss, "div"); break;
			case neg:  strcpy(ss, "neg"); break;
			case arg: strcpy(ss, "arg"); break;
			case call:  strcpy(ss, "call"); break;
			case push:  strcpy(ss, "push"); break;
			case pop:  strcpy(ss, "pop"); break;
			case proc: strcpy(ss, "proc"); break;
			case ret:   strcpy(ss, "ret"); break;
			case arrayl:  strcpy(ss, "arrayl"); break;
			case arrayr:  strcpy(ss, "arrayr"); break;
			case write:   strcpy(ss, "write"); break;
			case read:   strcpy(ss, "read"); break;
			case cmp:   strcpy(ss, "cmp"); break;
			case jmp:  strcpy(ss, "jmp"); break;
			case je:  strcpy(ss, "je"); break;
			case jne:  strcpy(ss, "jne"); break;
			case jg:  strcpy(ss, "jg"); break;
			case jge:  strcpy(ss, "jge"); break;
			case jl:  strcpy(ss, "jl"); break;
			case jle:  strcpy(ss, "jle"); break;
			case label:  strcpy(ss, "label"); break;
			case loopup:  strcpy(ss, "loopup"); break;
			case loopdown:  strcpy(ss, "loopdown"); break;
			default:strcpy(ss, NULL);
			}
			printf("%-8s", ss);
			for (int k = 0; k < 3; k++){
				char ss1[7];
				switch (code[i][j].operand[k].kind){
				case CONST: strcpy(ss1, "CONST"); break;
				case VAR:   strcpy(ss1, "VAR"); break;
				case PROC:  strcpy(ss1, "PROC"); break;
				case FUNC:  strcpy(ss1, "FUNC"); break;
				case PARA:  strcpy(ss1, "PARA"); break;
				case VPARA: strcpy(ss1, "VPARA"); break;
				case K_NULL:  strcpy(ss1, "K_NULL"); break;
				case STRING:  strcpy(ss1, "STRING"); break;
				case IMM:  strcpy(ss1, "IMM"); break;
				case REG:  strcpy(ss1, "REG"); break;
				case TEMP:  strcpy(ss1, "TEMP"); break;
				default:strcpy(ss1, NULL);
				}
				char ss2[6];
				switch (code[i][j].operand[k].type){
				case INT: strcpy(ss2, "INT"); break;
				case CHAR:   strcpy(ss2, "CHAR"); break;
				case ARRAY:  strcpy(ss2, "ARRAY"); break;
				case NUL:  strcpy(ss2, "NUL"); break;
				default:strcpy(ss2, NULL);
				}
				printf("(%-7s,%-6s,%-2d,%-2d)  ", ss1, ss2, code[i][j].operand[k].lev, code[i][j].operand[k].val);
			}
			printf("\n");
		}
		printf("\n\n");//不同层次之间分隔
	}
	printf("中间代码条数：%d\n", sum);
	printf("================================================================================\n\n");
}