base：原书代码

hw：作业代码



# 1 一些笔记

> 在当前目录(.)下创建一个新的软盘镜像a.img
>
> ` mkfs.fat -F 12 -C a.img 1440 `

## bootloader使用

![image-20201117212608456](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201117220539784.png)

## 加入内核

![image-20201117212714573](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201117212714573.png)

![image-20201117220539784](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201117212608456.png)

改用

`ld -m elf_i386 -s -o kernel.bin kernel.o`

## Makefile

```makefile
#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x30400

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -f elf
CFLAGS		= -I include/ -c -fno-builtin
LDFLAGS		= -s -Ttext $(ENTRYPOINT)
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
ORANGESBOOT	= boot/boot.bin boot/loader.bin
ORANGESKERNEL	= kernel.bin
OBJS		= kernel/kernel.o kernel/start.o lib/kliba.o lib/string.o
DASMOUTPUT	= kernel.bin.asm

# All Phony Targets
.PHONY : everything final image clean realclean disasm all buildimg

# Default starting position
everything : $(ORANGESBOOT) $(ORANGESKERNEL)

all : realclean everything

final : all clean

image : final buildimg

clean :
	rm -f $(OBJS)

realclean :
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL)

disasm :
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

# We assume that "a.img" exists in current folder
buildimg :
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc \
			boot/include/fat12hdr.inc boot/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(ORANGESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(ORANGESKERNEL) $(OBJS)

kernel/kernel.o : kernel/kernel.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o : kernel/start.c include/type.h include/const.h include/protect.h
	$(CC) $(CFLAGS) -o $@ $<

lib/kliba.o : lib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o : lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
```

* `=`定义变量

* `${XXX}`使用变量

* 标准语法

	```makefile
	target: prerequsites
			command
	```

	![image-20201117222459928](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201117222459928.png)

* `$@`代表target

* `$<`代表prerequisites的第一个名字

### 踩坑

* ld参数要加上 `-m elf_i386`
* gcc参数要加上`-m32`

## code

![image-20201118102113478](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201122002323760.png)

![image-20201118102127879](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201118102113478.png)

* break code是make code与0x80进行or操作得到的结果









# 2 实现

## 2.1 base

将orange书中的`chapter7/n`复制过来，跑一下

> 注意修改makefile里的坑

发现已经实现了以下的基本功能

* ⽀持回车键换⾏。
* 支持空格键，不支持tab。
* ⽀持⽤退格键删除输⼊内容。
* 可以输⼊并显⽰ a-z,A-Z 和 0-9 字符。

基本框架有了，就在其基础上做实现就可以，一个个来

## 2.2 清屏

目的：**从从屏幕左上⻆开始，以白色显示键盘输入的字符。**

实现：

* 在初始化之前将整个屏幕打印满空格，然后把指针移到最前面

在`main.c`中加入以下方法

```c
// 清屏，将显存指针disp_pos指向第一个位置
void cleanScreen(){
	disp_pos = 0;
	int i;
	for (i = 0 ; i < SCREEN_SIZE; ++i){
		disp_str(" ");
	}
	disp_pos = 0;
}
```

## 2.3 tab支持

### 2.3.1 支持输入

