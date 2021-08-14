all: clientX serverZ

serverZ: serverZ.o 
	gcc -o serverZ serverZ.o   -lpthread -lrt

serverZ.o: serverZ.c
	gcc -c -g  serverZ.c  -Wall

clientX: clientX.o
	gcc clientX.c -o clientX -lpthread -lrt


clientX.o : clientX.c
	gcc -c -g -Wall clientX.c 




clean:
	rm clientX serverZ *.o