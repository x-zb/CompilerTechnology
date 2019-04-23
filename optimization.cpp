#include<stdio.h>
#include"state.h"
extern void *malloc1(int n);



extern QFRT code[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE];
extern int cx[MAX_LEVEL + 1];
extern int max_l;




//====================基本块划分======================
//(所有函数对参数code进行操作)

BBLOCK bblocks[MAX_BBLOCK_NUM];

void gen_block(int blockNo,int bbstart,int bbend,int x0,int x1,int y){
	bblocks[blockNo].bbstart=bbstart;
	bblocks[blockNo].bbend=bbend;
	bblocks[blockNo].later_bb_index[0] = x0;
	bblocks[blockNo].later_bb_index[1] = x1;
	bblocks[blockNo].later_bb_label_index = y;
}


int bblocks_dividing_1(QFRT code[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE],int lev, int start, int end){
	int block_index = -1;
	int i,bstart,bend=start-1;
	while (bend != end){
		bstart = bend + 1;
		i = bstart;
		if (code[lev][i].opt == label &&i!=end){
			i++;
		}
		while (code[lev][i].opt != label&&code[lev][i].opt != jmp&&code[lev][i].opt != je
			&&code[lev][i].opt != jne&&code[lev][i].opt != jg&&code[lev][i].opt != jge&&code[lev][i].opt != jl
			&&code[lev][i].opt != jle&&code[lev][i].opt != loopup&&code[lev][i].opt != loopdown&&i!=end){
			i++;
		}		
		if (code[lev][i].opt == label&&i!=end){
			bend = i - 1;
		}
		else{
			bend = i;
		}
	    /////////////////////////////////////////////
		if (i == end){
			if (code[lev][i].opt == jmp){
				gen_block(++block_index, bstart, bend, -1,-1, code[lev][i].operand[0].val);//最后一条中间代码操作符为jmp
			}
			else if (code[lev][i].opt == je || code[lev][i].opt == jne || code[lev][i].opt == jg || code[lev][i].opt == jge
				|| code[lev][i].opt == jl || code[lev][i].opt == jle){
				gen_block(++block_index, bstart, bend, -1, block_index + 1, code[lev][i].operand[0].val);
			}
			else if (code[lev][i].opt == loopup || code[lev][i].opt == loopdown){
				gen_block(++block_index, bstart, bend, -1, block_index + 1, code[lev][i].operand[2].val);
			}
			else{
				gen_block(++block_index, bstart, bend, block_index + 1, -1, -1);//最后一条中间代码的操作符为非转移操作符
			}
		}
		else if (code[lev][i].opt == label){
			gen_block(++block_index, bstart, bend, block_index+1, -1, -1);
		}
		else if (code[lev][i].opt==jmp){
			gen_block(++block_index, bstart, bend, -1, -1, code[lev][i].operand[0].val);
		}
		else if(code[lev][i].opt==loopup||code[lev][i].opt==loopdown){
			gen_block(++block_index, bstart, bend, -1,block_index + 1,code[lev][i].operand[2].val );
		}
		else{//code[lev][i].opt为条件转移操作符
			gen_block(++block_index, bstart, bend, -1, block_index + 1, code[lev][i].operand[0].val);
		}
	}
	gen_block(++block_index, -1, -1, -1, -1,-1);//在最后加一个“结束”基本块,开始位置和结束位置都为-1
	return block_index;
}


void bblocks_dividing_2(QFRT code[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE],int lev, int max_index){
	int j;
	for (int i = 0; i < max_index; i++){
		if (bblocks[i].later_bb_label_index != -1){
			for (j = 0; j < max_index; j++){
				if (code[lev][bblocks[j].bbstart].opt == label
					&& code[lev][bblocks[j].bbstart].operand[0].val == bblocks[i].later_bb_label_index){
					break;
				}
			}
			bblocks[i].later_bb_index[0] = j;
		}
	}
}


//===============基本块内部的公共子表达式删除（DAG图）=====================
//(所有函数对code[][]和code2[][]进行操作)


int node_num = -1;//DAG图结点序号，从0开始
NODETABLE nodetable[MAX_NODE_NUM];//结点表
int ntable_p = -1;//指向结点表尾
DAGNODE_POINTER nodepointer[MAX_NODE_NUM];//DAG图结点指针数组，下标对应结点序号，用于访问结点
QFRT code2[MAX_LEVEL + 1][2 * MAX_SOURCE_FILE];        //code2用来存放优化后的各层次的中间代码
int cx2[MAX_LEVEL + 1];                                //cx2[i]为各层次中间代码的当前位置
//extern void print_node(int nodeNo);

