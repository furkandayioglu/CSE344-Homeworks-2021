all: myFind

myFind: hw1.o

		gcc hw1.o -o myFind

hw1.o : hw1.c

		gcc -c -Wall -std=gnu99 hw1.c


clean:

	sudo rm -f *.o myFind