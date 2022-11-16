all: equipment server

common.o: common.c common.h
	gcc -Wall -c common.c

equipment: equipment.c common.o
	gcc -Wall -o equipment equipment.c common.o

server: server.c common.o
	gcc -Wall -o server server.c common.o

clean:
	@rm -rf equipment server common.o