//============从四元式导出DAG图================

void gen_node(DAGNODE_POINTER &p,OPERAND op1,int n){  //产生DAG图叶结点
	p = (DAGNODE_POINTER)malloc1(sizeof(DAGNODE));
	p->a.kind = op1.kind;
	p->a.lev = op1.lev;
	p->a.type = op1.type;
	p->a.val = op1.val;
	p->x = null;
	p->nodeNo = n;
	p->lchild = NULL;
	p->rchild = NULL;
	nodepointer[n] = p;//填指针数组
	//print_node(n); printf("\n");
}

void gen_node(DAGNODE_POINTER &p,OPERATOR opt, int n,DAGNODE_POINTER left,DAGNODE_POINTER right){ //产生DAG图中间结点
	p = (DAGNODE_POINTER)malloc1(sizeof(DAGNODE));
	p->a.kind = K_NULL;
	p->x = opt;
	p->nodeNo = n;
	p->lchild = left;
	p->rchild = right;
	nodepointer[n] = p;//填指针数组
	//print_node(n); printf("\n");
}

void gen_node(DAGNODE_POINTER &p, OPERATOR opt, OPERAND op1 ,int n, DAGNODE_POINTER left, DAGNODE_POINTER right){ //产生arrayl结点！！
	p = (DAGNODE_POINTER)malloc1(sizeof(DAGNODE));
	p->a.kind = op1.kind;
	p->a.type = op1.type;
	p->a.lev = op1.lev;
	p->a.val = op1.val;
	p->x = opt;
	p->nodeNo = n;
	p->lchild = left;
	p->rchild = right;
	nodepointer[n] = p;//填指针数组
	//print_node(n); printf("\n");
}

void gen_nodetable(OPERAND op1, int n){//填结点表
	ntable_p++;
	nodetable[ntable_p].a.kind = op1.kind;
	nodetable[ntable_p].a.type = op1.type;
	nodetable[ntable_p].a.lev = op1.lev;
	nodetable[ntable_p].a.val = op1.val;
	nodetable[ntable_p].nodeNo = n;
	//print_nodetable();
}

bool isNode(OPERATOR x){ //判断操作符是否为add,sub,mul,div,arrayr,arrayl
	if (x == add || x == sub || x == mul || x == div || x == arrayr || x == arrayl){
		return true;
	}
	else{
		return false;
	}
}


bool isDAG(OPERATOR x){ //参与DAG图优化的四元式有assign,neg,add,sub,mul,div,arrayr,arrayl
	if (x == assign||x==neg||x==add || x == sub || x == mul || x == div || x == arrayr || x == arrayl){
		return true;
	}
	else{
		return false;
	}
}

int lookup_nodetable(OPERAND op1){  //查结点表，返回结点表索引
	int i;
	for (i = ntable_p; i >= 0; i--){
		if (operand_identical(op1, nodetable[i].a)){
			break;
		}
	}
	return i;
}



