section .data
red:			db 1Bh,"[31m"
redSize: 		equ $-red
backgrould:		db 1Bh,"[0m"
bgSize:			equ $-backgrould


section .text
	GLOBAL my_printRed
	GLOBAL my_printWhite

my_printRed:
	mov eax,4
	mov ebx,1
	mov ecx,red
	mov edx,redSize
	int 80h

	mov eax,4
	mov ebx,1
	mov ecx,[esp+4]
	mov edx,[esp+8]
	int 80h
	call print_bgcolor
	ret

my_printWhite:
	mov eax,4
	mov ebx,1
	mov ecx,[esp+4]
	mov edx,[esp+8]
	int 80h
	ret

print_bgcolor:
	mov eax,4
	mov ebx,1
	mov ecx,backgrould
	mov edx,bgSize
	int 80h
	ret