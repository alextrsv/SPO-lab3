#include "server.h"

static char root[PATH_MAX] = {'\0'};

static inline size_t dirent_len(struct dirent* dirent) {
    return sizeof(dirent->d_type) +
           sizeof(dirent->d_ino) +
           sizeof(dirent->d_reclen) +
           sizeof(dirent->d_off) +
           sizeof(strlen(dirent->d_name)) +
           strlen(dirent->d_name) + 1;
}

static bool check_dir(char* dir) {
    if (strlen(dir) < strlen(root)) return false;
    return strncmp(dir, root, strlen(root)) == 0;
}

static struct message_strct* handle_hello_message(struct message_strct* message) {
    struct message_strct* output_message = malloc(sizeof(struct message_strct));
    output_message->command = HELLO;
    output_message->data = malloc(strlen(HELLO_MSG_TXT) + 1);
    strcpy(output_message->data, HELLO_MSG_TXT);
    output_message->data_length = strlen(HELLO_MSG_TXT) + 1;
    strcpy(output_message->dir, root);
    return output_message;
}

static struct message_strct* handle_default_message(struct message_strct* message) {
    struct message_strct* output_message = malloc(sizeof(struct message_strct));
    output_message->command = HELP;
    output_message->data = malloc(strlen(DEFAULT_MSG_TXT) + 1);
    strcpy(output_message->data, DEFAULT_MSG_TXT);
    output_message->data_length = strlen(DEFAULT_MSG_TXT) + 1;
    return output_message;
}



static struct message_strct* handle_cd_message(struct message_strct* message) {
    struct message_strct* output_message = calloc(1, sizeof(struct message_strct));
    output_message->data_length = 0;
    char real_path[PATH_MAX] = {'\0'};
    char selected_dir[PATH_MAX] = {'\0'};
    strncpy(selected_dir, message->data, message->data_length);
    puts(selected_dir);
    realpath(selected_dir, real_path);
    puts(real_path);
    if (check_dir(real_path)) {
        output_message->command = CD;
        strcpy(output_message->dir, real_path);
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}

static struct message_strct* handle_ls_message(struct message_strct* message) {
    struct message_strct* output_message = malloc(sizeof(struct message_strct));
    DIR* dir;
    struct dirent* entry;
    size_t dirent_size;
    size_t data_ptr = 0;
    output_message->data_length = 0;
    output_message->data = malloc(1);

    strcpy(output_message->dir, message->dir);

    if (check_dir(message->dir)) {
        output_message->command = LS;
        dir = opendir(message->dir);
        if (!dir) {
            output_message->command = ERROR;
            return output_message;
        }
        while ((entry = readdir(dir)) != NULL) {
            dirent_size = dirent_len(entry);
            data_ptr = output_message->data_length;
            output_message->data_length += dirent_size;
            output_message->data = realloc(output_message->data, output_message->data_length);
            memcpy(output_message->data + data_ptr, entry, dirent_size);
        }
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}

static struct message_strct* handle_cat_message(struct message_strct* message) {
    struct message_strct* output_message = malloc(sizeof(struct message_strct));
    char filename[PATH_MAX] = {'\0'};
    char buffer[DATA_MAX] = {'\0'};
    size_t buf_len;
    FILE* file;

    output_message->data_length = 0;
    output_message->data = malloc(1);

    if (message->data[0] == '/') {
        strncpy(filename, message->data, message->data_length);
    } else {
        strcat(filename, message->dir);
        strncat(filename, message->data, message->data_length);
    }
    if (check_dir(filename)) {
        output_message->command = CAT;
        file = fopen(filename, "rb");
        while (fread(buffer, DATA_MAX, 1, file) > 0) {
            output_message->data = realloc(output_message->data, output_message->data_length + DATA_MAX);
            strncpy(output_message->data + output_message->data_length, buffer, DATA_MAX);
            output_message->data_length += DATA_MAX;
        }
        buf_len = strlen(buffer);
        output_message->data = realloc(output_message->data, output_message->data_length + buf_len);
        strncpy(output_message->data + output_message->data_length, buffer, buf_len);
        output_message->data_length += buf_len;
        fclose(file);
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}

static struct message_strct* handle_upload_message(struct message_strct* message) {
    struct message_strct* output_message = malloc(sizeof(struct message_strct));
    char buffer[DATA_MAX] = {'\0'};
    unsigned long long data_ptr = 0;
    FILE* file = fopen(message->dir, "wb");

