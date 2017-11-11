task6.o: prog.c
	gcc -c prog.c -o prog.o
	
task6: prog.o
	gcc prog.o -o prog
	
clean:
	rm -rf cached.txt
	rm -rf uncached.txt
	rm -rf prog.o
	rm -rf prog
