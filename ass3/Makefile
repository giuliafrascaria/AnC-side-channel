all: clean prog.o prog

prog.o: prog.c
	gcc -c prog.c -o prog.o

prog: prog.o
	gcc prog.o -o prog

clean:
	rm -rf prog.o
	rm -rf prog
	rm scan.txt
	rm uncached.txt