    if (check_dir(message->dir) && file != NULL) {

        while(data_ptr < message->data_length) {
            strncpy(buffer, message->data + data_ptr, MIN(DATA_MAX, message->data_length));
            fwrite(buffer, MIN(DATA_MAX, message->data_length), 1, file);
            data_ptr += DATA_MAX;
        }
        output_message->command = UPDATE;

        fclose(file);
    } else {
        output_message->command = ERROR;
    }

    return output_message;
}

static struct message_strct* handle_download_message(struct message_strct* message) {
    struct message_strct* output_message = malloc(sizeof(struct message_strct));
    char filename[PATH_MAX] = {'\0'};
    char buffer[DATA_MAX] = {'\0'};
    size_t buf_len;
    FILE* file;

    output_message->data_length = 0;
    output_message->data = malloc(1);

    if (message->data[0] == '/') {
        strncpy(filename, message->data, message->data_length);
    } else {
        strcat(filename, message->dir);
        strncat(filename, message->data, message->data_length);
    }

    if (check_dir(filename)) {
        output_message->command = DOWNLOAD;
        file = fopen(filename, "rb");
        while (fread(buffer, DATA_MAX, 1, file) > 0) {
            output_message->data = realloc(output_message->data, output_message->data_length + DATA_MAX);
            strncpy(output_message->data + output_message->data_length, buffer, DATA_MAX);
            output_message->data_length += DATA_MAX;
        }
        buf_len = strlen(buffer);
        output_message->data = realloc(output_message->data, output_message->data_length + buf_len);
        strncpy(output_message->data + output_message->data_length, buffer, buf_len);
        output_message->data_length += buf_len;
        fclose(file);
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}


static struct message_strct* handle_message(struct message_strct* message) {
    switch (message->command) {
        case CD:
            return handle_cd_message(message);
        case LS:
            return handle_ls_message(message);
        case CAT:
            return handle_cat_message(message);
        case UPLOAD:
            return handle_upload_message(message);
        case DOWNLOAD:
            return handle_download_message(message);
        case HELLO:
            return handle_hello_message(message);
    }
    return handle_default_message(message);
}

void* exit_handler(void* socket_desc) {
    char input;
    do {
        scanf("%c", &input);
    } while (input != EXIT_CHAR);
    close(*(int*) socket_desc);
    exit(0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    struct message_strct *input_message, *output_message;
    fd_set active_fd_set, read_fd_set;
    pthread_t exit_thread;

void do_process(){
    
    socklen_t adrlen = sizeof client_addr;
    while (true) {
        read_fd_set = active_fd_set;

        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("select()");
            return;
        }


        for (int i = 0; i < sizeof(active_fd_set)/sizeof(int); i++) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == server_fd) {
                    new_socket = accept(server_fd, (struct sockaddr*) &client_addr, &adrlen);
                    if (new_socket < 0) {
                        perror("accept()");
                        return;
                    }
                    FD_SET(new_socket, &active_fd_set);
                    puts("Connection accepted");
                    puts("TEST-0");
                    input_message = receive_message(new_socket);
                    output_message = handle_message(input_message);

                    send_message(output_message, new_socket);
                    puts("TEST-0.5");

                    free_message(input_message);
                    free_message(output_message);
                    puts("TEST-0.75");
            
                } else {
                    puts("TEST---1");
                    input_message = receive_message(i);
                    if (input_message->command != END) {
                        output_message = handle_message(input_message);
                        puts("TEST---2");
                        if (input_message->command == UPLOAD && output_message->command == UPDATE) {
                            puts("TEST---3");
                            for (int j = 0; j < sizeof(active_fd_set)/sizeof(int); j++) {
                                puts("into circle");
                                printf("\n%s,%d\n", "before sending, j=", j);
                                send_message(output_message, j);
                            }
                        } else {
                            send_message(output_message, i);
                        }
                    } else {
                        puts("ENDSIG is achived!!!!!-------------------------------");
                        close(i);
                        FD_CLR(i, &active_fd_set);
                    }
                    free_message(input_message);
                }
            }
        }
    }
}


void server_go(char* root_dir, int port) {

     int* server_socket = malloc(1);

     signal(SIGPIPE, SIG_IGN);


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Couldn't create socket");
        return;
    }

    if ((bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0) {
        perror("Failed to bind server");
        return;
    }

    listen(server_fd, MAX_BACKLOG);

    strcpy(root, root_dir);

    *server_socket = server_fd;

    if (pthread_create(&exit_thread, NULL, exit_handler, (void*) server_socket) != 0) {
        perror("Couldn't create exit thread.");
        return;
    }

    FD_ZERO(&active_fd_set);
    FD_SET(server_fd, &active_fd_set);

    printf("port: %u.\nroot directory: %s\n", port, root_dir);



    do_process();

}
