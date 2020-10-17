#!/bin/bash
nasm -f elf32 main.asm 
ld -m elf_i386 main.o -o main
./main
