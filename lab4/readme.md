## 1 添加新task

![image-20201208105527572](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201208105527572.png)

## 2 封装系统调用

### 2.1 myprint

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

### 2.2 mysleep

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

### 2.3 PV操作

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
	int i,index=0;
	// 选出优先级最高的进程，优先唤醒
	for (i = 0; i < (-mutex->value+1); ++i)
	{
		if(mutex->queue[i]->priority > wake->priority){
			wake = mutex->queue[i];
			index = i;
		}
	}
	//然后整体往前移,现在value指的是队列中剩下元素个数的负数
	for (i = index; i < (-mutex->value); ++i)
	{
		mutex->queue[i] = mutex->queue[i + 1];
	}
	wake->isBlock = 0;
}
```

## 3 读者优先

### 3.1 理解

* 如果有读者在读，写者就得一直等待；
* 已有进程运行时，有写者和读者进入，即使读者晚于写者到来，也可以优先进入临界区进行读
* 读者进程的优先级高于写者进程，这意味着在进程调度和睡眠唤醒时候都是如此；

### 3.2 修改PROCESS

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
	int isDone;			// 1，已完成；0，未完成
	char type;			// 'r'/'w'
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

### 3.3 时间中断处理

```c
/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	ticks++;
	p_proc_ready->ticks--;
	schedule();
}

```

去掉原有代码一些不必要的东西

### 3.4 信号量PV操作

```c
// global.h
typedef struct semaphore{
    int value;
    PROCESS* queue[NR_TASKS];
}Semaphore;


EXTERN Semaphore readMutex; // value=并发数
EXTERN Semaphore writeMutex;// value=1
EXTERN Semaphore countMutex;// value=1
EXTERN Semaphore writeMutexMutex;// value=1
```

**读者进程伪代码**：

```c
void reader()
{
    while(1){
		P(&countMutex);
			if (readPreparedCount == 0){ 
             P(&writeMutex);
          }
			readPreparedCount++;
		V(&countMutex);

		P(&readMutex);
			readCount++; // 记录正在读的数量，供F打印
			// 读文件
			readCount--;
		V(&readMutex);

		P(&countMutex);
			readPreparedCount--;
			if (readPreparedCount == 0){
				V(&writeMutex);
			}
		V(&countMutex);
    
		p_proc_ready->isDone = solveHunger; // 解决饿死
		milli_delay(10); // 废弃当前时间片，至少等到下个时间片才能进入循环    
    }
}
```

**写者进程伪代码**

```c
void writer(){
    while (1){	
		P(&writeMutexMutex); // 只允许一个写者进程在writeMutex排队，其他写者进程只能在writeMutexMutex排队
		P(&writeMutex);
			//写文件
		V(&writeMutex);
		V(&writeMutexMutex);

		p_proc_ready->isDone = solveHunger; // 解决饿死
		milli_delay(10); // 废弃当前时间片，至少等到下个时间片才能进入循环 
	}
}
```

**F进程**

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

### 3.5 调度算法

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
	if(p_proc_ready->type=='r'||p_proc_ready->type=='w'){ // 修改状态，供F打印
		nowStatus = p_proc_ready->type;
	}
	enable_irq(CLOCK_IRQ);
}
```

### 3.6 结果

> 为了体现读者优先，我们在读者进程开头加上`mysleep(10)`，也就是让写者进程先到达

#### 并发量1

读者进程到达后，写者进程被饿死，这很好理解，因为不断有读者进程在排队，而读者进程优先级高，所以轮不到写者进程

![image-20201214211659778](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214211659778.png)



#### 并发量2

读者进程到达后，写者进程被饿死，这很好理解，因为不断有读者进程在排队，而读者进程优先级高，所以轮不到写者进程

![image-20201214211801956](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214211801956.png)

#### 并发量3

读者进程到达后，写者进程被饿死，这很好理解，因为不断有读者进程在排队，而读者进程优先级高，所以轮不到写者进程

![image-20201214211840976](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214220154791.png)

## 4 写者优先

### 4.1 理解

