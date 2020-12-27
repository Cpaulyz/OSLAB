
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void A();
void B();
void C();
void D();
void E();
void F();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);


/* 以下是系统调用相关 */

/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
PUBLIC  void     sys_myprint(char* s);
// PUBLIC  void     sys_myprint_color(char* s,int color);
PUBLIC  void     sys_sleep(int milli_seconds);
PUBLIC  void     sys_p(void* mutex);
PUBLIC  void     sys_v(void* mutex);

/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();
PUBLIC  void    myprint(char* s);  /*封装打印的系统调用*/ 
// PUBLIC  void    myprint_color(char* s,int color);  /*封装打印的系统调用*/ 
PUBLIC  void    mysleep(int milli_seconds);  /*封装睡眠*/ 
PUBLIC  void    P(void* mutex);  
PUBLIC  void    V(void* mutex);   

