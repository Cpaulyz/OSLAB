section .data
BootMessage:		db	'OS_LAB_1 sample big number add & mul',0x0d,0x0a
					db  'Please input x and y:',0x0d,0x0a
EndMessage:			db	0x0d,0x0a,'Please input x and y:',0x0d,0x0a
msg_end:

input:			times	128	db 0
number_x:		times	21	db 0
number_y:		times 	21 	db 0
number_add:		times	21	db 0
number_sub:		times	21	db 0
number_mul:		times	42	db 0
len_x:						dd 0
len_y:						dd 0
len_add:					dd 0
len_sub:					dd 5
len_mul:					dd 0
max_len:					dd 0
Carry:						db 0 ;进位符

neg_x:						db 0
neg_y:						db 0
neg_mul:					db 0
neg_add:					db 0
neg_sub:					db 0

bigNumAddr:					dd 0
smallNumAddr:				dd 0

tmp:						db 0 ;打印临时用
newLine:     				db  0Ah
minus:       				db  2DH
section .text
  global main
main: 
	mov	ecx, BootMessage ; 参数二：要显示的字符串
	mov	edx, EndMessage - BootMessage	; 参数三：字符串长度
	call DispStr

      ;接收输入
	mov eax,3
	mov ebx,0
	mov ecx,input
	mov edx,128
	int 80h

	call getXY
	; call PrintX
	; call PrintY
	; call AddXY
	call ADD_OR_SUB
	call MulXY
	; call SubXY
	; call PrintSub

	; 退出程序
    mov ebx, 0							; 参数一：退出代码
    mov eax, 1							; 系统调用号(sys_exit)
    int 0x80							; 调用内核功能


;=========================read X Y from input=======================
getXY:
	call getX
	call getY
	ret
	getX:
		mov ebx,input
		mov esi,0
		mov byte al,[ebx+esi]
	add_x_to_stack:
		push eax
		inc esi
		mov byte al,[ebx+esi]
		cmp al,' ' 						;遇到空格就停止
		jnz add_x_to_stack
		mov [len_x],esi					; save length
		mov ebx,number_x
		mov edi,0
	save_x_to_mem:
		pop eax
		cmp al,'-' 						; 如果读到符号，说明结束
		jz set_x_neg
		sub al,0x30
		mov [ebx+edi],al
		inc edi
		cmp di,si
		jnz save_x_to_mem
		ret
	set_x_neg:
		mov byte[neg_x],1
		dec si
		mov [len_x],si
		ret 

	getY:
		mov ebx,input+1
		mov esi,[len_x]
		add ebx,esi
		mov esi,0
		mov si,[neg_x]
		add ebx,esi						; 调整到' '后面开始读
		mov esi,0
		mov byte al,[ebx+esi]
	add_y_to_stack:
		push eax
		inc esi
		mov byte al,[ebx+esi]
		cmp al,0AH 						;遇到换行就停止
		jnz add_y_to_stack
		mov [len_y],esi					; save length
		mov ebx,number_y
		mov edi,0
	save_y_to_mem:
		pop eax
		cmp al,'-' 						; 如果读到符号，说明结束
		jz set_y_neg
		sub al,0x30
		mov [ebx+edi],al
		inc edi
		cmp di,si
		jnz save_y_to_mem
		ret
	set_y_neg:
		mov byte[neg_y],1
		dec si
		mov [len_y],si
		ret 
;=========================END READ XY ==============================

