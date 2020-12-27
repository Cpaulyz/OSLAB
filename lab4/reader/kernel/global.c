
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"


PUBLIC	PROCESS			proc_table[NR_TASKS];

PUBLIC  PROCESS* pre_proc = proc_table;

PUBLIC	char			task_stack[STACK_SIZE_TOTAL];

PUBLIC	TASK	task_table[NR_TASKS] = {{A, STACK_SIZE_A, "A"},
					{B, STACK_SIZE_B, "B"},
					{C, STACK_SIZE_C, "C"},
					{D, STACK_SIZE_D, "D"},
					{E, STACK_SIZE_E, "E"},
					{F, STACK_SIZE_F, "F"}};

PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks,
                                                    sys_myprint,
                                                    sys_sleep,
                                                    sys_p,
                                                    sys_v};


PUBLIC char writeStr[] = " writing.";
PUBLIC char readStr[] = " reading.";
PUBLIC char endStr[] = " end.    ";