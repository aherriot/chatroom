CC = gcc
DEBUG = -g
CFLAGS = -Wall -lpthread -c $(DEBUG)
LFLAGS = -Wall -lpthread $(DEBUG)

all: client server

client: client.o chatroom_utils.o
	$(CC) $(LFLAGS) client.o chatroom_utils.o -o client

server: server.o chatroom_utils.o
	$(CC) $(LFLAGS) server.o chatroom_utils.o -o server


client.o: client.c chatroom_utils.h
	$(CC) $(CFLAGS) client.c

server.o: server.c chatroom_utils.h
	$(CC) $(CFLAGS) server.c

chatroom_utils.o: chatroom_utils.h chatroom_utils.c
	$(CC) $(CFLAGS) chatroom_utils.c

clean:
	rm -rf *.o *~ client server
