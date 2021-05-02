all: program

program: midterm.o

		gcc midterm.o -o program -lrt -pthread 

midterm.o : midterm.c

		gcc -c -Wall -std=gnu99 midterm.c


clean:

	sudo rm -f *.o program