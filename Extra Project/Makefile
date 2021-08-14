all: clientX serverZ serverY

serverY: serverY.o
	gcc -o serverY serverY.o   -lpthread -lrt

serverY.o:serverY.c
	gcc -c -g  serverY.c  -Wall

serverZ: serverZ.o 
	gcc -o serverZ serverZ.o   -lpthread -lrt

serverZ.o: serverZ.c
	gcc -c -g  serverZ.c  -Wall

clientX: clientX.o
	gcc clientX.o -o clientX -lpthread -lrt


clientX.o : clientX.c
	gcc -c -g -Wall clientX.c 



clean:
	rm clientX serverZ serverY *.o