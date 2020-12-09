
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
	
	PROCESS *select;
	for (select = proc_table; select < proc_table + NR_TASKS; select++)
	{
		if (isRunnable(select))
		{
			break;
		}
	} // 找到第一个可运行，假设一定存在
	PROCESS *p;
	for (p = proc_table; p < proc_table + NR_TASKS; p++)
	{
		if (isRunnable(p))
		{
			if(p->priority > select->priority){
				select = p;
			}else if(p->priority == select->priority && p->useTime < select->useTime){
				select = p;
			}
		}
	}
	select->useTime++;
	p_proc_ready = select;
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
	disp_str(s);
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
	//然后整体往前移,现在value指的是队列中剩下元素个数的负数
	int i;
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