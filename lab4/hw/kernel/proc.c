
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
	PROCESS* p;
	int	 greatest_ticks = 0;

	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table+NR_TASKS; p++) {
			if (p->ticks > greatest_ticks && p->wakeup_ticks <= get_ticks()) {
				disp_int(p-proc_table);
				disp_str("<");
				disp_int(p->ticks);
				disp_str(">  ");
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		// if (!greatest_ticks) {
		// 	for (p = proc_table; p < proc_table+NR_TASKS; p++) {
		// 		if(p->wakeup_ticks <= get_ticks()){
		// 			p->ticks = p->priority;
		// 		}
		// 	}
		// }
	}
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
PUBLIC void sys_myprint(char* s)
{
	disp_str(s);
}

/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int milli_seconds)
{
	p_proc_ready->wakeup_ticks = get_ticks() + milli_seconds / (1000 /HZ);
	disp_str("sleep");
	disp_int(p_proc_ready->wakeup_ticks);
	schedule();
}