void Q2DAG(int lev,int start,int end){ //将四元式转化成DAG图
	int index,k;
	int nodei, nodej, nodek;
	DAGNODE_POINTER p1, p2, p3;

	node_num = -1;//DAG图结点序号，从0开始
	ntable_p = -1;//指向结点表尾

	for (int i = start; i <= end; i++){
		if (isNode(code[lev][i].opt)){
			index = lookup_nodetable(code[lev][i].operand[1]);
			if (index == -1){
				gen_node(p1, code[lev][i].operand[1], ++node_num);
				nodei = node_num;
				gen_nodetable(code[lev][i].operand[1], nodei);
			}
			else {
				p1 = nodepointer[nodetable[index].nodeNo];
				nodei = nodetable[index].nodeNo;
			}
			
			index = lookup_nodetable(code[lev][i].operand[2]);
			if (index == -1){
				gen_node(p2, code[lev][i].operand[2], ++node_num);
				nodej = node_num;
				gen_nodetable(code[lev][i].operand[2], nodej);
			}
			else {
				p2 = nodepointer[nodetable[index].nodeNo];
				nodej = nodetable[index].nodeNo;
			}
			//////////////////////////////////////////////////////////
			for (k = 0; k <= node_num; k++){
				if (nodepointer[k]->x == code[lev][i].opt&&nodepointer[k]->lchild->nodeNo == nodei
					&&nodepointer[k]->rchild->nodeNo == nodej){
					nodek = nodepointer[k]->nodeNo;
					break;
				}
			}
			if (k > node_num){
				if (code[lev][i].opt == arrayl){
					gen_node(p3, code[lev][i].opt, code[lev][i].operand[0],++node_num, p1, p2);
				}
				else{
					gen_node(p3, code[lev][i].opt, ++node_num, p1, p2);
				}
				nodek = node_num;
			}
			/////////////////////////////////////////////////////////
			index = lookup_nodetable(code[lev][i].operand[0]);
			if (index == -1){
				gen_nodetable(code[lev][i].operand[0], nodek);
			}
			else{
				nodetable[index].nodeNo = nodek;
			}
		}
		else if (code[lev][i].opt == neg){
			index = lookup_nodetable(code[lev][i].operand[1]);
			if (index == -1){
				gen_node(p1, code[lev][i].operand[1], ++node_num);
				nodei = node_num;
				gen_nodetable(code[lev][i].operand[1], nodei);
			}
			else {
				p1 = nodepointer[nodetable[index].nodeNo];
				nodei = nodetable[index].nodeNo;
			}
			/////////////////////////////////////////////
			for (k = 0; k <= node_num; k++){
				if (nodepointer[k]->x == neg&&nodepointer[k]->lchild->nodeNo == nodei){
					nodek = nodepointer[k]->nodeNo;
					break;
				}
			}
			if (k > node_num){
				gen_node(p3, neg, ++node_num, p1, NULL);
				nodek = node_num;
			}
			////////////////////////////////////////////
			index = lookup_nodetable(code[lev][i].operand[0]);
			if (index == -1){
				gen_nodetable(code[lev][i].operand[0], nodek);
			}
			else{
				nodetable[index].nodeNo = nodek;
			}
		}
		else if (code[lev][i].opt == assign){
			index = lookup_nodetable(code[lev][i].operand[1]);
			if (index == -1){
				gen_node(p1, code[lev][i].operand[1], ++node_num);
				nodei = node_num;
				gen_nodetable(code[lev][i].operand[1], nodei);
			}
			else {
				p1 = nodepointer[nodetable[index].nodeNo];
				nodei = nodetable[index].nodeNo;
			}
			/////////////////////////////////////////////
			for (k = 0; k <= node_num; k++){
				if (nodepointer[k]->x == assign&&nodepointer[k]->rchild->nodeNo == nodei){
					nodek = nodepointer[k]->nodeNo;
					break;
				}
			}
			if (k > node_num){
				gen_node(p3, assign, code[lev][i].operand[0],++node_num, NULL,p1);
				nodek = node_num;
			}
			////////////////////////////////////////////
			index = lookup_nodetable(code[lev][i].operand[0]);
			if (index == -1){
				gen_nodetable(code[lev][i].operand[0], nodek);
			}
			else{
				nodetable[index].nodeNo = nodek;
			}
		}
	}
}

//============从DAG图导出四元式=============


void initial_cx2(){                       //在optimize()中将各层的cx2初始化为-1
	for (int i = 0; i <= MAX_LEVEL; i++){
		cx2[i] = -1;
	}
}


void regen_code(int prog_level,OPERATOR opt, OPERAND op1, OPERAND op2, OPERAND op3){
	//printf("level:%d\n", prog_level);
	cx2[prog_level]++;
	
	code2[prog_level][cx2[prog_level]].opt = opt;

	code2[prog_level][cx2[prog_level]].operand[0].kind = op1.kind;
	code2[prog_level][cx2[prog_level]].operand[0].type = op1.type;
	code2[prog_level][cx2[prog_level]].operand[0].lev = op1.lev;
	code2[prog_level][cx2[prog_level]].operand[0].val = op1.val;

	code2[prog_level][cx2[prog_level]].operand[1].kind = op2.kind;
	code2[prog_level][cx2[prog_level]].operand[1].type = op2.type;
	code2[prog_level][cx2[prog_level]].operand[1].lev = op2.lev;
	code2[prog_level][cx2[prog_level]].operand[1].val = op2.val;

	code2[prog_level][cx2[prog_level]].operand[2].kind = op3.kind;
	code2[prog_level][cx2[prog_level]].operand[2].type = op3.type;
	code2[prog_level][cx2[prog_level]].operand[2].lev = op3.lev;
	code2[prog_level][cx2[prog_level]].operand[2].val = op3.val;
}

