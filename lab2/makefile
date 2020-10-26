main:
	rm -f main
	nasm -f elf -o my_print.o my_print.asm
	g++ -m32 -o main main.cpp my_print.o
	rm -f *.o
	./main
.PHONY:clean
clean:
	-rm -f *.o
	-rm -f main 
