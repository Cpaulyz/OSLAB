
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PRIVATE void push_pos(CONSOLE* p_con,int pos);
PRIVATE int pop_pos(CONSOLE* p_con);
PUBLIC void exit_esc(CONSOLE* p_con);
PUBLIC void search(CONSOLE *p_con);
PRIVATE void redo(CONSOLE *p_con);
PUBLIC void push_out_char(CONSOLE* p_con,char ch);
PUBLIC void push_out_char(CONSOLE* p_con,char ch){
	// 新增，压入操作队列
	p_con->out_char_stack.ch[p_con->out_char_stack.ptr++]=ch;
}
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

/*======================================================================*
		新增方法，退出ESC模式
 *======================================================================*/
PUBLIC void exit_esc(CONSOLE* p_con){
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	// 清屏，用空格填充
	int i;
	// 删除ESC开始后的红色输入
	for(i=0;i<p_con->cursor-p_con->search_start_pos;++i){ 
		*(p_vmem-2-2*i) = ' ';
		*(p_vmem-1-2*i) = DEFAULT_CHAR_COLOR;
	}
	// 把ESC前的普通输入还原为白色
	for(i=0;i<p_con->search_start_pos*2;i+=2){ 
		*(u8*)(V_MEM_BASE + i + 1) = DEFAULT_CHAR_COLOR;
	}
	// 复位指针
	p_con->cursor = p_con->search_start_pos;
	p_con->pos_stack.ptr = p_con->pos_stack.search_start_ptr;
	p_con->out_char_stack.ptr = p_con->out_char_stack.search_start_ptr;
	flush(p_con); // 更新p_con，这个不能漏
}
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

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
	// 初始化pos_stack的ptr指针
	p_tty->p_console->pos_stack.ptr = 0;
	// 初始化out_char_stack
	p_tty->p_console->out_char_stack.ptr = 0;


	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

	switch(ch) {
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
		}
		break;
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
	case 'z':
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
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

