all: tclnt tsvr
tsvr: testServer.c
	gcc -Wall -g testServer.c -o tsvr -lpthread -lrt
tclnt: testClient.c
	gcc -Wall -g testClient.c -o tclnt -lpthread -lrt
clean:
	rm tclnt tsvr *.o