* 在`tty.c`的``in_process`方法中加入TAB判断

	```c
	...
			case TAB:
				put_key(p_tty, '\t');
				break;
	...
	```

* 在`console.h`中定义TAB的宽度

	```c
	#define TAB_WIDTH 			4
	```

* 在输出的时候判断`\t`，对`console.c`的`out_char()`方法进行修改

	```c
		case '\t': // TAB输出，将cursor往后移动TAB_WIDTH
			if(p_con->cursor < p_con->original_addr +
			    p_con->v_mem_limit - TAB_WIDTH){
				int i;
				for(i=0;i<TAB_WIDTH;++i){ // 用空格填充
					*p_vmem++ = ' ';
					*p_vmem++ = DEFAULT_CHAR_COLOR;
				}
				push_pos(p_con,p_con->cursor);
				p_con->cursor += TAB_WIDTH; // 调整光标
			}
			break;
	```

至此已经可以实现TAB的输入了，但是关于删除还没解决，这时候删除一次还只能删除一个空格，TAB要删除四次

### 2.3.2 支持删除

在用书上代码的时候，发现除了TAB退格有问题，换行的退格也有问题

按照源代码，换行后退格，会退到**上一行的最后位置**

![image-20201120214713096](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201122150357777.png)

这显然不是之前的位置，所以很直观的想法就是**在打印之前，拿一个变量来存储前一个位置**

但是存一个位置显然是不够的，得存每个走过的位置，所以需要**拿一个栈来存**

* `console.h`

	```c
	/*新增，记录光标曾在位置*/
	typedef struct cursor_pos_stack 
	{
		int *ptr;
		int pos[SCREEN_SIZE];
	}POSSTACK;
	
	/* CONSOLE */
	typedef struct s_console
	{
		unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
		unsigned int	original_addr;		/* 当前控制台对应显存位置 */
		unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
		unsigned int	cursor;			/* 当前光标位置 */
		POSSTACK pos_stack;	/*新增*/
	}CONSOLE;
	```

* 初始化时候，在`console.c/init_screen`中新增

	```c
		// 初始化pos_stack的ptr指针
		p_tty->p_console->pos_stack.ptr = p_tty->p_console->pos_stack.ptr;
	```

* `console.h`新增两个方法

	```c
	PRIVATE void push_pos(CONSOLE* p_con,int pos);
	PRIVATE int pop_pos(CONSOLE* p_con);
	/*======================================================================*
			新增方法，用于记录/获取指针所处位置
	 *======================================================================*/
	PRIVATE void push_pos(CONSOLE* p_con,int pos){
		p_con->pos_stack.pos[p_con->pos_stack.ptr++] = pos;
	}
	PRIVATE int pop_pos(CONSOLE* p_con){
		if(p_con->pos_stack.ptr==0){
			return 0; // 不会发生这种情况
		}else{
			--p_con->pos_stack.ptr;
			return p_con->pos_stack.pos[p_con->pos_stack.ptr];
		}
	}
	```

* 修改输出方法

	主要做的事情是：在cursor移动前`push_pos`，在退格前`pop_pos`来获取之前的位置

	```c
	case '\n':
			if (p_con->cursor < p_con->original_addr +
			    p_con->v_mem_limit - SCREEN_WIDTH) {
				push_pos(p_con,p_con->cursor);
				p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
					((p_con->cursor - p_con->original_addr) /
					 SCREEN_WIDTH + 1);
			}
			break;
		case '\b':
			if (p_con->cursor > p_con->original_addr) {
				// p_con->cursor--;
				int temp = p_con->cursor; // 原先位置
				p_con->cursor = pop_pos(p_con);
				int i;
				for(i=0;i<temp-p_con->cursor;++i){ // 用空格填充
					*(p_vmem-2-2*i) = ' ';
					*(p_vmem-1-2*i) = DEFAULT_CHAR_COLOR;
				}
				// *(p_vmem-2) = ' ';
				// *(p_vmem-1) = DEFAULT_CHAR_COLOR;
			}
			break;
		case '\t': // TAB输出，将cursor往后移动TAB_WIDTH
			if(p_con->cursor < p_con->original_addr +
			    p_con->v_mem_limit - TAB_WIDTH){
				push_pos(p_con,p_con->cursor);
				p_con->cursor += TAB_WIDTH; // 直接调整光标即可
			}
			break;
		default:
			if (p_con->cursor <
			    p_con->original_addr + p_con->v_mem_limit - 1) {
				*p_vmem++ = ch;
				*p_vmem++ = DEFAULT_CHAR_COLOR;
				push_pos(p_con,p_con->cursor);
				p_con->cursor++;
			}
			break;
	```

## 2.4 shift组合键

发现在`keyboard.c`里，orange帮我们实现好了，那没事了。

## 2.5 清屏

目的：每隔20s清屏

实现：很直接的想法，在定时进程TestA里进行修改

```c
void TestA()
{
	int i = 0;
	while (1) {
		// disp_str("A."); 
		cleanScreen();
		milli_delay(100000); //经过尝试发现设置为 100000 差不多是20秒左右
	}
}
```

发现是可以的，但是光标不会复位，所以需要重新初始化screen

在`tty.c`中添加方法

```c
// 新增，初始化所有tty的screen
PUBLIC void init_all_screen(){
	TTY *p_tty;
	for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
	{
		init_screen(p_tty);
	}
	select_console(0);
}
```

修改后的TestA为：

```c
void TestA()
{
	int i = 0;
	while (1) {
		// disp_str("A."); 
		cleanScreen();
		init_all_screen();
		milli_delay(100000); //经过尝试发现设置为 100000 差不多是20秒左右
	}
}
```

运行完发现报错了

![image-20201122002323760](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201122134918046.png)

查了各种资料，发现大概是因为用户进程不能使用tty里的方法？

尝试将TestA的进程从PROCS转为TASKS

```c
// global.c
PUBLIC	TASK	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "tty"},
	{TestA, STACK_SIZE_TESTA, "TestA"}}; // 新增，改为TASKS

