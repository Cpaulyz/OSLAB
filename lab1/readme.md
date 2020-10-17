# Hello OS

安装nasm

安装bochs

```shell
###安装步骤:
 
##本人使用环境为
 
VMware虚拟机CentOs7
 
 
##下载源码及依赖:
 
sudo yum install gtk2 gtk2-devel
 
sudo yum install libXt libXt-devel
 
sudo yum install libXpm libXpm-devel
 
sudo yum install SDL SDL-devel
 
sudo yum install libXrandr-devel.x86_64
 
sudo yum install xorg-x11-server-devel
 
 
 
##源码配置和安装
 
获取和解压
 
源码地址：https://sourceforge.net/projects/bochs/files/bochs/2.6.9/
 
wget  https://sourceforge.net/projects/bochs/files/bochs/2.6.9/bochs-2.6.9.tar.gz
 
tar -xvfz bochs-2.6.9.tar.gz
 
 
进入加压目录
cd  bochs-2.6.9
 
创建编译执行目录:
mkdir  build  && cd  build 
 
##安装配置
 
../configure --prefix=/usr/local/myinstall/boch-2.6.9  --enable-ne2000 --enable-all-optimizations --enable-cpu-level=6 --enable-x86-64 --enable-vmx=2 --enable-pci --enable-usb --enable-usb-ohci --enable-e1000 --enable-debugger --enable-debugger-gui --enable-disasm --with-sdl --with-x11
 
 
## 编译
 
$make
 
## 安装
 
$make install
```



https://blog.csdn.net/muzi_since/article/details/102559187

编写boot.bin

```assembly
	org	07c00h			; 告诉编译器程序加载到7c00处
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	call	DispStr			; 调用显示字符串例程
	jmp	$			; 无限循环
DispStr:
	mov	ax, BootMessage
	mov	bp, ax			; ES:BP = 串地址
	mov	cx, 16			; CX = 串长度
	mov	ax, 01301h		; AH = 13,  AL = 01h
	mov	bx, 000ch		; 页号为0(BH = 0) 黑底红字(BL = 0Ch,高亮)
	mov	dl, 0
	int	10h			; 10h 号中断
	ret
BootMessage:		db	"Hello, OS world!"
times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55				; 结束标志
```



```
nasm boot.asm -o boot.bin
dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
```



编写.bochsrc

```properties
# how much memory the emulated machine will have
megs: 32

# filename of ROM images
# BIOS已经修改，地址可以不加，可以根据文件大小进行推断，如里加地址要与文件大小相匹配
romimage: file=$BXSHARE/BIOS-bochs-latest
vgaromimage: file=$BXSHARE/VGABIOS-lgpl-latest

# what disk images will be used 
# 配置镜像名
floppya: 1_44=a.img, status=inserted

# choose the boot disk.
boot: a

# where do we send log messages?
log: bochsout.txt

# disable the mouse, since Finix is text only
mouse: enabled=0

# enable key mapping, using US layout as default.
# 键盘的映射方式也改变了
keyboard: keymap=$BXSHARE/keymaps/x11-pc-us.map
```



运行

输入6 c

