all: clientX serverZ 
clientX: clientX.c
	gcc -Wall -g clientX.c -o clientX -lpthread -lrt
serverZ: serverZ.c
	gcc -Wall -g serverZ.c -o serverZ -lpthread -lrt
clean:
	rm clientX serverZ *.o