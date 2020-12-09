
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

	proc_table[0].priority = 2;
	proc_table[1].priority = 2;
	proc_table[2].priority = 2;
	proc_table[3].priority = 3;

	proc_table[0].ticks = proc_table[0].needTime = 2;
	proc_table[1].ticks = proc_table[1].needTime = 3;
	proc_table[2].ticks = proc_table[2].needTime = 3;
	proc_table[3].ticks = proc_table[3].needTime = 1;

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	/*初始化信号量相关*/
	readNum = 3;
	readMutex.value = readNum;
	writeNum = 1;
	writeMutex.value = writeNum;

	// fake
	nowStatus = 'r';

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
                               TestA
 *======================================================================*/
void TestA()
{
	int i = 0;
	while (1)
	{
		P(&readMutex);
		int j;
		// printColorStr("Astart.   ", 'r');
		printColorStr("A start.  ", 'r');
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("read A.   ", 'r');
			if(j==p_proc_ready->needTime-1){
				printColorStr("A end.    ", 'r');
			}
			milli_delay(10);
		}
		V(&readMutex);
		
		// p_proc_ready->isDone = 1;
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while (1)
	{

		P(&readMutex);
		printColorStr("B start.  ", 'g');
		int j;
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("read B.   ", 'g');
			if(j==p_proc_ready->needTime-1){
				printColorStr("B end.    ", 'g');
			}
			milli_delay(10);
		}
		V(&readMutex);
		// p_proc_ready->isDone = 1;
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while (1)
	{

		P(&readMutex);
		printColorStr("C start.  ", 'b');
		int j;
		for (j = 0; j < p_proc_ready->needTime; ++j)
		{
			printColorStr("read C.   ", 'b');
				if(j==p_proc_ready->needTime-1){
				printColorStr("C end.    ", 'b');
			}
			milli_delay(10);
		}
		// printColorStr("C.        ", 'b');
		// printColorStr("Cend.     ", 'b');

		V(&readMutex);
		// p_proc_ready->isDone = 1;
	}
}

void F()
{
	while (1)
	{
		if (nowStatus == 'r')
		{
			printColorStr("now read ",'w');
			int nowRead;
			if (readMutex.value >= 0)
			{
				nowRead = readNum - readMutex.value;
			}
			else
			{
				nowRead = readNum;
			}

			char num = '0' + nowRead - 0;
			char tem[2] = {num, '\0'};
			printColorStr(tem,'w');
		}
		else
		{
			printColorStr("now write ",'w');
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
	default:
		disp_str(s);
		break;
	}
}