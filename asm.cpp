#include<stdio.h>
#include"state.h"



char reg_name[6][9] = { "ebx", "esi", "edi", "eax", "edx", "null_reg" };
int pool[2] = { 0 };//��ʱ�Ĵ����أ�pool[0]Ϊecx�д�ŵ���ʱ�������,pool[1]Ϊedx�д�ŵ���ʱ�������



extern b_ITEM btab[MAX_ITEM_NUM];
extern char stab[50][MAX_STRING_LEN + 1];
//extern QFRT code[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE];        
extern int cx[MAX_LEVEL + 1];
extern int max_l;

extern OPERAND ex_var[MAX_LEVEL][MAX_PROC_NUM][MAX_ITEM_NUM];
extern REGISTER globle_reg[MAX_LEVEL][MAX_PROC_NUM][MAX_ITEM_NUM];

char buf[5] = "bufa";


bool operand_identical(OPERAND op1, OPERAND op2){
	if (op1.kind == op2.kind&&op1.type==op2.type&&op1.lev==op2.lev&&op1.val==op2.val){
		return true;
	}
	else{
		return false;
	}
}



void gen_head(FILE *fp,OPERAND op,bool &same_level,int i){
	same_level = true;
	if (op.lev != i&&(op.kind==VAR||op.kind==PARA||op.kind==VPARA)){
		same_level = false;
		fprintf(fp, "\tmov ecx,display+%d*4\n", op.lev);          //��Ϊ�Ͳ������ecx�����ַ
		if (op.kind == VPARA){
			fprintf(fp, "\tmov ecx,[ecx-%d*4]\n", op.val);        //��Ϊ�Ͳ������ecx�����ַ
		}
	}
	else if (op.kind == VPARA){
		fprintf(fp, "\tmov ecx,[ebp-%d*4]\n", op.val);           //��Ϊ�����βΣ�ecx��ʵ�ε�ַ
	}
}                                                                //��Ϊͬ��Ǳ����βΣ���gen_head()�������κλ�����


void gen_operand(FILE *fp,OPERAND op,bool same_level){ //ȡֵ
	if (op.kind == IMM || op.kind == CONST){
		fprintf(fp, "DWORD PTR %d", op.val);
	}
	else if (op.kind == REG){
		fprintf(fp, "%s", reg_name[op.val]);
	}
	else if (op.kind == VAR || op.kind == PARA||op.kind==TEMP){
		if (same_level){
			fprintf(fp, "[ebp-%d*4]", op.val);
		}
		else{
			fprintf(fp, "[ecx-%d*4]", op.val);
		}
	}
	else if (op.kind == VPARA){
		fprintf(fp, "[ecx]");
	}
}

void gen_operand_addr(FILE *fp, OPERAND op, bool same_level){//ȡ��ַ
	if (op.kind == IMM || op.kind == CONST){
		error("������������������Ϊ����ʵ�Σ�");
		return;
	}
	else if (op.kind == VAR || op.kind == PARA){
		if (same_level){
			fprintf(fp, "\tmov eax,ebp\n");
			fprintf(fp, "\tsub eax,%d*4\n", op.val);
		}
		else{
			fprintf(fp, "\tmov eax,ecx\n");
			fprintf(fp, "\tsub eax,%d*4\n", op.val);
		}
	}
	else if (op.kind == VPARA){
		fprintf(fp, "\tmov eax,ecx\n");
	}
	else if (op.kind == TEMP){
		error("��ʱ����������Ϊ����ʵ�Σ�");
	}
}


void gen_read(FILE *fp,OPERAND op){ //���������eax��
	fprintf(fp, "\tpush OFFSET bufferIn\n");
	if (op.type == INT){
		fprintf(fp, "\tpush OFFSET szFmtIn1\n");
	}
	else{
		fprintf(fp, "\tpush OFFSET szFmtIn2\n");
	}
	fprintf(fp, "\tcall crt_scanf\n");
	fprintf(fp, "\tpop eax\n");
	fprintf(fp, "\tpop eax\n");
	fprintf(fp, "\tmov eax,bufferIn\n");
}



