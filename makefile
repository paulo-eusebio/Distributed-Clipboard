CFLAGS= -g -Wall -O3 -std=gnu11

default: all

all: app clipboard

app: app_teste.c library.c utils.c
	gcc app_teste.c library.c utils.c -o app $(CFLAGS)

clipboard: clipboard.c library.c utils.c
	gcc clipboard.c library.c utils.c -o clipboard $(CFLAGS)

clean:
	rm -f *.o app clipboard