PUBLIC  TASK    user_proc_table[NR_PROCS] = {
	// {TestA, STACK_SIZE_TESTA, "TestA"},
	{TestB, STACK_SIZE_TESTB, "TestB"},
	{TestC, STACK_SIZE_TESTC, "TestC"}};

//proc.h
/* Number of tasks & procs */
// #define NR_TASKS	1
// #define NR_PROCS	3
#define NR_TASKS	2
#define NR_PROCS	2
```

至此，完成清屏，但是后面还需要进行修改。

## 2.6 ESC

### 2.6.1 切换模式/输出红色

需要记录模式状态，加入代码

```c
//global.c
PUBLIC int mode;  
/*
0：正常模式
1：搜索模式
*/

//global.h
extern int mode;//标识模式
```

响应的，只有mode==0的时候可以清屏，所以需要修改

```c
void TestA()
{
	int i = 0;
	while (1) {
		if(mode==0){
			// disp_str("A."); 
			cleanScreen();
			init_all_screen();
			milli_delay(100000); //经过尝试发现设置为 100000 差不多是20秒左右
		}else{
			milli_delay(10);
		}
	}
}
```

在`tty.c`中加入代码

```c
PUBLIC void in_process(TTY *p_tty, u32 key)
{		...
    	case ESC:
		// ESC 切换模式
			if(mode==0){
				mode = 1;
			}else if(mode==1){
				mode = 0;
				// TODO：清除内容
			}
 			break;
 		...
}
```

修改`console.c`中

```c
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	...
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
			if(mode==0){
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}else{
				*p_vmem++ = RED; //输出红色
			}
			push_pos(p_con,p_con->cursor);
			p_con->cursor++;
		}
		break;
	}
	...
}
```

![image-20201122134918046](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201120214713096.png)

完成

### 2.6.2 搜索输入栈

搜索模式下输入需要做的事情有：

* 记录输入的字符（其实不用，因为从显存里就可以拿到）
* 记录输入开始的位置（以便退出ESC的时候清空输入）

修改`console.h`，加入搜索输入栈、并在CONSOLE和POSSTACK中开辟变量，记录ESC开始位置

```c
/*新增，记录光标曾在位置*/
typedef struct cursor_pos_stack 
{
	int ptr; //offset
	int pos[SCREEN_SIZE];
	int search_start_ptr;/*新增：ESC模式开始时候的ptr位置*/
}POSSTACK;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;				/* 当前光标位置 */
	unsigned int 	search_start_pos;	/*新增：ESC模式开始时候的cursor位置*/
	POSSTACK pos_stack;					/*新增*/	
}CONSOLE;
```

#### (1)实现退出时光标复位

在`console.c`中增加方法

```c
PUBLIC void exit_esc(CONSOLE* p_con);

/*======================================================================*
		新增方法，退出ESC模式
 *======================================================================*/
PUBLIC void exit_esc(CONSOLE* p_con){
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	// 清屏，用空格填充
	int i;
	for(i=0;i<p_con->cursor-p_con->search_start_pos;++i){ 
		*(p_vmem-2-2*i) = ' ';
		*(p_vmem-1-2*i) = DEFAULT_CHAR_COLOR;
	}
	// 复位指针
	p_con->cursor = p_con->search_start_pos;
	p_con->pos_stack.ptr = p_con->pos_stack.search_start_ptr;
	flush(p_con); // 更新p_con，这个不能漏
}
```

#### (2)搜索

`console.c`中，新增一个方法，直接暴力匹配了

```c
PUBLIC void search(CONSOLE *p_con);
/*======================================================================*
		新增方法，搜索，并将搜索结果标为红色
 *======================================================================*/
