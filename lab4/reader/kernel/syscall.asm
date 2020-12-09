
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_myprint      	equ 1 ; 
_NR_mysleep      	equ 2 ; 
_NR_p      	equ 3 ; 
_NR_v      	equ 4 ; 
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global	myprint
global	mysleep
global	P
global	V

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

myprint:
	mov	eax, _NR_myprint
	mov ecx,[esp+4]
	int	INT_VECTOR_SYS_CALL
	ret
mysleep:
	mov	eax, _NR_mysleep
	mov ecx,[esp+4]
	int	INT_VECTOR_SYS_CALL
	ret
P:
	mov eax,_NR_p
	mov ecx,[esp+4];
	int INT_VECTOR_SYS_CALL
	ret

V:
	mov eax,_NR_v
	mov ecx,[esp+4]
	int INT_VECTOR_SYS_CALL
	ret