;=============================print=================================
 Print:
	DispStr:
	    mov ebx, 1    					; 参数一：文件描述符(stdout)
	    mov eax, 4						; 系统调用号(sys_write)
	    int 0x80						; 调用内核功能
	    ret

	PrintNewline:	
	;换行					
		mov ecx,newLine
		mov edx,1
		call DispStr
	    ret 
	PrintSign:			
	; 打印正负号，判断位存在al中，al=1的话打印一个'-'
		cmp al,1
		jnz print_sign_ret
		mov ecx,minus
		mov edx,1
		call DispStr
	print_sign_ret:
		ret

	PrintX:
		mov al,[neg_x]
		call PrintSign
		mov edx,1
		mov esi,[len_x]
	printxchar:
		mov byte cl,[esi+number_x-1]
		add cl,0x30
		mov byte[tmp],cl
		mov ecx,tmp
		call DispStr
		dec esi
		cmp esi,0
		jz endprintx
		jmp printxchar
	endprintx:
		mov edx,1
		mov byte[tmp],' '
		mov ecx,tmp
		call DispStr
		mov ecx,[len_x]
		add cl,0x30
		mov byte[tmp],cl
		mov ecx,tmp
		call DispStr
		call PrintNewline
		ret

	PrintY:
		mov al,[neg_y]
		call PrintSign
		mov edx,1
		mov esi,[len_y]
	printychar:
		mov byte cl,[esi+number_y-1]
		add cl,0x30
		mov byte[tmp],cl
		mov ecx,tmp
		call DispStr
		dec esi
		cmp esi,0
		jz endprinty
		jmp printychar
	endprinty:
		mov edx,1
		mov byte[tmp],' '
		mov ecx,tmp
		call DispStr
		mov ecx,[len_y]
		add cl,0x30
		mov byte[tmp],cl
		mov ecx,tmp
		call DispStr
		call PrintNewline
		ret

	PrintAdd:
		mov al,[neg_add]
		call PrintSign
		mov edx,1
		mov esi,[len_add]
	printAddchar:
		mov byte cl,[esi+number_add-1]
		add cl,0x30
		mov byte[tmp],cl
		mov ecx,tmp
		call DispStr
		dec esi
		cmp esi,0
		jz endprintadd
		jmp printAddchar
	endprintadd:
		call PrintNewline
		ret


	PrintMul:
		mov al,[neg_mul]
		call PrintSign
		mov edx,1
		mov esi,[len_mul]
	printMulchar:
		; mov ecx,[number_mul]
		mov ecx,[esi+number_mul-1]
		add cl,0x30
		mov byte[tmp],cl
		mov ecx,tmp
		call DispStr
		dec esi
		cmp esi,0
		jz endprintmul
		jmp printMulchar
	endprintmul:
		call PrintNewline
		ret

	PrintSub:
		; 减法的位数可能会减少，所以需要跳过前面的无效0
		mov al,[neg_sub]
		call PrintSign
		mov edx,1
		mov esi,[len_sub]
	skip_zero:
		cmp byte[esi+number_sub-1],0
		jnz printSubchar
		dec esi
		cmp esi,1
		jz printSubchar 					; 如果只剩一位了，无论如何都是要打印的
		jmp skip_zero
	printSubchar:
		; mov ecx,[number_mul]
		mov ecx,[esi+number_sub-1]
		add cl,0x30
		mov byte[tmp],cl
		mov ecx,tmp
		call DispStr
		dec esi
		cmp esi,0
		jz endprintsub
		jmp printSubchar
	endprintsub:
		call PrintNewline
		ret
;============================END PRINT============================

;===============================ADD===============================
AddXY:
	mov eax,[len_x]
	mov ebx,[len_y]
	mov [max_len],eax
	cmp eax,ebx
	jge start_add
	mov [max_len],ebx						; 记录最大长度
	start_add:
		mov esi,0
	AddLoop:
		mov ebx,number_x
		mov al,[ebx+esi] ; x
		mov ebx,number_y
		mov ah,[ebx+esi] ; y

		add al,ah
		add al,[Carry]
		mov byte [Carry],0 					; set carry = 0
		cmp al,10
		jl SaveAdd
		; if greater then take carry
		sub al,10
		mov byte [Carry],1
	SaveAdd:	
		mov ebx,number_add
		mov [ebx+esi],al
		inc esi
		cmp esi,[max_len]
		jl AddLoop
		; 最后检查carry位还有没有
		mov ah,[Carry] 						; ah没用了，借来存一下Carry位
		cmp ah,1
		jnz end_add
		mov byte [ebx+esi],0x01
		inc esi
	end_add:
		mov [len_add],esi 					;把结果的长度存起来，打印用
		; call PrintAdd
		ret