PUBLIC void search(CONSOLE *p_con){
	int i,j;
	int begin,end; // 滑动窗口
	for(i = 0; i < p_con->search_start_pos*2;i+=2){ // 遍历原始白色输入
		begin = end = i; // 初始化窗口为0
		int found = 1; // 是否匹配
		// 遍历匹配
		for(j = p_con->search_start_pos*2;j<p_con->cursor*2;j+=2){
			if(*((u8*)(V_MEM_BASE+end))==*((u8*)(V_MEM_BASE+j))){
				end+=2;
			}else{
				found = 0;
				break;
			}
		}
		// 如果找到，标红
		if(found == 1){
			for(j = begin;j<end;j+=2){
				*(u8*)(V_MEM_BASE + j + 1) = RED;
			}
		}
	}
}
```

在`tty.c`的`in_process()`中调用

```c
		case ENTER:
			if(mode==0){
				put_key(p_tty, '\n');
			}else if(mode==1){
				search(p_tty->p_console);
			}
			break;
```

![image-20201122150357777](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201118102127879.png)

完成了，但是还有以下问题没有解决：

* **退出的时候颜色变回白色**
* **按回车后，屏蔽除了ESC外的输出**

#### (3)退出的时候颜色变回白色

在(1)中的`exit_esc`代码中加上以下循环即可

```c
	for(i=0;i<p_con->search_start_pos*2;i+=2){ 
		*(u8*)(V_MEM_BASE + i + 1) = DEFAULT_CHAR_COLOR;
	}
```

#### (4)按回车后，屏蔽除了ESC外的输出

给mode加一个定义

```c
// global.c
PUBLIC int mode;  
/*
0：正常模式
1：搜索模式
2：ESC+ENTER
*/
```

在mode1下输入enter时切换到mode2

```c
		case ENTER:
			if(mode==0){
				put_key(p_tty, '\n');
			}else if(mode==1){
				search(p_tty->p_console);
				mode = 2; //切换模式，禁止输入
			}
			break;
```

在`tty.c`中的`in_process`进入处理前进行判断

```c
	// ESC+ENTER下不允许输入
	if (mode == 2)
	{
		if ((key & MASK_RAW) == ESC)
		{
			mode = 0;
			// 清除内容
			exit_esc(p_tty->p_console);
		}
		return; // 无论如何都返回，不进行后续处理了
	}
```

## 2.7 撤销

按下 control + z 组合键可以撤回操作（包含回车和 Tab 和删除），直到初始状态。

### 2.7.1 识别control+z

先不管撤销，先识别出来，然后打印一个*

```c
// global.h
extern int control; // 是否按下了control

// global.c
PUBLIC int control;
/*
0:没有按下
1:按下了，make
*/
```

在`keyboard.c`的`keyboard_read`中添加

```c
// 新增，识别是否按下了control，如果按下了则把control设为1，否则为0
control = ctrl_l||ctrl_r;
```

在`console.h`中，在`out_char()`方法中添加

```c
	case 'z':
	case 'Z':
		if(control){
			ch = '*';
		}
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
			if(mode==0){
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}else{
				*p_vmem++ = RED;//输出红色
			}
			push_pos(p_con,p_con->cursor);
			// push_search(p_con,ch);
			p_con->cursor++;
		}
		break;
```

![image-20201122160523586](https://cyzblog.oss-cn-beijing.aliyuncs.com/img/image-20201122160523586.png)

ok，control+z已经能够识别了，接下来做撤销操作。

### 2.7.2 撤销操作

> 可以撤回操作（包含回车和 Tab 和删除），直到初始状态。

说白了，就是要**记录每一步操作**

#### (1)mode0

在`console.h`中新增数据结构

```c
typedef struct out_char_stack 
{
	int ptr; //offset
	char ch[SCREEN_SIZE];
}OUTCHARSTACK;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;				/* 当前光标位置 */
	unsigned int 	search_start_pos;	/*新增：ESC模式开始时候的cursor位置*/
	POSSTACK pos_stack;					/*新增*/	
	OUTCHARSTACK out_char_stack;		/*新增*/
}CONSOLE;
```

在`tty.c`中，write的时候将out_char的ch压入操作记录栈

```c
PRIVATE void tty_do_write(TTY *p_tty)
{
	if (p_tty->inbuf_count)
	{
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES)
		{
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;
		push_out_char(p_tty->p_console,ch); // 新增，压入操作记录栈
		out_char(p_tty->p_console, ch);
	}
}

