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