#ifndef LAB3_MESSAGE_H
#define LAB3_MESSAGE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <stdbool.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define DATA_MAX 3000

#define HELLO_MSG_TXT   "hello handeshake message"

#define DEFAULT_MSG_TXT    "default message"

enum command {
    END = 0,
    HELP,
    CD,
    LS,
    CAT,
    UPLOAD,
    DOWNLOAD,
    EXIT,
    HELLO, // handshake with server
    ERROR, // server error. only from server
    UPDATE // update current directory
};

struct message_strct {
    enum command command;
    char dir[PATH_MAX];
    unsigned long long data_length;
    char* data;
};

void send_message(struct message_strct* message, int socket_desc);
struct message_strct* receive_message(int socket_desc);
struct message_strct* copy_message(struct message_strct* src);
void free_message(struct message_strct* message);


#endif //LAB3_MESSAGE_H