//console.c
PUBLIC void push_out_char(CONSOLE* p_con,char ch){
	// 新增，压入操作队列
	p_con->out_char_stack.ch[p_con->out_char_stack.ptr++]=ch;
}
```

判断control+z，进行撤销

```c
	case 'z':
	case 'Z':
		if(control){
			if(mode==0){
			// clean and init screen
			disp_pos = 0;
			int i;
			for (i = 0 ; i < SCREEN_SIZE; ++i){
				disp_str(" ");
			}
			disp_pos = 0;
			// 初始化pos_stack的ptr指针
			p_con->pos_stack.ptr = 0;
			p_con->cursor = disp_pos / 2;

			flush(p_con);
			redo(p_con);
			return; // 撤销操作后直接返回
			}
		}

		//TODO:现在只有在mode0下正常，mode1下不正常
```

具体撤销方法使用redo实现

```c
/*======================================================================*
		新增方法，redo以实现撤销操作，少做1步的操作
 *======================================================================*/
PRIVATE void redo(CONSOLE *p_con){
	p_con->out_char_stack.ptr-=2; // z也被压栈了，所以要-=2而不是1
	if(p_con->out_char_stack.ptr<=0){
		p_con->out_char_stack.ptr=0;
		return;		// 如果已经清空了，直接返回，不操作
	} 
	int i;
	for(i=0;i<p_con->out_char_stack.ptr;++i){
		out_char(p_con,p_con->out_char_stack.ch[i]);
		// out_char(p_con,'*');
	}
}
```

#### (2)mode1

加入数据结构`search_start_ptr`

```c
typedef struct out_char_stack 
{
	int ptr; //offset
	char ch[SCREEN_SIZE];
	int search_start_ptr;/*新增：ESC模式开始时候的ptr位置*/
}OUTCHARSTACK;
```

修改mode切换的出入口相应地方

```c
//tty.c in_process
// 记录ESC开始前的位置
p_tty->p_console->search_start_pos = p_tty->p_console->cursor;
p_tty->p_console->pos_stack.search_start_ptr = p_tty->p_console->pos_stack.ptr;
p_tty->p_console->out_char_stack.search_start_ptr = p_tty->p_console->out_char_stack.ptr;
		
//console.c exit_esc
// 复位指针
p_con->cursor = p_con->search_start_pos;
p_con->pos_stack.ptr = p_con->pos_stack.search_start_ptr;
p_con->out_char_stack.ptr = p_con->out_char_stack.search_start_ptr;
```

control+Z操作

```c
	case 'Z':
		if(control&&(mode==0||mode==1)){ // mode=0或1下才可以进行撤销，逻辑不一样，只能撤销当前模式下的输入 
			int temp; // 清屏开始的位置
			if(mode==0){
				temp = 0;	
				// 初始化pos_stack的ptr指针
				p_con->pos_stack.ptr = 0;
			}else if(mode==1){
				temp = p_con->search_start_pos*2;
				// 还原
				p_con->pos_stack.ptr = p_con->pos_stack.search_start_ptr;
			}
			// clean and init screen
			disp_pos = temp;
			int i;
			for (i = 0 ; i < SCREEN_SIZE; ++i){
				disp_str(" ");
			}
			disp_pos = temp;
			p_con->cursor = disp_pos / 2;
			flush(p_con);
			redo(p_con);
			return; // 撤销操作后直接返回
		}
```

其中的redo方法也有修改

```c
/*======================================================================*
		新增方法，redo以实现撤销操作，少做1步的操作
 *======================================================================*/
PRIVATE void redo(CONSOLE *p_con){
	int start;
	if(mode==0){
		start = 0;
	}else if(mode==1){
		start = p_con->out_char_stack.search_start_ptr;
	}
	p_con->out_char_stack.ptr-=2; // z也被压栈了，所以要-=2而不是1
	if(p_con->out_char_stack.ptr<=start){
		p_con->out_char_stack.ptr=start;
		return;		// 如果已经清空了，直接返回，不操作
	} 
	int i;
	for(i=start;i<p_con->out_char_stack.ptr;++i){
		out_char(p_con,p_con->out_char_stack.ch[i]);
	}
}
```

