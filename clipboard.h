#ifndef CLIPBOARD
#define CLIPBOARD

#define INBOUND_FIFO "INBOUND_FIFO"
#define OUTBOUND_FIFO "OUTBOUND_FIFO"
#include <sys/types.h>

struct Message {
    int type;
    int region;
    int length;
    char message[100];
};

// type -> 0 if copy, 1 if paste

int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);

#endif