void regen_code(int prog_level, OPERATOR opt, OPERAND op1, OPERAND op2){
	//printf("level:%d\n", prog_level);
	cx2[prog_level]++;
	
	code2[prog_level][cx2[prog_level]].opt = opt;

	code2[prog_level][cx2[prog_level]].operand[0].kind = op1.kind;
	code2[prog_level][cx2[prog_level]].operand[0].type = op1.type;
	code2[prog_level][cx2[prog_level]].operand[0].lev = op1.lev;
	code2[prog_level][cx2[prog_level]].operand[0].val = op1.val;

	code2[prog_level][cx2[prog_level]].operand[1].kind = op2.kind;
	code2[prog_level][cx2[prog_level]].operand[1].type = op2.type;
	code2[prog_level][cx2[prog_level]].operand[1].lev = op2.lev;
	code2[prog_level][cx2[prog_level]].operand[1].val = op2.val;

	code2[prog_level][cx2[prog_level]].operand[2].kind = K_NULL;
}


/*void copy_operand(OPERAND &op1, OPERAND op2){
	op1.kind = op2.kind;
	op1.type = op2.type;
	op1.lev = op2.lev;
	op1.val = op2.val;
}*/


OPERAND search_for_operand(int nodeNo){//输入结点序号，返回该结点对应项目的索引，用于得到操作数
	int k;

	if (nodepointer[nodeNo]->x == null){//若为叶结点，返回该叶结点作为操作数
		return nodepointer[nodeNo]->a;
	}

	if (nodepointer[nodeNo]->x == arrayl || nodepointer[nodeNo]->x == assign){//若为arrayl或assign结点，返回该结点中操作数(即数组名)作为操作数，以防在结点表中无法查到
		return nodepointer[nodeNo]->a;
	}

	for (k = 0; k <= ntable_p; k++){      //优先使用TEMP(临时变量)作为操作数
		if (nodetable[k].nodeNo == nodeNo&&nodetable[k].a.kind==TEMP){
			break;
		}
	}
	if (k == ntable_p + 1){             //若无临时变量，取先填入结点表的VAR|IMM|CONST|PARA|VPARA作为操作数
		for (k = 0; k <= ntable_p; k++){
			if (nodetable[k].nodeNo == nodeNo){
				break;                            
			}
		}
	}
	return nodetable[k].a;
}

