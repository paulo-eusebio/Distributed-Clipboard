CFLAGS= -g -Wall -lpthread -std=gnu11 

default: all

all: appteste appteste3 appfiles clipboard

appteste: app_teste.c library.c utils.c linkedList.c
	gcc app_teste.c library.c utils.c linkedList.c -o appteste $(CFLAGS)

appteste3: app_teste3.c library.c utils.c linkedList.c
	gcc app_teste3.c library.c utils.c linkedList.c -o appteste3 $(CFLAGS)

appfiles: app_files.c library.c utils.c linkedList.c
	gcc app_files.c library.c utils.c linkedList.c -o appfiles $(CFLAGS)

clipboard: clipboard.c library.c utils.c clipthreads.c linkedList.c
	gcc clipboard.c library.c utils.c clipthreads.c linkedList.c -o clipboard $(CFLAGS)

clean:
	rm -f *.o app clipboard