![image-20201006212118414](https://img2020.cnblogs.com/blog/1958143/202010/1958143-20201010001819257-2036464664.png)

![image-20201006212129929](https://img2020.cnblogs.com/blog/1958143/202010/1958143-20201010001818903-573321368.png)



# 大整数运算

## 编译运行

```
nasm -f elf32 calculator.asm 
ld -m elf_i386 calculator.o -o calculator 
./calculator
```

## 运行截图

![image-20201010001737115](https://img2020.cnblogs.com/blog/1958143/202010/1958143-20201010001818469-1372832431.png)

# Note

### 主引导扇区

0面0道1扇区 512字节

1. ROM-BIOS将他加载到逻辑地址0x0000:0x7c00处
2. 判断是否有效（最后两个字节是0x55和0xAA）
3. 如果有效，jmp 0x0000:0x7c00继续执行

### 显卡

* 图形模式
* 文本模式

不同的工作模式下，对显存内容的解释不同



# 指令

### loop

* CX内容-1
* 如果CX不为0，跳转，否则顺序执行
* 后面跟的是偏移量（编译阶段用digit的地址-loop所在的地址）

```assembly
digit: 
         xor dx,dx
         div si
         mov [bx],dl                   ;保存数位
         inc bx 
         loop digit
```

### div

**第一种**

16位除8位

被除数在AX种，商在AL，余数在AH中

除数由8位通用寄存器或者内存提供

```assembly
div cl
div byte [0x0023]
```

**第二种**

32位除16位

被除数高16位在DX，低16位在AX

商在AX，余数在DX

```assembly
         mov ax,number                 ;取得标号number的偏移地址
         mov bx,10

         ;设置数据段的基地址
         mov cx,cs
         mov ds,cx

         ;求个位上的数字
         mov dx,0
         div bx
         mov [0x7c00+number+0x00],dl   ;保存个位上的数字
```

### jns

如果未设置符号位SF(sign flag)，就跳转

> 很多运算会改变SF，比如dec，如果结果最高位是0，则SF=0；如果结果最高位是1，则SF=1

```assembly
         ;显示各个数位
         mov bx,number 
         mov si,4                      
   show:
         mov al,[bx+si]
         add al,0x30
         mov ah,0x04
         mov [es:di],ax
         add di,2
         dec si
         jns show
```

### pop&push

8086只能压入一个字

```assembly
push ax ;true
push word [label_a] ;true

push al ;false
push byte [label_a] ;false
```

SS:栈基址

SP:栈指针

* push： 先把SP减去字长（16位处理器上是2），然后存
* pop：根据SS:SP，先取，然后增长SP

# 寻址

> 操作的数值从哪去，送到哪里去，叫做寻址

## 寄存器寻址

操作数就在寄存器中

```assembly
mov ax,cx
add bx,0xf00
inc dx
```

## 立即寻址

指令的操作数是一个立即数

```assembly
add bx,0xf000
mov dx,label_a
```

## 内存寻址

实际上是寻找偏移地址

### 直接寻址

```assembly
mov ax,[0x5c0f]
add word [0x0230],0x5000
xor byte [es:label_b],0x05
```

第一、二条，翻译为[ds:5c05]、[ds:0x0230]

第三条只是用了段超越前缀，也就是改变了段地址，但还是直接寻址

### 基址寻址

地址部分使用BX或者BP来提高偏移地址

* [BX] → [DS:BX]
* [BP] → [SS:BP]

```assembly
mov [bx],dx
mov ax,[bp]
```

允许使用一个偏移量

```assembly
mov dx,[bp-2]
```

### 变址寻址

类似基址寻址，但使用的寄存器是SI、DI

除非是用段超越前缀，否则都是用ds来进行偏移

允许使用一个偏移量

```assembly
mov [si],dx
mov ax,[di]
and byte [di+label_a],0x80
mov [si+0x100],al
```

### 基址变址寻址

使用一个基址寄存器（bx、bp），外加一个变址寄存器（si、di）

```assembly
mov ax,[bx+si]
add word [bx+di],0x3000
```

> 另外一种分类方式（问题清单里的）
>
> https://blog.csdn.net/u012206617/article/details/86554855





# checklist

### 1.请简述 80x86 系列的发展历史

x86泛指一系列基于[Intel 8086](https://baike.baidu.com/item/Intel 8086)且向后兼容的[中央处理器](https://baike.baidu.com/item/中央处理器)[指令集架构](https://baike.baidu.com/item/指令集架构)。最早的8086处理器于1978年由[Intel](https://baike.baidu.com/item/Intel)推出，为16位[微处理器](https://baike.baidu.com/item/微处理器)。

Intel在早期以80x86这样的数字格式来命名处理器，包括[Intel 8086](https://baike.baidu.com/item/Intel 8086)、80186、[80286](https://baike.baidu.com/item/80286)、[80386](https://baike.baidu.com/item/80386)以及[80486](https://baike.baidu.com/item/80486)，由于以“86”作为结尾，因此其架构被称为“x86”。

* 16位处理器
	* 最开始时8086处理器是16位结构，数据总线16位，地址总线20位，主存容量1MB，主要的功能有一些指令系统，其后的8088在指令系统的基础上增加了一些实用指令，涉及到堆栈操作，输入输出指令，移位指令，乘法指令，支持高级语言的指令。
	* 80286处理器仍是16位结构，但是地址总线扩展为24位，也就是贮存能有16MB容量，在实方式下相当于快速8086，但是在相对于8086新增的保护方式中，能提供存储管理，保护机制，多任务管理的硬件支持。
* IA32处理器（32位80x86处理器）
	* 80386处理器采用32位结构，数据总线32位，地址总线32位，可寻址达到4GB，其兼容16位的80286处理器，但是也新增了位操作，条件设置指令。同时还提供了虚拟8086工作方式（一种类似于8086工作方式但是又接受保护方式的管理，早期的32位PC系统采用保护方式，其MS-DOS命令行就是一种虚拟8086工作方式）。
	* 80486处理器，相当于80386+80387（数学协处理器）+8KB Cache，

### 2.说明小端和大端的区别，并说明 80x86 系列采用了哪种方式？

* 1)大端模式：Big-Endian就是高位字节排放在内存的低地址端，低位字节排放在内存的高地址端。其实大端模式才是我们直观上认为的模式，和字符串存储的模式差类似）

	低地址 --------------------> 高地址
	0x12  |  0x34  |  0x56  |  0x78

* 2)小端模式：Little-Endian就是低位字节排放在内存的低地址端，高位字节排放在内存的高地址端。

	低地址 --------------------> 高地址
	0x78  |  0x56  |  0x34  |  0x12

* 80x86采用小端

### 3.8086 有哪五类寄存器，请分别举例说明其作用？

![](https://img-blog.csdnimg.cn/20191012194138924.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQxMTQ2NjUw,size_16,color_FFFFFF,t_70)

### 4.什么是寻址？立即寻址和直接寻址的区别是什么？

* 操作的数值从哪去，送到哪里去，叫做寻址

* 立即寻址

	* 指令的操作数是一个立即数

		```assembly
		add bx,0xf000
		mov dx,label_a
		```

* 直接寻址

	* 
		```assembly
		mov ax,[0x5c0f]
		add word [0x0230],0x5000
		xor byte [es:label_b],0x05
		```

		第一、二条，翻译为[ds:5c05]、[ds:0x0230]

		第三条只是用了段超越前缀，也就是改变了段地址，但还是直接寻址

### 5.请举例说明寄存器间接寻址、寄存器相对寻址、基址加变址寻址、相对基址加变址寻址四种方式的区别

* 直接寻址，偏移地址值直接出现在执行代码中

	```assembly
	mov 寄存器，[偏移地址]
	mov [偏移地址]，寄存器
	```

* 寄存器间接寻址，偏移地址通过寄存器取得

	```assembly
	mov 寄存器，[寄存器]
	mov [寄存器]，寄存器
	```

* 寄存器相对寻址，偏移地址值通过[寄存器+偏移量值]的形式运算后获得

	```assembly
	mov 寄存器,[寄存器+偏移量值]
	mov 寄存器,ds:[寄存器+偏移量值]
	mov [寄存器+偏移量值]，寄存器
	mov ds:[寄存器+偏移量值]，寄存器
	```

	![image-20201017184431823](C:\Users\admin\AppData\Roaming\Typora\typora-user-images\image-20201017184431823.png)

* 基址加变址寻址，偏移地址值通过[基址寄存器+变址存储器]的形式运算后获得

	```assembly
	mov ax,[bx+si]
	add word [bx+di],0x3000
	```

* 相对基址加编址寻址

	```assembly
	mov 寄存器,[基址寄存器+变址寄存器+偏移量值]
	mov [基址寄存器+变址寄存器+偏移量值],寄存器
	```

### 6. 请分别简述 MOV 指令和 LEA 指令的用法和作用？

lea是“load effective address”的缩写，简单的说，lea指令可以用来将一个内存地址直接赋给目的操作数，例如：
lea eax,[ebx+8]就是将ebx+8这个值直接赋给eax，而不是把ebx+8处的内存地址里的数据赋给eax。

而mov指令则恰恰相反，例如：
mov eax,[ebx+8]则是把内存地址为ebx+8处的数据赋给eax。

> 如LEA EAX, [ EBX + ECX ]，它相当于计算EBX和ECX的值，将这个值保存到EAX寄存器中。原因：由于EBX+ECX计算出来的值是该内存地址，而通过[EBX+ECX]得到的是内存地址保存的值，而LEA命令是加载该值的有效地址并且保存到目标寄存器中，也就是将EBX+ECX的值保存到EAX寄存器中
>  `由于加载的是有效地址，而不是实际地址，所以EAX中保存的是EBX+ECX，而不是ds:EBX+ECX`

https://www.jianshu.com/p/01e8d5ef369f

### 7. 请说出主程序与子程序之间至少三种参数传递方式

* 寄存器法
	* 主程序中把要传递的参数放在某一指定的寄存器中，然后从子程序中取出指定的寄存器参数
* 约定单元法
* 堆栈法

### 8. 如何处理输入和输出，代码中哪里体现出来？

输入

```assembly
 MOV  AH, 1
 INT  21H
 ;存在AL中
```

1.了解**INT 21H**的**09H**号中断调用（输出字符串）

① **LEA DX，字符串的开头**    或    **MOV DX，OFFSET字符串的开头** 

② **MOV AH，09H**

③ **INT 21H**

2.在定义字符串的时候要在末尾加上**'$'**作为字符串的结束标志。

3.了解**INT 21H**的**0AH**号中断调用（输入字符串）

① **LEA DX，字符串的开头**    或    **MOV DX，OFFSET字符串的开头** 

② **MOV AH，0AH**

③ **INT 21H**

或者使用10h中断

https://blog.csdn.net/hua19880705/article/details/8125706

### 9. 有哪些段寄存器

![image-20201017204652050](C:\Users\admin\AppData\Roaming\Typora\typora-user-images\image-20201017204652050.png)

### 10. 通过什么寄存器保存前一次的运算结果，在代码中哪里体现出来。

一般是目的寄存器

也有特殊，比如除法

> 无符号除法指令DIV(DIVision) DIV OPRD  ;
>
> 除数OPRD决定是8位除法还是16位除法;
>
> OPRD=8位,则被除数默认在AX中,AX除以OPRD的商保存在AL中,余数保存在AH中;
>
> OPRD=16位,则被除数默认在DX与AX中,结果的商保存在AX中,余数保存到DX中

### 11. 解释 boot.asm 文件中，org 0700h 的作用

### 12. boot.bin 应该放在软盘的哪一个扇区？为什么？

0面0道1扇区 512字节

1. ROM-BIOS完成之前，最后一件事是从外存储设备读取更多的指令交给处理器执行
2. 如果计算机设置从硬盘启动，ROM-BIOS会读取主引导扇区的内容，将他加载到逻辑地址0x0000:0x7c00处
3. 判断是否有效（最后两个字节是0x55和0xAA）
4. 如果有效，jmp 0x0000:0x7c00继续执行

### \13. loader 的作用有哪些？

### \14. 解释 NASM 语言中 [ ] 的作用

### 15. 解释语句 `times 510-($-$$) db 0，为什么是 510?` `$` 和 `$$` 分别表示什么？

* 一个有效的主引导扇区，最后两个字节应当是0x55和0xAA，所以是510，最后两个字节填充`dw 0xaa55`
* `$`当前行的地址
* `$$`当前节（section）的开始处的地址

### 16. 解释配置文件 bochsrc 文件中各参数的含义

```properties
# 第一步，首先设置Bochs在运行过程中能够使用的内存，本例为32MB。
# 关键字为：megs 单位为MB
megs: 32

# 设置Bochs所使用的磁盘，软盘的关键字为floppy。
# 若只有一个软盘，则使用floppya即可，若有多个，则为floppya，floppyb...
# 这是用来设置软盘的相关属性。类型为1.44M容量的软盘， 镜像文件名为a.img
floppya: 1_44=a.img, status=inserted

# 选择启动盘符。
boot: floppy

# 不知道？？？？
display_library: sdl

```