void DAG2Q(int lev){                //生成当前DAG图对应的四元式
	
	int parent_num[MAX_NODE_NUM] = { 0 };//记录每个结点当前父结点个数
	int current_node[MAX_NODE_NUM] = { 0 };//尚未入栈的结点为1
	int node_stack[MAX_NODE_NUM];//结点栈
	int stack_top = -1;//结点栈栈顶指针
	int k;
	int current, left, right;//用于从结点序号生成中间代码
	
	int current_nodeNo;//用于给未赋值的局部变量和参数赋值
	int var[MAX_NODE_NUM] = { 0 };
	bool isFirst;

	for (int i = 0; i <= node_num; i++){//通过nodepointer[]遍历所有结点，初始化parent_num[]和current_node[]
		if (nodepointer[i]->lchild != NULL){
			parent_num[nodepointer[i]->lchild->nodeNo]++;
		}
		if (nodepointer[i]->rchild != NULL){
			parent_num[nodepointer[i]->rchild->nodeNo]++;
		}
		if (nodepointer[i]->x != null){
			current_node[i] = 1;
		}
	}

	//printf("parent_num:");
	//for (int i = 0; i <= node_num; i++){//
	//	printf("%d ", parent_num[i]);
	//}
	//printf("\n");
	////////////////////////////////////////////////////////////////
	while (1){//计算结点入栈顺序的启发式算法，适于X86结构
		for (k = node_num; k >= 0; k--){
			if (current_node[k] == 1 && parent_num[k] == 0){
				break;
			}
		}
		if (k == -1){
			break;
		}
		node_stack[++stack_top] = k;
		current_node[k] = 0;
		if (nodepointer[k]->lchild != NULL){
			parent_num[nodepointer[k]->lchild->nodeNo]--;
		}
		if (nodepointer[k]->rchild != NULL){
			parent_num[nodepointer[k]->rchild->nodeNo]--;
		}
		while (nodepointer[k]->lchild!=NULL&&parent_num[nodepointer[k]->lchild->nodeNo] == 0 
			&& current_node[nodepointer[k]->lchild->nodeNo] == 1){
			k = nodepointer[k]->lchild->nodeNo;
			node_stack[++stack_top] = k;
			current_node[k] = 0;
			if (nodepointer[k]->lchild != NULL){
				parent_num[nodepointer[k]->lchild->nodeNo]--;
			}
			if (nodepointer[k]->rchild != NULL){
				parent_num[nodepointer[k]->rchild->nodeNo]--;
			}
		}
	}
	///////////////////////////////////////////////////////
	//printf("node_stack:");
	//for (int i = 0; i <= stack_top; i++){//打印栈内容
	//	printf("%d ", node_stack[i]);
	//}
	//printf("\n");
	
	while (stack_top != -1){//根据栈内容，结合结点表生成中间代码
		current = node_stack[stack_top--];

		if (nodepointer[current]->x == assign){
			right = nodepointer[current]->rchild->nodeNo;
			regen_code(lev, nodepointer[current]->x, search_for_operand(current), search_for_operand(right));
		}
		else if (nodepointer[current]->rchild != NULL){
			left = nodepointer[current]->lchild->nodeNo;
			right = nodepointer[current]->rchild->nodeNo;
			regen_code(lev, nodepointer[current]->x, search_for_operand(current), search_for_operand(left), search_for_operand(right));
		}
		else{
			left = nodepointer[current]->lchild->nodeNo;
			regen_code(lev, nodepointer[current]->x, search_for_operand(current), search_for_operand(left));
		}
	}
	
	//给未赋值的局部变量和参数赋值
	for (k = 0; k <= ntable_p; k++){
		if ((nodetable[k].a.kind == VAR || nodetable[k].a.kind == PARA || nodetable[k].a.kind == VPARA)
			&& nodetable[k].a.type != ARRAY){
			var[k] = 1;
		}
	}
	
	isFirst = true;
	//current_nodeNo = -1;
	for (k = 0; k <= ntable_p; k++){
		if (var[k]==1){
			if (isFirst){
				current_nodeNo = nodetable[k].nodeNo;
				isFirst = false;
				var[k] = 0;
			}
			else{
				if (nodetable[k].nodeNo == current_nodeNo){
					if (nodepointer[current_nodeNo]->x == assign){
						right = nodepointer[current_nodeNo]->rchild->nodeNo;
						regen_code(lev, nodepointer[current]->x, nodetable[k].a, search_for_operand(right));
					}
					else if (nodepointer[current_nodeNo]->rchild != NULL){
						left = nodepointer[current_nodeNo]->lchild->nodeNo;
						right = nodepointer[current_nodeNo]->rchild->nodeNo;
						regen_code(lev, nodepointer[current]->x, nodetable[k].a, search_for_operand(left), search_for_operand(right));
					}
					else{
						left = nodepointer[current_nodeNo]->lchild->nodeNo;
						regen_code(lev, nodepointer[current]->x, nodetable[k].a, search_for_operand(left));
					}
					var[k] = 0;
				}
			}
		}
		if (k == ntable_p){
			isFirst = true;
		}
	}
}


//=====================活跃变量分析=====================
//(所有函数对code2[][]进行操作)

OPERAND ex_var[MAX_LEVEL][MAX_PROC_NUM][MAX_ITEM_NUM];//参与全局寄存器分配的局部变量及参数表（操作数类型为VAR(包括变量和数组名),PARA）
int ex_varp = -1;
int def[MAX_BBLOCK_NUM][MAX_ITEM_NUM];
int use[MAX_BBLOCK_NUM][MAX_ITEM_NUM];
int in[MAX_BBLOCK_NUM][MAX_ITEM_NUM];
int out[MAX_BBLOCK_NUM][MAX_ITEM_NUM];
int ConflictGraph[MAX_ITEM_NUM][MAX_ITEM_NUM];//冲突图
int edge_num[MAX_ITEM_NUM];                   //冲突图各结点的边数

REGISTER globle_reg[MAX_LEVEL][MAX_PROC_NUM][MAX_ITEM_NUM];      //全局寄存器分配结果
REGISTER regs[5] = { ebx, esi, edi ,eax,edx };

int get_exvar_index(int lev,int procNo,OPERAND op1){ //查询当前ex_var[]表中操作数op1的索引，若没有查找到，返回-1
	int i;
	for (i = ex_varp; i >=0; i--){
		if (operand_identical(op1, ex_var[lev][procNo][i])){
			break;
		}
	}
	return i;
}