void gen_write(FILE *fp, OPERAND op){
	if (op.kind == STRING){
		buf[3]++;
		fprintf(fp, "\t.DATA\n");
		fprintf(fp, "%s DB \"%s\",0\n",buf,stab[op.val]);
		fprintf(fp, "\t.CODE\n");
		fprintf(fp, "\tpush OFFSET %s\n",buf);
		fprintf(fp, "\tpush OFFSET szFmtOut3\n");
		fprintf(fp, "\tcall crt_printf\n");
		fprintf(fp, "\tpop eax\n");
		fprintf(fp, "\tpop eax\n");
	}
	else if (op.type == INT||op.type==CHAR){
		fprintf(fp, "\tpush eax\n");
		if (op.type == INT){
			fprintf(fp, "\tpush OFFSET szFmtOut1\n");
		}
		else{
			fprintf(fp, "\tpush OFFSET szFmtOut2\n");
		}
		fprintf(fp, "\tcall crt_printf\n");
		fprintf(fp, "\tpop eax\n");
		fprintf(fp, "\tpop eax\n");
	}
	else{
		error("write����������");
	}
}


void gen_asmfile(QFRT code[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE], int cx[MAX_LEVEL + 1]){                                           //ecx,ebx���ڴ��ַ��eax,edx���ڼ���
	FILE *fpasm;
	bool same_level;
	int first_arg_pos ;
	bool first_arg = false;
	if ((fpasm = fopen("C:\\12171060.asm", "w")) == NULL){
		printf("�޷���Ŀ���ļ���\n");
		return;
	}
	
	

	fprintf(fpasm,"\t.386\n");
	fputs("\t.model flat, stdcall\n", fpasm);
	fputs("\toption casemap : none\n\n", fpasm);

	fputs("\tinclude \\masm32\\include\\windows.inc\n", fpasm);
	fputs("\tinclude \\masm32\\include\\user32.inc\n", fpasm);
	fputs("\tinclude \\masm32\\include\\kernel32.inc\n", fpasm);
	fputs("\tinclude \\masm32\\include\\masm32.inc\n", fpasm);
	fputs("\tinclude \\masm32\\include\\msvcrt.inc\n\n", fpasm);

	fputs("\tincludelib \\masm32\\lib\\user32.lib\n", fpasm);
	fputs("\tincludelib \\masm32\\lib\\kernel32.lib\n", fpasm);
	fputs("\tincludelib \\masm32\\lib\\masm32.lib\n", fpasm);
	fputs("\tincludelib \\masm32\\lib\\msvcrt.lib\n\n", fpasm);

	fputs("\t.STACK 4096\n\n", fpasm);

	fputs("\t.DATA\n", fpasm);
	fputs("display DWORD 6 DUP(?)\n", fpasm);
	fputs("szFmtOut1 BYTE \"%d\",0ah,0\n", fpasm);
	fputs("szFmtOut2 BYTE \"%c\",0ah,0\n", fpasm);
	fputs("szFmtOut3 BYTE \"%s\",0ah,0\n", fpasm);
	fputs("szFmtOut4 BYTE \"%s\",0\n", fpasm);
	fputs("szFmtIn1 BYTE \"%d\",0\n", fpasm);
	fputs("szFmtIn2 BYTE \" %c\",0\n", fpasm);//�ո�%c�Ǳ�Ҫ�ģ���ȡ�������ʱ���Իس��ָ�
	fputs("bufferIn DD ?\n\n", fpasm);

	fprintf(fpasm,"\t.CODE\n");
	for (int i = max_l; i >= 0; i--){
		//int b_index;//��¼��ǰ���̻�����btab�е�����
		if (i == 0){
			//b_index = 0;
			fprintf(fpasm, "_start:\n");
			fprintf(fpasm, "\tsub esp,4\n");
			fprintf(fpasm, "\tmov ebp, esp\n");
			fprintf(fpasm, "\tmov display,ebp\n");
			fprintf(fpasm, "\tsub esp, %d * 4\n",btab[0].bpara_num+btab[0].bvar_num+btab[0].bper_num+11);
		}
		for (int j = 0; j <= cx[i]; j++){
			if (code[i][j].opt == assign){
				gen_head(fpasm, code[i][j].operand[1], same_level, i);
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[0], same_level, i);
				fprintf(fpasm, "\tmov ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, ",eax\n");
			}
			else if (code[i][j].opt == add){
				gen_head(fpasm, code[i][j].operand[1], same_level, i);//mov eax,op1
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[2], same_level, i);//add eax,op2
				fprintf(fpasm, "\tadd eax,");
				gen_operand(fpasm, code[i][j].operand[2], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[0], same_level, i);//mov op0,eax
				fprintf(fpasm, "\tmov ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, ",eax\n");
			}
			else if (code[i][j].opt == sub){
				gen_head(fpasm, code[i][j].operand[1], same_level, i);
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[2], same_level, i);
				fprintf(fpasm, "\tsub eax,");
				gen_operand(fpasm, code[i][j].operand[2], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[0], same_level, i);
				fprintf(fpasm, "\tmov ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, ",eax\n");
			}
			else if (code[i][j].opt == mul){
				gen_head(fpasm, code[i][j].operand[1], same_level, i);
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[2], same_level, i);
				fprintf(fpasm, "\timul eax,");
				gen_operand(fpasm, code[i][j].operand[2], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[0], same_level, i);
				fprintf(fpasm, "\tmov ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, ",eax\n");
			}
			else if (code[i][j].opt == div){
				gen_head(fpasm, code[i][j].operand[1], same_level, i);
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");
				
				if (code[i][j].operand[2].kind == IMM || code[i][j].operand[2].kind == CONST){//������������ΪIMM,CONST
					gen_head(fpasm, code[i][j].operand[2], same_level, i);
					fprintf(fpasm, "\tmov ebx,");
					gen_operand(fpasm, code[i][j].operand[2], same_level);
					fprintf(fpasm, "\n");
					fprintf(fpasm, "\tcdq\n");
					fprintf(fpasm, "\tidiv ebx\n");
				}
				else{                                                                        //������������ΪVAR,TEMP,PARA,VPARA
					gen_head(fpasm, code[i][j].operand[2], same_level, i);
					fprintf(fpasm, "\tcdq\n");
					fprintf(fpasm, "\tidiv DWORD PTR ");
					gen_operand(fpasm, code[i][j].operand[2], same_level);
					fprintf(fpasm, "\n");
				}
				gen_head(fpasm, code[i][j].operand[0], same_level, i);
				fprintf(fpasm, "\tmov ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, ",eax\n");
			}
			else if (code[i][j].opt == neg){
				gen_head(fpasm, code[i][j].operand[1], same_level, i);
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				fprintf(fpasm, "\tneg eax\n");

				gen_head(fpasm, code[i][j].operand[0], same_level, i);
				fprintf(fpasm, "\tmov ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, ",eax\n");
			}
			else if (code[i][j].opt == arg){
				if (!first_arg){
					first_arg = true;
					first_arg_pos = j;
					//printf("first_arg_pos:%d\n", j);//���ڵ���
				}
			}
			else if (code[i][j].opt == call){
				fprintf(fpasm, "\tsub esp,4\n");
				fprintf(fpasm, "\tpush ebp\n");
				if(first_arg){ //first_arg==true��ʾ��ʵ�Σ���ʱfirst_arg_posΪ��һ��arg��Ԫʽ��λ��
					for (int n = first_arg_pos,arg_index=0; n < j; n++){
						if (code[i][n].opt == arg){
							if (btab[code[i][j].operand[0].val].bp_list[arg_index].pkind == PARA){//PARA��ֵ
								arg_index++;
								if (code[i][n - 1].opt == arrayr&&operand_identical(code[i][n - 1].operand[0], code[i][n].operand[0]) == true){//����һ����ԪʽΪarrayr��������Ԫ��ֵ������ʱ����
									gen_head(fpasm, code[i][n-1].operand[1], same_level, i);       //mov eax,&op1
									gen_operand_addr(fpasm, code[i][n-1].operand[1], same_level);

									gen_head(fpasm, code[i][n-1].operand[2], same_level, i);       //mov edx,op2
									fprintf(fpasm, "\tmov edx,");
									gen_operand(fpasm, code[i][n-1].operand[2], same_level);
									fprintf(fpasm, "\n");

									fprintf(fpasm, "\timul edx,4\n");                            //imul edx,4
									fprintf(fpasm, "\tsub eax,edx\n");                           //sub eax,edx     eax=&op1-op2*4

									fprintf(fpasm, "\tmov edx,[eax]\n");                         //mov edx,[eax]

									gen_head(fpasm, code[i][n-1].operand[0], same_level, i);       //mov op0,edx
									fprintf(fpasm, "\tmov ");
									gen_operand(fpasm, code[i][n-1].operand[0], same_level);
									fprintf(fpasm, ",edx\n");
								}
								gen_head(fpasm, code[i][n].operand[0], same_level, i);
								fprintf(fpasm, "\tpush ");
								gen_operand(fpasm, code[i][n].operand[0], same_level);
								fprintf(fpasm, "\n");
							}
							else{                                                             //VPARA����ַ
								arg_index++;
								if (code[i][n - 1].opt == arrayr&&operand_identical(code[i][n - 1].operand[0], code[i][n].operand[0]) == true){//����һ����ԪʽΪarrayr��������Ԫ�ص�ַ����eax����ջ
									gen_head(fpasm, code[i][n - 1].operand[1], same_level, i);       //mov eax,&op1
									gen_operand_addr(fpasm, code[i][n - 1].operand[1], same_level);

									gen_head(fpasm, code[i][n - 1].operand[2], same_level, i);       //mov edx,op2
									fprintf(fpasm, "\tmov edx,");
									gen_operand(fpasm, code[i][n - 1].operand[2], same_level);
									fprintf(fpasm, "\n");

									fprintf(fpasm, "\timul edx,4\n");                            //imul edx,4
									fprintf(fpasm, "\tsub eax,edx\n");                           //sub eax,edx     eax=&op1-op2*4
									fprintf(fpasm, "\tpush eax\n");
								}
								else{//������������ַ
									gen_head(fpasm, code[i][n].operand[0], same_level, i);
									gen_operand_addr(fpasm, code[i][n].operand[0], same_level);
									fprintf(fpasm, "\tpush eax\n");
								}
							}
						}
					}
				}
				first_arg = false;
				fprintf(fpasm, "\tmov eax,esp\n");
				fprintf(fpasm, "\tadd eax,%d*4\n", btab[code[i][j].operand[0].val].bpara_num + 1);
				fprintf(fpasm, "\tmov ebp,eax\n");
				fprintf(fpasm, "\tmov display+%d*4,ebp\n", btab[code[i][j].operand[0].val].blev);
				fprintf(fpasm, "\tcall %s\n", btab[code[i][j].operand[0].val].proc_name);
				if (code[i][j].operand[1].kind != K_NULL){
					gen_head(fpasm, code[i][j].operand[1], same_level, i);
					fprintf(fpasm, "\tpop ");
					gen_operand(fpasm, code[i][j].operand[1], same_level);
					fprintf(fpasm, "\n");
				}
			}
			else if (code[i][j].opt == proc){
				//b_index = code[i][j].operand[0].val;
				fprintf(fpasm, "%-s\tPROC NEAR32\n", btab[code[i][j].operand[0].val].proc_name);
				fprintf(fpasm, "\tpushad\n");
				fprintf(fpasm, "\tpushf\n");
				fprintf(fpasm, "\tsub esp,2\n");
				fprintf(fpasm, "\tsub esp,%d*4\n", btab[code[i][j].operand[0].val].bvar_num + btab[code[i][j].operand[0].val].bper_num);
			}
			else if (code[i][j].opt == ret){
				fprintf(fpasm, "\tmov eax,ebp\n");
				fprintf(fpasm, "\tsub eax,%d*4-2\n", btab[code[i][j].operand[0].val].bpara_num + 11);
				fprintf(fpasm, "\tmov esp,eax\n");
				fprintf(fpasm, "\tpopf\n");
				fprintf(fpasm, "\tpopad\n");
				fprintf(fpasm, "\tmov ebp,[ebp-4]\n");
				fprintf(fpasm, "\tret %d*4\n", btab[code[i][j].operand[0].val].bpara_num + 1);
				fprintf(fpasm, "%-s\tENDP\n", btab[code[i][j].operand[0].val].proc_name);
			}
			else if (code[i][j].opt == arrayl){
				gen_head(fpasm, code[i][j].operand[0], same_level, i);       //mov eax,&op0
				gen_operand_addr(fpasm, code[i][j].operand[0], same_level);

				gen_head(fpasm, code[i][j].operand[1], same_level, i);       //mov edx,op1
				fprintf(fpasm, "\tmov edx,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				fprintf(fpasm, "\timul edx,4\n");                            //imul edx,4
				fprintf(fpasm, "\tsub eax,edx\n");                           //sub eax,edx     eax=&op0-op1*4

				gen_head(fpasm, code[i][j].operand[2], same_level, i);       //mov edx,op2
				fprintf(fpasm, "\tmov edx,");
				gen_operand(fpasm, code[i][j].operand[2], same_level);
				fprintf(fpasm, "\n");

				fprintf(fpasm, "\tmov [eax],edx\n");                         //mov [eax],edx
			}
			else if (code[i][j].opt == arrayr){
				if (code[i][j + 1].opt != arg){//����һ����ԪʽΪarg���򲻲���������Ԫ�ظ�����ʱ�����Ĵ���
					gen_head(fpasm, code[i][j].operand[1], same_level, i);       //mov eax,&op1
					gen_operand_addr(fpasm, code[i][j].operand[1], same_level);

					gen_head(fpasm, code[i][j].operand[2], same_level, i);       //mov edx,op2
					fprintf(fpasm, "\tmov edx,");
					gen_operand(fpasm, code[i][j].operand[2], same_level);
					fprintf(fpasm, "\n");

					fprintf(fpasm, "\timul edx,4\n");                            //imul edx,4
					fprintf(fpasm, "\tsub eax,edx\n");                           //sub eax,edx     eax=&op1-op2*4

					fprintf(fpasm, "\tmov edx,[eax]\n");                         //mov edx,[eax]

					gen_head(fpasm, code[i][j].operand[0], same_level, i);       //mov op0,edx
					fprintf(fpasm, "\tmov ");
					gen_operand(fpasm, code[i][j].operand[0], same_level);
					fprintf(fpasm, ",edx\n");
				}
			}
			else if (code[i][j].opt == write){
				if (code[i][j].operand[0].kind == STRING){
					gen_write(fpasm, code[i][j].operand[0]);
					if (code[i][j].operand[1].kind != K_NULL){
						gen_head(fpasm, code[i][j].operand[1], same_level, i);
						fprintf(fpasm, "\tmov eax,");
						gen_operand(fpasm, code[i][j].operand[1], same_level);
						fprintf(fpasm, "\n");
						gen_write(fpasm, code[i][j].operand[1]);
					}
				}
				else if (code[i][j].operand[0].type == INT || code[i][j].operand[0].type==CHAR){//����kind==CONST,VAR,TEMP,IMM,PARA,VPARA
					gen_head(fpasm, code[i][j].operand[0], same_level, i);
					fprintf(fpasm, "\tmov eax,");
					gen_operand(fpasm, code[i][j].operand[0], same_level);
					fprintf(fpasm, "\n");
					gen_write(fpasm, code[i][j].operand[0]);
				}
				else{
					error("write����һ����������");
				}
			}
			else if (code[i][j].opt == read){
				
						gen_read(fpasm, code[i][j].operand[0]);

						gen_head(fpasm, code[i][j].operand[0], same_level, i);
						fprintf(fpasm, "\tmov ");
						gen_operand(fpasm, code[i][j].operand[0], same_level);
						fprintf(fpasm, ",eax\n");
					
				
			}
			else if (code[i][j].opt == cmp){
				gen_head(fpasm, code[i][j].operand[0], same_level, i);//mov eax,op0
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, "\n");
				
				gen_head(fpasm, code[i][j].operand[1], same_level, i);//mov edx,op1
				fprintf(fpasm, "\tmov edx,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");
				
				fprintf(fpasm, "\tcmp eax,edx\n");//cmp eax,edx
			}
			else if (code[i][j].opt == jmp){
				fprintf(fpasm, "\tjmp %s\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == je){
				fprintf(fpasm, "\tje %s\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == jne){
				fprintf(fpasm, "\tjne %s\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == jg){
				fprintf(fpasm, "\tjg %s\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == jge){
				fprintf(fpasm, "\tjge %s\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == jl){
				fprintf(fpasm, "\tjl %s\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == jle){
				fprintf(fpasm, "\tjle %s\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == label){
				fprintf(fpasm, "%s:\n", stab[code[i][j].operand[0].val]);
			}
			else if (code[i][j].opt == loopup){
				gen_head(fpasm, code[i][j].operand[0], same_level, i);//inc op0
				fprintf(fpasm, "\tinc DWORD PTR ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[0], same_level, i);//mov eax,op0
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[1], same_level, i);//mov edx,op1
				fprintf(fpasm, "\tmov edx,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				fprintf(fpasm, "\tcmp eax,edx\n");//cmp eax,edx

				fprintf(fpasm, "\tjle %s\n",stab[code[i][j].operand[2].val]);//jle op2
			}
			else if (code[i][j].opt == loopdown){
				gen_head(fpasm, code[i][j].operand[0], same_level, i);//inc op0
				fprintf(fpasm, "\tdec DWORD PTR ");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[0], same_level, i);//mov eax,op0
				fprintf(fpasm, "\tmov eax,");
				gen_operand(fpasm, code[i][j].operand[0], same_level);
				fprintf(fpasm, "\n");

				gen_head(fpasm, code[i][j].operand[1], same_level, i);//mov edx,op1
				fprintf(fpasm, "\tmov edx,");
				gen_operand(fpasm, code[i][j].operand[1], same_level);
				fprintf(fpasm, "\n");

				fprintf(fpasm, "\tcmp eax,edx\n");//cmp eax,edx

				fprintf(fpasm, "\tjge %s\n", stab[code[i][j].operand[2].val]);//jle op2
			}
			else if (code[i][j].opt == null){
				continue;
			}
			else{
				error("�Ƿ���Ԫʽ��������");
				return;
			}
		}
	}

	fputs("\tmov esp,ebp\n", fpasm);
	fputs("\tadd esp,4\n", fpasm);
	fputs("\tpush 0\n", fpasm);
	fputs("\tcall ExitProcess\n", fpasm);
	fputs("\tEND _start\n", fpasm);
	printf("12171060.asm created!\n");
	fclose(fpasm);
}