;===========================END ADD============================

;============================MUL===============================
MulXY:
	;反正下面都用的ax，不影响
	mov edi,0
	mov esi,0

	;预计算长度（mul_check中会进一步调整）
	mov eax,[len_x]
	add eax,[len_y]
	mov [len_mul],eax						; 先记长度为lenx+leny，最后再判断要不要扣掉最高一位

	;计算正负
	mov al,[neg_x]
	mov ah,[neg_y]
	xor al,ah
	cmp al,0
	jz xloop								; 如果x、y异号，把乘法结果符号设为1，也就是负
	mov byte[neg_mul],1
xloop:
	; AddLoop
	mov al,[number_x+esi] 					; X的位数
	mov ah,[number_y+edi]					; Y的位数
	mul ah 									; al*ah，结果存在ax
	add al,[number_mul+edi+esi]
	mov bl,10 	
	div bl									; 除10，AL存储除法操作的商，AH存储除法操作的余数
	add byte [number_mul+1+edi+esi],al
	mov byte [number_mul+edi+esi],ah

	; call PrintMul
	inc esi
	cmp esi,[len_x]
	jz next_yloop
	jmp xloop
next_yloop:
	mov esi,0
	inc edi
	cmp edi,[len_y]
	jz mul_check
	jmp xloop
mul_check:
	mov esi,[len_mul]
	mov al,[number_mul+esi-2]
	;if al >= 10,then deal
	cmp al,10
	jl mul_check_len
	xor ah,ah
	mov bl,10 	
	div bl									; 除10，AL存储除法操作的商，AH存储除法操作的余数
	add byte [number_mul+esi-1],al
	mov byte [number_mul+esi-2],ah
mul_check_len:								; 检查最高位，如果是0要扣掉
	mov al,[number_mul+esi-1]
	cmp al,0
	jnz mul_ret
	sub byte [len_mul],1
mul_ret:
	call PrintMul
	ret
;===========================END MUL============================

;============================SUB===============================
SubXY:
	mov esi,0
	; 找出比较大的数字，先比较长度，假设x比较长
	mov eax,[len_x]
	mov ebx,[len_y]
	mov [max_len],eax
	mov dword [bigNumAddr],number_x
	mov dword [smallNumAddr],number_y
	cmp eax,ebx
	jge start_sub
	mov [max_len],ebx						; 记录最大长度
	mov [len_sub],ebx
start_sub:
	mov ebx,[bigNumAddr]
	mov al,[ebx+esi]	;x
	mov ebx,[smallNumAddr]
	mov ah,[ebx+esi]	;y
	sub al,ah
	sub al,[Carry]
	mov byte[Carry],0						; 清零借位
	cmp al,0
	jge save_sub_to_men
	mov byte[Carry],1						; 借位
	add al,10
save_sub_to_men:
	mov [number_sub+esi],al
	inc esi
	cmp esi,[max_len]
	jnz start_sub

	;结束，这时候再检查一次，如果carry位没有清空，说明大小反了，重置大小，再算一遍
sub_carry_check:
	cmp byte[Carry],0
	jz sub_ret
	mov dword [bigNumAddr],number_y
	mov dword [smallNumAddr],number_x
	mov byte[Carry],0
	mov byte[neg_sub],1
	mov esi,0
	jmp start_sub
sub_ret:
	ret


ADD_OR_SUB:
	mov al,[neg_x]
	mov ah,[neg_y]
	xor al,ah
	cmp al,0
	jz do_add
	do_sub:
		call SubXY
		cmp byte[neg_x],1
		jz change_sign
		call PrintSub
		jmp add_sub_ret
	change_sign:
		xor byte[neg_sub],1
		call PrintSub
		jmp add_sub_ret
	do_add:
		mov al,[neg_x]
		mov byte[neg_add],al
		call AddXY
		call PrintAdd
		jmp add_sub_ret
	add_sub_ret:
		ret