void gen_exvar_item(int lev,int procNo,OPERAND op1){
	ex_varp++;
	ex_var[lev][procNo][ex_varp].kind = op1.kind;
	ex_var[lev][procNo][ex_varp].type = op1.type;
	ex_var[lev][procNo][ex_varp].lev = op1.lev;
	ex_var[lev][procNo][ex_varp].val = op1.val;
}

void initial_exvar(int lev, int procNo,int start, int end){//计算一个过程的变量参数表，运行结束后ex_varp为变量和参数个数
	for (int i = start; i <= end; i++){
		for (int j = 0; j <= 2; j++){
			if ((code2[lev][i].operand[j].kind == VAR || code2[lev][i].operand[j].kind == PARA)
				&& get_exvar_index(lev,procNo,code2[lev][i].operand[j]) == -1){
				gen_exvar_item(lev,procNo,code2[lev][i].operand[j]);
			}
		}
	}
}

bool isDef(QFRT mid_code,OPERAND op1){//判断操作数op1在中间代码mid_code中是否被定义
	if ((mid_code.opt==assign&&operand_identical(mid_code.operand[0],op1))
		|| (mid_code.opt == add &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == sub &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == mul &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == div &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == neg &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == call &&operand_identical(mid_code.operand[1], op1))
		|| (mid_code.opt == arrayr &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == loopup &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == loopdown &&operand_identical(mid_code.operand[0], op1))){
			return true;
		}
	else{
		return false;
	}
}

bool isUse(QFRT mid_code, OPERAND op1){
	if ((mid_code.opt == assign&&operand_identical(mid_code.operand[1], op1))
		|| (mid_code.opt == add &&(operand_identical(mid_code.operand[1], op1) || operand_identical(mid_code.operand[2], op1)))
		|| (mid_code.opt == sub && (operand_identical(mid_code.operand[1], op1) || operand_identical(mid_code.operand[2], op1)))
		|| (mid_code.opt == mul && (operand_identical(mid_code.operand[1], op1) || operand_identical(mid_code.operand[2], op1)))
		|| (mid_code.opt == div && (operand_identical(mid_code.operand[1], op1) || operand_identical(mid_code.operand[2], op1)))
		|| (mid_code.opt == neg &&operand_identical(mid_code.operand[1], op1))
		|| (mid_code.opt == arrayl && (operand_identical(mid_code.operand[0], op1) || operand_identical(mid_code.operand[1], op1)||operand_identical(mid_code.operand[2],op1)))
		|| (mid_code.opt == arrayr && (operand_identical(mid_code.operand[1], op1) || operand_identical(mid_code.operand[2], op1)))
		|| (mid_code.opt == write && (operand_identical(mid_code.operand[0], op1) || operand_identical(mid_code.operand[1], op1)))
		|| (mid_code.opt == read &&operand_identical(mid_code.operand[0], op1))
		|| (mid_code.opt == cmp && (operand_identical(mid_code.operand[0], op1) || operand_identical(mid_code.operand[1], op1)))
		|| (mid_code.opt == loopup && (operand_identical(mid_code.operand[0], op1) || operand_identical(mid_code.operand[1], op1)))
		|| (mid_code.opt == loopdown && (operand_identical(mid_code.operand[0], op1) || operand_identical(mid_code.operand[1], op1)))){
		return true;
	}
	else{
		return false;
	}
}


void get_DefUse(int lev,int procNo,int max_bblock_index){
	int i, j,k;
	int firstDef, firstUse;
	for ( i = 0; i < max_bblock_index; i++){
		for (j = 0; j <= ex_varp; j++){
			for (k = bblocks[i].bbstart; k <= bblocks[i].bbend; k++){
				if (isDef(code2[lev][k], ex_var[lev][procNo][j])){
					break;
				}
			}
			firstDef = k;
			for (k = bblocks[i].bbstart; k <= bblocks[i].bbend; k++){
				if (isUse(code2[lev][k], ex_var[lev][procNo][j])){
					break;
				}
			}
			firstUse = k;
			if (firstDef == bblocks[i].bbend + 1 && firstUse == bblocks[i].bbend + 1){
				def[i][j] = 0;
				use[i][j] = 0;
			}
			else if (firstUse <= firstDef){
				use[i][j] = 1;
				def[i][j] = 0;
			}
			else {
				use[i][j] = 0;
				def[i][j] = 1;
			}
		}
	}
}

