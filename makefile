CFLAGS= -g -Wall -lpthread -std=gnu11 

default: all

all: app clipboard

app: app_teste.c library.c utils.c linkedList.c
	gcc app_teste.c library.c utils.c linkedList.c -o app $(CFLAGS)

clipboard: clipboard.c library.c utils.c clipthreads.c linkedList.c
	gcc clipboard.c library.c utils.c clipthreads.c linkedList.c -o clipboard $(CFLAGS)

clean:
	rm -f *.o app clipboard
