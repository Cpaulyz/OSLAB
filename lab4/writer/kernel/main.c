
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].type = proc_table[1].type = proc_table[2].type = 'r';
	proc_table[3].type = proc_table[4].type  = 'w';

	int readPriority = 1;
	int writePriority = 2;

	proc_table[0].priority = readPriority; //A
	proc_table[1].priority = readPriority; //B
	proc_table[2].priority = readPriority; //C
	proc_table[3].priority = writePriority; //D
	proc_table[4].priority = writePriority; //E
	proc_table[5].priority = 99; //F

	proc_table[0].ticks = proc_table[0].needTime = 2;
	proc_table[1].ticks = proc_table[1].needTime = 3;
	proc_table[2].ticks = proc_table[2].needTime = 3;
	proc_table[3].ticks = proc_table[3].needTime = 3;
	proc_table[4].ticks = proc_table[4].needTime = 4;
	proc_table[5].ticks = proc_table[5].needTime = 1;

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	/*初始化信号量相关*/
	readNum = 1;
	readMutex.value = readNum;
	writeNum = 1;
	writeMutex.value = writeNum;
	readCountMutex.value = 1;
	writeCountMutex.value = 1;
	readPermission.value = 0;
	readPermissionMutex.value = 1;

	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);					   /* 让8259A可以接收时钟中断 */
	disp_pos = 0;
	for (i = 0; i < 80 * 25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;
	restart();

	while (1)
	{
	}
}

/*======================================================================*
                               A
 *======================================================================*/
void A()
{
	int i = 0;
	while (1)
	{
		P(&readPermissionMutex);
		if(writeCount!=0){
			P(&readPermission);
		}
		V(&readPermissionMutex);

		P(&readMutex);

		// 判断修改在读人数
		P(&readCountMutex);
		if(readCount==0){
			P(&writeMutex);
		}
		readCount++;
		V(&readCountMutex);

		int j;
		printColorStr("A start.  ", 'r');
		// printColorStr("<A,R,+> ", 'r');
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("A reading.", 'r');
			if(j==p_proc_ready->needTime-1){
				printColorStr("A end.    ", 'r');
			}else{
				milli_delay(10);
			}
		}


		P(&readCountMutex);
		readCount--;
		if(readCount==0){
			V(&writeMutex);
		}
		V(&readCountMutex);

		V(&readMutex);
		
		p_proc_ready->isDone = 1;
			milli_delay(10);
	}
}

/*======================================================================*
                               B
 *======================================================================*/
void B()
{
	int i = 0x1000;
	while (1)
	{		
		P(&readPermissionMutex);
		if(writeCount!=0){
			P(&readPermission);
		}
		V(&readPermissionMutex);

		P(&readMutex);

		// 判断修改在读人数
		P(&readCountMutex);
		if(readCount==0){
			P(&writeMutex);
		}
		readCount++;
		V(&readCountMutex);

		printColorStr("B start.  ", 'g');
		int j;
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("B reading.", 'g');
			if(j==p_proc_ready->needTime-1){
				printColorStr("B end.    ", 'g');
			}else{
				milli_delay(10);
			}
		}
		
		P(&readCountMutex);
		readCount--;
		if(readCount==0){
			V(&writeMutex);
		}
		V(&readCountMutex);

		V(&readMutex);
		p_proc_ready->isDone = 1;
			milli_delay(10);
	}
}

/*======================================================================*
                               C
 *======================================================================*/
void C()
{
	int i = 0x2000;
	while (1)
	{		
		P(&readPermissionMutex);
		if(writeCount!=0){
			P(&readPermission);
		}
		V(&readPermissionMutex);

		P(&readMutex);		
		
		// 判断修改在读人数
		P(&readCountMutex);
		if(readCount==0){
			P(&writeMutex);
		}
		readCount++;
		V(&readCountMutex);

		printColorStr("C start.  ", 'b');
		int j;
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("C reading.", 'b');
			if(j==p_proc_ready->needTime-1){
				printColorStr("C end.    ", 'b');
			}else{
				milli_delay(10);
			}
		}

		P(&readCountMutex);
		readCount--;
		if(readCount==0){
			V(&writeMutex);
		}
		V(&readCountMutex);
		
		V(&readMutex);
		p_proc_ready->isDone = 1;
			milli_delay(10);
	}
}
void D(){
	
		mysleep(10);
	while (1)
	{
		P(&writeCountMutex);
		writeCount++;
		V(&writeCountMutex);

		P(&writeMutex);
		printColorStr("D start.  ", 'p');
		int j;
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("D writing.", 'p');
			if(j==p_proc_ready->needTime-1){
				printColorStr("D end.    ", 'p');
			}else{
				milli_delay(10);
			}
		}
		V(&writeMutex);

		P(&writeCountMutex);
		writeCount--;
		if(writeCount==0&&readPermission.value<0){ // 如果有读进程在等待
			V(&readPermission);
		}
		V(&writeCountMutex);
		
		p_proc_ready->isDone = 1;
		milli_delay(10);
	}
	
}
void E(){
		mysleep(10);
	while (1)
	{
		P(&writeCountMutex);
		writeCount++;
		V(&writeCountMutex);

		P(&writeMutex);
		printColorStr("E start.  ", 'y');
		int j;
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("E writing.", 'y');
			if(j==p_proc_ready->needTime-1){
				printColorStr("E end.    ", 'y');
			}else{
				milli_delay(10);
			}
		}
		V(&writeMutex);

		P(&writeCountMutex);
		
		writeCount--;
		if(writeCount==0&&readPermission.value<0){ // 如果有读进程在等待
			V(&readPermission);
			
		}
		V(&writeCountMutex);
		
		p_proc_ready->isDone = 1;
		milli_delay(10);
	}
	
}
void F()
{
	while (1)
	{
		if (nowStatus == 'r')
		{
			printColorStr("<read==",'w');
			char num = '0' + readCount - 0;
			char tem[4] = {num,'>',' ','\0'};
			printColorStr(tem,'w');
		}
		else if(nowStatus == 'w')
		{
			printColorStr("<writing> ",'w');
		}else{
			printColorStr("<<START>> ",'w');
		}

		mysleep(10);
	}
}

void printColorStr(char* s,char color){
	if(disp_pos>80*25*2){
		return;
	}
	switch (color)
	{
	case 'r':
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, RED));
		break;	
	case 'g':
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, GREEN));
		break;	
	case 'b':
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, BLUE));
		break;	
	case 'w':
		disp_str(s);
		break;
	case 'y':
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, YELLO));
		break;	
	case 'p':
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, PURPLE));
		break;	
	default:
		disp_str(s);
		break;
	}
}