void initial_LVA(int max_block_index){
	for (int i = 0; i <= max_block_index; i++){
		for (int j = 0; j <= ex_varp; j++){
			def[i][j] = 0;
			use[i][j] = 0;
			in[i][j] = 0;
			out[i][j] = 0;
		}
	}
	for (int i = 0; i <= ex_varp; i++){
		for (int j = 0; j <= ex_varp; j++){
			ConflictGraph[i][j] = 0;
		}
		edge_num[i] = -1;//edge_num[]=-1表示该变量不在冲突图内，不参与全局寄存器分配
	}
}


void Union(int *A,int *B){//将B并入A，A和B均用ex_varp维向量表示
	for (int i = 0; i <= ex_varp; i++){
		if (A[i] == 1 || B[i] == 1){
			A[i] = 1;
		}
		else{
			A[i] = 0;
		}
	}
}


void LiveVariableAnalysis(int lev,int procNo,int max_blocks_index,int procStart,int procEnd){
	bool isChanged = true;//记录每一遍计算是否有in[B]发生改变
	
	ex_varp = -1;
	initial_exvar(lev, procNo,procStart, procEnd);
	//print_exvar(lev,procNo);//打印局部变量和参数表，用于调试
	initial_LVA(max_blocks_index);
	get_DefUse(lev, procNo,max_blocks_index);
	//print_set(max_blocks_index, "def[B]", def);//打印def[B]，用于调试
	//print_set(max_blocks_index, "use[B]", use);//打印use[B]，用于调试
	
	while (isChanged){                          //计算in[B],out[B]，书上算法11.5
		int pre_in;
		isChanged = false;
		for (int k = max_blocks_index-1; k >=0; k--){
			if (bblocks[k].later_bb_index[0] != -1){
				Union(out[k], in[bblocks[k].later_bb_index[0]]);
			}
			if (bblocks[k].later_bb_index[1] != -1){
				Union(out[k], in[bblocks[k].later_bb_index[1]]);
			}
			for (int i = 0; i <= ex_varp; i++){
				pre_in = in[k][i];
				if ((out[k][i] == 1 && def[k][i] == 0)||use[k][i]==1){
					in[k][i] = 1;
				}
				else{
					in[k][i] = 0;
				}
				if (in[k][i] != pre_in){
					isChanged = true;
				}
			}
		}
	}
	//print_set(max_blocks_index, "in[B]", in);//打印in[B]，用于调试
	//print_set(max_blocks_index, "out[B]", out);//打印out[B]，用于调试
	
	for (int i = 0; i < max_blocks_index; i++){//构建冲突图
		for (int j = 0; j <= ex_varp; j++){
			if (in[i][j] == 1){
				for (int k = 0 ; k <= ex_varp; k++){
					if (in[i][k] == 1){
						ConflictGraph[j][k] = 1;//每个变量对应的行在自己的列处为1，即在冲突图中的变量对应行至少有一个1，不在冲突图中的变量对应行全零
					}
				}
			}
		}
	}
	//print_ConflictGraph();//打印冲突图，用于调试
	for (int i = 0; i <= ex_varp; i++){//计算edge_num[]
		for (int j = 0; j <= ex_varp; j++){
			if (ConflictGraph[i][j] == 1){
				edge_num[i]++;
			}
		}
	}
}

//=============全局寄存器分配(图着色算法)================

void initial_globle_reg(){//初始化全局寄存器分配结果，在optimize()中调用
	for (int i = 0; i < MAX_LEVEL; i++){
		for (int j = 0; j < MAX_PROC_NUM; j++){
			for (int k = 0; k < MAX_ITEM_NUM; k++){
				globle_reg[i][j][k] = non;
			}
		}
	}
}

