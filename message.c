#include "message.h"

void send_message(struct message_strct* message, int socket_desc) {
    if (write(socket_desc, message, sizeof(struct message_strct)) < 0) {
        return;
    }
    if (write(socket_desc, message->data, message->data_length * sizeof(char)) < 0) {
        return;
    }
}

struct message_strct* receive_message(int socket_desc) {
    struct message_strct* receiving_message = calloc(1, sizeof(struct message_strct));
    read(socket_desc, receiving_message, sizeof(struct message_strct));
    receiving_message->data = malloc(receiving_message->data_length * sizeof(char));
    read(socket_desc, receiving_message->data, receiving_message->data_length);
    return receiving_message;
}

struct message_strct* copy_message(struct message_strct* src) {
    struct message_strct* dst = NULL;
    dst = calloc(1, sizeof(struct message_strct));
    dst->data = calloc(1, src->data_length);
    memcpy(dst, src, sizeof(struct message_strct) + src->data_length);
    return dst;
}

void free_message(struct message_strct* message) {
    if (message == NULL) return;
    if (message->data != NULL) {
        free(message->data);
    }
    free(message);
}