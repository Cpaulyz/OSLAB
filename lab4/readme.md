## 添加新task

![image-20201208105527572](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201208105527572.png)

## 封装系统调用

### myprint

`proto.h`中

```c
PUBLIC  void    myprint(char* s);  /*封装打印的系统调用*/ 
PUBLIC  void     sys_myprint(char* s);
```

修改`const.h`，`#define NR_SYS_CALL   2`

在`global.c`中，修改

```c
PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks,
                                                    sys_myprint};
```

`syscall.asm`

```assembly
_NR_myprint      	equ 1 ; 
global	myprint
...
myprint:
	mov	eax, _NR_myprint
	mov ecx,[esp+4]
	int	INT_VECTOR_SYS_CALL
	ret
```

`proc.c`

```c
/*======================================================================*
                           sys_myprint
 *======================================================================*/
PUBLIC void sys_myprint(char* s)
{
	disp_str(s);
}
```

因为进行了参数传递，所以需要在进行系统调用时候也将参数压栈且出栈

`kernal.asm`

```assembly
sys_call:
        call    save
        sti
		push	ecx
        call    [sys_call_table + eax * 4]
        add 	esp,4
		mov     [esi + EAXREG - P_STACKBASE], eax
        cli
        ret
```

### mysleep

同上的步骤跳过，仅叙述不同的

1. 给PROCESS结构体加上`  int wakeup_ticks;  // 睡醒的时刻`

2. ```c
	/*======================================================================*
	                           sys_sleep
	 *======================================================================*/
	PUBLIC void sys_sleep(int milli_seconds)
	{
		p_proc_ready->wakeup_ticks = get_ticks() + milli_seconds / (1000 / HZ);
		schedule();
	}
	```

3. 添加辅助方法，用于判断进程是否可用

	```c
	PUBLIC int isRunnable(PROCESS* p){
		if(p->wakeup_ticks <= get_ticks()&&p->isBlock == 0&&p->isDone ==0){
			return 1;
		}else{
			return 0;
		}
	}
	```

### PV操作

```c
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
```

## 读者优先

### 修改PROCESS

```c
typedef struct s_proc
{
	STACK_FRAME regs; /* process registers saved in stack frame */

	u16 ldt_sel;			   /* gdt selector giving ldt base and limit */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

	int ticks; /* remained ticks */
	int priority;
	int wakeup_ticks;	// 睡醒的时刻
	int needTime;		// 需要的时间片
	int useTime;		// ·
	int isBlock;		// 1，被阻塞；0.非阻塞
	u32 pid;		 /* process id passed in from MM */
	char p_name[16]; /* name of the process */
} PROCESS;

```

对此进行一些说明

* ticks，废除了，没用
* priority，优先级，比如读者有限，则读者进程优先级高于写者进程。其中F进程优先级最高
* wakeup_ticks，使用mysleep后睡眠
* needTime，需要的时间片
* useTime，已使用的时间片，调度算法总会选择优先级最高进程中使用时间片最少的进程
* isBlock，阻塞状态，pv操作使用，如果在P时候被sleep，则Block；V时wake，则取消Block

### 时间中断处理

```c
/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	ticks++;
	p_proc_ready->ticks--;
	// if (k_reenter != 0) {
	// 	return;
	// }

	// 若开启，则为非抢占式
	// if (p_proc_ready->ticks > 0) {
	// 	return;
	// }
	// if(p_proc_ready->isBlock!=1){
	schedule();
}

```

去掉原有代码一些不必要的东西

### 信号量

```c
// global.h
typedef struct semaphore{
    int value;
    PROCESS* queue[NR_TASKS];
}Semaphore;


EXTERN Semaphore readMutex;
EXTERN Semaphore writeMutex;
EXTERN Semaphore countMutex;
EXTERN int readNum;     // 允许同时读的个数
EXTERN int writeNum;    // 允许同时写的个数，默认为1

EXTERN char nowStatus;
```

对ABC进行进行PV操作，目前仅实现读者有限，以A为例

```c
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
	}
}
```

F进程

```c
void F()
{
	while (1)
	{
        ...
		// 打印信息
		mysleep(10);
	}
}
```



### 调度算法

想法：

* 选择优先级最高的进程，优先运行useTime最少的进程
* 因为F进程优先级最高，所以如果有F，一定运行F，所以需要将F进程使用mysleep进行睡眠，避免其使用时间片
* 而在ABC进程中，没读一次进行`milli_delay(10);`操作，也就是让这个时间片内不允许做任何事，在进入到下个时间片后，程序由时间中断跳入`schedule`，进行进程调度，然后再解决下个时间片的事情
* F的实现方式其实是一种伪实现的方式，本质是在同一个时间片内先时间中断，然后由调度算法必定命中F，打印信息后，进入`mysleep(10);`，也就是等待下个时间片，然后在`mysleep`中再次进行`schedule()`，执行读写进程

```c
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
```

### 结果

**并发量1**

![image-20201210002202015](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201210002202015.png)

**并发量2**

![image-20201210002109412](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201210002236264.png)

**并发量3**

![image-20201210002236264](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201210002109412.png)