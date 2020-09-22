server: server.o utils.o
	cc server.o utils.o -o server -lpthread
server.o: server.c utils.h
	cc -c server.c
utils.o: utils.c utils.h
	cc -c utils.c