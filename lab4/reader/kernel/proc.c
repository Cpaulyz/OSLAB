
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	disable_irq(CLOCK_IRQ);
	
	// 先检查一遍，如果都Done了，重新开始
	check();
	// 新版
	PROCESS *select = proc_table+5;
	if(isRunnable(select)){
		//nothing
		p_proc_ready = select;
	}else{
		while (!isRunnable(pre_proc))
		{
			pre_proc++;
			if(pre_proc==select){
				pre_proc = proc_table;
			}
		}
		p_proc_ready = pre_proc;
		pre_proc++;
			if(pre_proc==select){
				pre_proc = proc_table;
			}

	}
	if(p_proc_ready->type=='r'||p_proc_ready->type=='w'){ // 修改状态，供F打印
		nowStatus = p_proc_ready->type;
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}
/*======================================================================*
                           sys_myprint
 *======================================================================*/
PUBLIC void sys_myprint(char *s)
{
	int offset = p_proc_ready - proc_table;
	switch (offset)
	{
	case 0:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, RED));
		break;
	case 1:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, GREEN));
		break;
	case 2:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, BLUE));
		break;
	case 5:
		disp_str(s);
		break;
	case 3:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, PURPLE));
		break;
	case 4:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, YELLO));
		break;
	default:
		disp_str(s);
		break;
	}
}

/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int milli_seconds)
{
	p_proc_ready->wakeup_ticks = get_ticks() + milli_seconds / (1000 / HZ);
	schedule();
}

/*======================================================================*
                           sys_p
 *======================================================================*/
PUBLIC void sys_p(void *mutex)
{
	disable_irq(CLOCK_IRQ); //保证原语
	Semaphore *semaphore = (Semaphore *)mutex;

	semaphore->value--;
	if (semaphore->value < 0)
	{
		sleep(semaphore);
	}
	enable_irq(CLOCK_IRQ);
}
/*======================================================================*
                           sys_v
 *======================================================================*/
PUBLIC void sys_v(void *mutex)
{
	disable_irq(CLOCK_IRQ); //保证原语
	Semaphore *semaphore = (Semaphore *)mutex;

	semaphore->value++;
	if (semaphore->value <= 0)
	{
		wakeup(semaphore);
	}
	enable_irq(CLOCK_IRQ);
}

PUBLIC void sleep(Semaphore *mutex)
{
	mutex->queue[-mutex->value - 1] = p_proc_ready;
	p_proc_ready->isBlock = 1; // 阻塞
	schedule();
}

PUBLIC void wakeup(Semaphore *mutex)
{

	PROCESS *wake = mutex->queue[0];
	int i,index=0;
	for (i = 0; i < (-mutex->value); ++i)
	{
		mutex->queue[i] = mutex->queue[i + 1];
	}
	wake->isBlock = 0;
}

PUBLIC int isRunnable(PROCESS* p){
	if(p->wakeup_ticks <= get_ticks()&&p->isBlock == 0&&p->isDone ==0){
		return 1;
	}else{
		return 0;
	}
}

void check(){
	PROCESS *p;
	int allDone = 1;
	for (p = proc_table; p < proc_table + NR_TASKS-1; p++){
		if(p->isDone==0){
			allDone = 0;
			return;
		}
	}
	if(allDone==1){
		//如果全部做完了，任务重启
		for (p = proc_table; p < proc_table + NR_TASKS-1; p++){
			p->isDone = 0;
		}
		disp_str("<RESTART> ");
	}
}