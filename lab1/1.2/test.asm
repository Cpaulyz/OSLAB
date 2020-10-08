	org	07c00h			; 告诉编译器程序加载到7c00处
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	xor cx,cx              ;设置堆栈段的段基地址
    mov ss,cx
    mov sp,cx
   ; call DispStr
;	jmp	ReadCharAndDisp			; 调用显示字符串例程
;	jmp	$			; 无限循环

	mov	bx, BootMessage
	mov	cx, EndMessage - BootMessage			; CX = 串长度
	call DispStr

ReadCharAndDisp:
		call input_x
	ReadChar:
		mov ah,0x00				;0功能号对应从键盘读字符
		int 16h					;键盘服务, int 0x16
	                    		; 中断返回时，字符的ASCII在AL中
		mov ah,0x0e
		int 0x10				;中断0x10的0x0e号功能将al中的字符输出到屏幕


		cmp al,' '				; 如果是空格，说明分割，切换到输入y
		jz SaveX  				; 存入X中
		cmp al,0x0d
		jz SaveY

		push ax
		mov si,[di]
		inc si
		mov [di],si  			; inc len

		jmp ReadChar		; 无条件跳转到readChar标签

	SaveX:
		mov bx,number_x
		mov di,0
		mov si,[len_x]
	save_x_to_mem:
		pop ax
		sub al,0x30
		mov [bx+di],al
		inc di
		cmp di,si
		jnz save_x_to_mem
		call input_y
		jmp ReadChar

	SaveY:
		mov bx,number_y
		mov di,0
		mov si,[len_y]
	save_y_to_mem:
		pop ax
		sub al,0x30
		mov [bx+di],al
		inc di
		cmp di,si
		jnz save_y_to_mem
		jmp END

	input_x:
		mov di,len_x
		mov bx,number_x
		ret 
	input_y:
		mov di,len_y
		mov bx,number_y
		ret

END:
	call printx
	call printy
	call Add_
	mov	bx, EndMessage
	mov	cx, msg_end - EndMessage			; CX = 串长度
	call DispStr
	mov byte [len_x],0
	mov byte [len_y],0
	mov byte [len_add],0
	jmp ReadCharAndDisp

Add_:
		mov ax,[len_x]
		mov bx,[len_y]
		mov [max_len],ax
		cmp ax,bx
		jge start_add
		mov [max_len],bx
	start_add:
		mov si,0
	AddLoop:
		mov bx,number_x
		mov al,[bx+si] ; x
		mov bx,number_y
		mov ah,[bx+si] ; y

		add al,ah
		add al,[Carry]
		mov byte [Carry],0 ; set carry = 0
		cmp al,10
		jl SaveAdd
		; if greater then take carry
		sub al,10
		mov byte [Carry],1
	SaveAdd:	
		mov bx,number_add
		mov [bx+si],al
		inc si
		cmp si,[max_len]
		jl AddLoop
		; 最后检查carry位还有没有
		mov ah,[Carry] ; ah没用了，借来存一下Carry位
		cmp ah,1
		jnz end_add
		mov byte [bx+si],0x01
		inc si
	end_add:
		mov [len_add],si ;把结果的长度存起来，打印用
		jmp printAdd
		ret


Print:
	newline:
		;换行
		mov ah,0x0e
		mov al,0x0d
	    int 0x10
		mov al,0x0a
	    int 0x10
	    ret 
	printx:
		call newline
		mov ah,0x0e
		mov cx,[len_x]
		mov bx,number_x
	printxchar:
		mov si,cx
	    mov al,[bx+si-1]
	    add al,0x30
	    int 0x10
	    loop printxchar
	    ret

	printy:
		call newline
		mov ah,0x0e
		mov cx,[len_y]
		mov bx,number_y
	printychar:
		mov si,cx
	    mov al,[bx+si-1]
	    add al,0x30
	    int 0x10
	    loop printychar
	    ret 
	printAdd:
		call newline
		mov ah,0x0e
		mov cx,[len_add]
		mov bx,number_add
	printAddchar:
		mov si,cx
	    mov al,[bx+si-1]
	    add al,0x30
	    int 0x10
	    loop printAddchar
	   	;-------------temp check lenx-------------
	    mov al,' '
	    int 0x10
	    mov ax,[len_add]
	    mov ah,0x0e
	    add al,0x30
	    int 0x10
	    ;-----------------------
	    ret 

DispStr:
    mov ah,0x0e
    mov al,[bx]
    int 0x10
    inc bx
    loop DispStr
    ret 

BootMessage:		db	'OS_LAB_1 sample big number add & mul',0x0d,0x0a
					db  'Please input x and y:',0x0d,0x0a
EndMessage:			db	0x0d,0x0a,'Input end..',0x0d,0x0a
msg_end:


number_x:		times	22	db 0
number_y:		times 	22 	db 0
number_add:		times	22	db 0
len_x:						dw 0
len_y:						dw 0
len_add:					dw 0
max_len:					dw 0
Carry:						db 0 ;进位符
neg_x:						db 0
neg_y:						db 0x1

times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55					; 结束标志