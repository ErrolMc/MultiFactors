client: client.o utils.o
	cc client.o utils.o -o client -lpthread
client.o: client.c utils.h
	cc -c client.c
utils.o: utils.c utils.h
	cc -c utils.c