void ColorDistribution(int lev,int procNo){
	int i;
	int reg_index = 0;
	bool isRemain = true;//记录冲突图中是否还有结点剩余
	bool canMove = true;//记录上一遍操作中是否有结点移出

	for (int i = 0; i <= ex_varp; i++){
		globle_reg[lev][procNo][i] = null_reg;
	}

	while (isRemain){
		isRemain = false;
		if (canMove){
			canMove = false;
			for (i = 0; i <= ex_varp; i++){
				if (edge_num[i] >= 0){
					isRemain = true;
					if (edge_num[i] < GLOBLE_REG_NUM){//若连接边数小于GLOBLE_REG_NUM
						//printf("移出第%d个变量，分配寄存器，边数：%d\n", i, edge_num[i]);
						canMove = true;
						globle_reg[lev][procNo][i] = regs[reg_index%GLOBLE_REG_NUM];//分配全局寄存器
						reg_index++;
						edge_num[i] = -1;                                           //移出结点
						for (int p = 0; p <= ex_varp; p++){
							if (ConflictGraph[i][p] == 1 && p != i){
								edge_num[p]--;
							}
						}
					}
				}
			}
		}
		else {
			for (i = 0; i <= ex_varp; i++){
				if (edge_num[i] >= 0){
					//printf("移出第%d个变量，不分配寄存器，边数：%d\n", i, edge_num[i]);
					isRemain = true;
					canMove = true;
					edge_num[i] = -1;            //结点直接移出
					for (int p = 0; p <= ex_varp; p++){
						if (ConflictGraph[i][p] == 1 && p != i){
							edge_num[p]--;
						}
					}
					break;
				}
			}
		}
	}
	//print_globle(lev,procNo);//打印全局寄存器分配结果，用于调试
}

void apply_globle(int lev,int procNo,int procStart,int procEnd){
	for (int i = procStart; i <= procEnd; i++){
		for (int j = 0; j < 3; j++){
			for (int k = 0; k <= ex_varp; k++){
				if (operand_identical(code2[lev][i].operand[j], ex_var[lev][procNo][k])&&globle_reg[lev][procNo][k]!=null_reg){
					code2[lev][i].operand[j].kind = REG;
					code2[lev][i].operand[j].type = NUL;
					code2[lev][i].operand[j].lev = -1;
					code2[lev][i].operand[j].val = globle_reg[lev][procNo][k];
				}
			}
		}
	}
}


//=========主优化函数===========
void optimize(){
	int i, j, max_bblock_index,i2;
	initial_cx2();
	initial_globle_reg();
	for (i = max_l; i >= 0; i--){
		int proc_start, proc_end = -1;
		int code2_start,procIndex=-1;
		//printf("cx[%d]=%d\n", i, cx[i]);//用于调试
		while (proc_end != cx[i]){
			proc_start = proc_end + 1;
			//printf("proc_start=%d\n", proc_start);//打印该过程起始位置，用于调试
			j = proc_start;
			while (code[i][j].opt != ret&&j<cx[i]){
				j++;
			}
			proc_end = j;
			procIndex++;
			//printf("proc_end=%d\n", proc_end);//打印该过程结束位置，用于调试
			//////////////////////////////////////////////////////////////////////将中间代码code[][]中本过程划分基本块
			max_bblock_index=bblocks_dividing_1(code,i, proc_start, proc_end);
			bblocks_dividing_2(code,i, max_bblock_index);
			//print_bblocks(max_bblock_index);//打印基本块划分结果，用于调试
			code2_start = cx2[i] + 1;
			/////////////////////////////////////////////////////////////////////消除局部公共子表达式
			for (int k = 0; k < max_bblock_index; k++){
				int start, end;
				i2 = bblocks[k].bbstart;
				while ( i2 <= bblocks[k].bbend){//将参与DAG图优化的连续中间代码段分别优化
					if (!isDAG(code[i][i2].opt)){
						regen_code(i, code[i][i2].opt, code[i][i2].operand[0], code[i][i2].operand[1], code[i][i2].operand[2]);
						i2++;
					}
					else{
						start = i2;
						while (isDAG(code[i][i2].opt)&&i2<=bblocks[k].bbend){
							i2++;
						}
						end = i2 - 1;
						Q2DAG(i, start, end);
						//print_DAG();//打印DAG图，用于调试
						//print_nodetable();//打印结点表，用于调试
						DAG2Q(i);
					}
				}
			}
			/////////////////////////////////////////////////////////////////////生成与code[i][]该过程对应的code2[i][]，下面对该部分code2[i][]划分基本块
			max_bblock_index = bblocks_dividing_1(code2, i, code2_start, cx2[i]);
			bblocks_dividing_2(code2, i, max_bblock_index);
			//print_bblocks(max_bblock_index);//打印基本块划分结果，用于调试
			LiveVariableAnalysis(i, procIndex,max_bblock_index, code2_start, cx2[i]);//活跃变量分析构建冲突图
			ColorDistribution(i, procIndex);//图着色算法分配全局寄存器
			//apply_globle(i, procIndex, code2_start, cx2[i]);//应用于中间代码

		}
	}
}