* 已有进程运行时，有写者和读者进入，即使读者晚于写者到来，也优先进行写操作

	例如，对于到达顺序【R1】【W1】【R2】【W2】【R3】而言，运行顺序为

	【R1】【W1】【W2】【R2】【R3】

* 写者进程的优先级高于读者进程，这意味着在进程调度和睡眠唤醒时候都是如此；

### 4.2 信号量PV操作

使用的信号量如下

```c
typedef struct semaphore{
    int value;
    PROCESS* queue[NR_TASKS];
}Semaphore;


EXTERN Semaphore readMutex; // value=1 ，保证只有一个在读
EXTERN Semaphore writeMutex; // value=读并发数
EXTERN Semaphore readCountMutex;// value=1
EXTERN Semaphore writeCountMutex;// value=1
EXTERN Semaphore readPermission; // value=1
EXTERN Semaphore readPermissionMutex; // // value=1，保证只有一个读者被卡在readPermission
```

**读进程伪代码如下：**

```c
void reader()
{
	while (1){
		P(&readPermissionMutex); // 保证只有一个被卡在readPermission
		P(&readPermission); // 判断修改在读人数
		P(&readCountMutex);
			if (readPreparedCount == 0){
				P(&writeMutex);
			}
			readPreparedCount++;
		V(&readCountMutex);
		V(&readPermission);
		V(&readPermissionMutex);

		P(&readMutex);
			readCount++;
			//读操作
			readCount--;
		V(&readMutex);

		P(&readCountMutex);
			readPreparedCount--;
			if (readPreparedCount == 0){
				V(&writeMutex);
			}
		V(&readCountMutex);

		p_proc_ready->isDone = solveHunger;
		milli_delay(10);
	}
}
```

**写进程伪代码：**

```c
void writer(char process)
{
	while (1){
		P(&writeCountMutex);
			writeCount++;
			if(writeCount==1){
				P(&readPermission);
			}
		V(&writeCountMutex);

		P(&writeMutex);
		// 写操作
		V(&writeMutex);

		P(&writeCountMutex);
			writeCount--;
			if (writeCount == 0){ 
				V(&readPermission);
			}
		V(&writeCountMutex);

		p_proc_ready->isDone = solveHunger;
		milli_delay(10);
	}
}
```

### 4.3 结果

> 为了体现写者优先，我们在写者进程开头加上`mysleep(10)`，也就是让读者进程先到达

并发量为1、2、3的情况都是一样的，写者进程到达后，原先排队的读者进程就很难再进入。这里和读者优先有一些不同在于，读者进程并不会被饿死，而会“偷”到一些机会进行运行。

这其实与写者优先**并不矛盾**，实际上是**由于调度算法决定**的，本人在实验中使用的调度算法是优先级调度，而同优先级情况下会优先选择已使用时间片较少的进程。

以此处为例

![image-20201214220154791](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214211840976.png)

在D进程运行过程中，D的时间片使用时间一直＜E，所以E没有机会运行，也就是`writeCount`一直为1。当D进程退出后，运行了`V(&readPermission);`，使得读者进程被允许执行。此时`readPermission.value`回到了0。

这时候E才到达，即使调度算法选择了E，由于`writeCount==0`，E会被卡在`P(&readPermission);`，从而B偷的时间读。

回到写者优先的定义上，

> 已有进程运行时，有写者和读者进入，即使读者晚于写者到来，也优先进行写操作

而这时候，在D进程时候，只有B到了，D进程结束后E才进入，所以并不矛盾！

#### 并发量1

![image-20201214220039204](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214220039204.png)

#### 并发量2

![image-20201214220041404](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214220041404.png)

#### 并发量3

![image-20201214220043389](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214220043389.png)

## 5 解决饥饿问题

进程结构体引入变量`isDone`，所有进程都运行过一遍后，`isDone`重置，以白色输出`<RESTART>`

在`main.c`中修改此行可对解决饥饿进行开关

```c
// 是否解决饿死
solveHunger = 0;
```

解决饥饿后的并发量为3读者优先：

![image-20201214222203902](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201214222203902.png)