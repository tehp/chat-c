CC=clang
CFLAGS=-Wall -lpthread

ex: server client
	$(CC) $(CFLAGS) server.c -o server
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f *.o
