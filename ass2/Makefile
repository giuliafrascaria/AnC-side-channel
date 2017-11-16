all: clean prog.o prog

prog.o: prog.c
	gcc -c prog.c -o prog.o
	
prog: prog.o
	gcc prog.o -o prog
	
clean:
	rm -rf cached.txt
	rm -rf uncached.txt
	rm -rf prog.o
	rm -rf prog
	rm -rf plots.png
