#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curses.h>
#include <pthread.h>
#include <dirent.h>
#include <locale.h>

#include "message.h"

static char current_path[PATH_MAX] = {'\0'};
static char downloaded_filename[PATH_MAX] = {'\0'};
static char last_notification[DATA_MAX] = {'\0'};
struct message_strct* last_ls_message;
int selected_element;

static void print_menu() {
    printw("Use num lk keys to navigate\n");
    printw("USE '8' to go UP.\n");
    printw("USE '2' to go UP.\n");
    printw("USE '5' to go into DIR or view FILE.\n");
    printw("USE 'u' to go upload file. Type filename.\n");
    printw("USE 'd' to download file.\n");
    printw("To exit the program press 'q'.\n");

    refresh();
}

static inline size_t dirent_len(struct dirent* dirent) {
    return sizeof(dirent->d_type) +
           sizeof(dirent->d_ino) +
           sizeof(dirent->d_reclen) +
           sizeof(dirent->d_off) +
           sizeof(strlen(dirent->d_name)) +
           strlen(dirent->d_name) + 1;
}

static void make_handshake(int socket_desc) {
    struct message_strct* hello_msg = malloc(sizeof(struct message_strct));
    struct message_strct* resp_msg;
    hello_msg->command = HELLO;

    send_message(hello_msg, socket_desc);
    resp_msg = receive_message(socket_desc);

    puts(resp_msg->data);

    if (strncmp(resp_msg->data, HELLO_MSG_TXT, strlen(HELLO_MSG_TXT)) != 0) {
        printf("Not expected message from server on handshake\n");
        return;
    }

    strcpy(current_path, resp_msg->dir);
    free(hello_msg);
    free_message(resp_msg);
}

static void send_ls_message(int socket_desc, char* path) {
    struct message_strct* ls_msg = malloc(sizeof(struct message_strct));

    ls_msg->command = LS;
    strcpy(ls_msg->dir, path);
    ls_msg->data_length = 0;
    ls_msg->data = NULL;
    send_message(ls_msg, socket_desc);
    free(ls_msg);
}

static void send_cd_message(int socket_desc, char* path) {
    struct message_strct* cd_msg = calloc(1, sizeof(struct message_strct));

    cd_msg->command = CD;
    cd_msg->data_length = strlen(current_path) + strlen(path) + 2;
    cd_msg->data = calloc(1, cd_msg->data_length);

    strcpy(cd_msg->dir, current_path);
    strcpy(cd_msg->data, current_path);
    strcat(cd_msg->data, "/");
    strcat(cd_msg->data, path);

    send_message(cd_msg, socket_desc);
    free_message(cd_msg);
}

static void send_cat_message(int socket_desc, char* path) {
    struct message_strct* cat_msg = calloc(1, sizeof(struct message_strct));

    cat_msg->command = CAT;
    cat_msg->data_length = strlen(current_path) + strlen(path) + 2;
    cat_msg->data = calloc(1, cat_msg->data_length);

    strcpy(cat_msg->dir, current_path);
    strcpy(cat_msg->data, current_path);
    strcat(cat_msg->data, "/");
    strcat(cat_msg->data, path);

    send_message(cat_msg, socket_desc);
    free_message(cat_msg);
}

static void send_download_message(int socket_desc, char* filename) {
    struct message_strct* download_msg = calloc(1, sizeof(struct message_strct));

    download_msg->command = DOWNLOAD;
    download_msg->data_length = strlen(current_path) + strlen(filename) + 2;
    download_msg->data = calloc(1, download_msg->data_length);

    strcpy(download_msg->dir, current_path);
    strcpy(download_msg->data, current_path);
    strcat(download_msg->data, "/");
    strcat(download_msg->data, filename);

    send_message(download_msg, socket_desc);
    free_message(download_msg);
}

static void send_upload_message(int socket_desc, char* filename) {
    struct message_strct* upload_msg = calloc(1, sizeof(struct message_strct));
    char buffer[DATA_MAX] = {'\0'};
    char* filename_ptr = filename + strlen(filename);
    size_t buf_len;

    FILE* file = fopen(filename, "rb");

    upload_msg->data = NULL;

    if (file != NULL) {
        upload_msg->command = UPLOAD;
        strcpy(upload_msg->dir, current_path);
        strcat(upload_msg->dir, "/");
        // change filename. Save only from last /
        while (*--filename_ptr != '/') {}
        *filename_ptr = '\0';
        strcat(upload_msg->dir, ++filename_ptr);

        upload_msg->data_length = 0;
        upload_msg->data = malloc(1);

        while (fread(buffer, DATA_MAX, 1, file) > 0) {
            upload_msg->data = realloc(upload_msg->data, upload_msg->data_length + DATA_MAX);
            strncpy(upload_msg->data + upload_msg->data_length, buffer, DATA_MAX);
            upload_msg->data_length += DATA_MAX;
        }
        buf_len = strlen(buffer);
        upload_msg->data = realloc(upload_msg->data, upload_msg->data_length + buf_len);
        strncpy(upload_msg->data + upload_msg->data_length, buffer, buf_len);
        upload_msg->data_length += buf_len;

        send_message(upload_msg, socket_desc);
        fclose(file);
    } else {
        strcpy(last_notification, "Can't open file ");
        strcat(last_notification, filename);
    }

    free_message(upload_msg);
}

static struct dirent* get_selected_dirent() {
    size_t data_ptr = 0, i = 0;
    struct dirent* element = malloc(sizeof(struct dirent));

    while(data_ptr < last_ls_message->data_length) {
        memcpy(element, last_ls_message->data + data_ptr, sizeof(struct dirent));
        if (i++ == selected_element) {
            return element;
        }

        data_ptr += dirent_len(element);
    }
    return NULL;
}

static char* get_selected_name() {
    struct dirent* entry = get_selected_dirent();
    char* name = calloc(1, strlen(entry->d_name) + (entry->d_type == DT_DIR ? 2 : 1));
    strcpy(name, entry->d_name);
    if (entry->d_type == DT_DIR) {
        strcat(name, "/");
    }
    free(entry);
    return name;
}

static unsigned char get_selected_type() {
    struct dirent* element = get_selected_dirent();
    puts("after getting selected dirent");
    unsigned char type = element->d_type;
    free(element);
    puts("getting selected type");
    return type;
}

static void print_directory(int step) {
    size_t data_ptr = 0, i = 0;
    struct dirent* element = malloc(sizeof(struct dirent));
    bool printed_selected = false;

    selected_element += step;

    clear();
    print_menu();

    if (selected_element < 0) {
        selected_element = 0;
    }

    while(data_ptr < last_ls_message->data_length) {
        memcpy(element, last_ls_message->data + data_ptr, sizeof(struct dirent));
        if (i++ == selected_element) {
            printw("> %s", element->d_name);
            printed_selected = true;
        } else {
            printw("  %s", element->d_name);
        }

        if (element->d_type == DT_DIR) {
            printw("/\n");
        } else {
            printw("\n");
        }
        data_ptr += dirent_len(element);
    }

    free(element);

    if (printed_selected == false) {
        print_directory(-1);
    }

    refresh();
}

static void print_file(struct message_strct* file_message) {
    file_message->data[file_message->data_length] = '\0';
    clear();
    printw("To go back press 'r'\n\n");
    printw("%s", file_message->data);
    refresh();
}

static void save_file(struct message_strct* file_message) {
    FILE* file;
    char filename[PATH_MAX] = {'\0'};
    char buffer[DATA_MAX] = {'\0'};
    unsigned long long data_ptr = 0;

    strcpy(filename, getenv("HOME"));
    strcat(filename, "/Загрузки/");
    strcat(filename, downloaded_filename);
    file = fopen(filename, "wb");

    if (file != NULL) {

        while (data_ptr < file_message->data_length) {
            strncpy(buffer, file_message->data + data_ptr, MIN(DATA_MAX, file_message->data_length));
            fwrite(buffer, MIN(DATA_MAX, file_message->data_length), 1, file);
            data_ptr += DATA_MAX;
        }

        strcpy(last_notification, "File ");
        strcat(last_notification, downloaded_filename);
        strcat(last_notification, " was saved as ");
        strcat(last_notification, filename);

        fclose(file);
    } else {
        endwin();
        puts(filename);
        puts("Can't download file in DOWNLOADS directory");
        exit(EXIT_FAILURE);
    }
}

static void start_tui_handler(int socket_desc) {
    int input_char, i;
    char* name;
    char inputFilename[PATH_MAX] = {'\0'};

    int view_mode = 0;
    /*
        0   - view dir mode
        1   - view file mode
        2   - upload file mode
    */

    do {
        input_char = getch();

        if (view_mode == 2) {
            i = 0;
            while (input_char != '\n') {
                inputFilename[i++] = input_char;
                input_char = getch();
            }
            inputFilename[PATH_MAX - 1] = '\0';
            inputFilename[i] = '\0';
            send_upload_message(socket_desc, inputFilename);
            view_mode = 0;
            noecho();
            cbreak();
            print_directory(0);
            continue;
        }

        switch (input_char) {
            case '7':
                if (view_mode == 1) {
                    send_ls_message(socket_desc, current_path);
                    view_mode = 0;
                }
                break;
            case '8':
                if (view_mode == 0) {
                    print_directory(-1);
                }
                break;
            case '2':
                if (view_mode == 0) {
                    print_directory(1);
                }
                break;
            case 'd':
                if (view_mode == 0) {
                    name = get_selected_name();
                    if (get_selected_type() == DT_REG) {
                        strcpy(downloaded_filename, name);
                        send_download_message(socket_desc, name);
                    }
                    free(name);
                }
                break;
            case 'u':
                if (view_mode == 0) {
                    view_mode = 2;
                    echo();
                    nocbreak();
                }
                break;
            case '5':
                if (view_mode == 0) {
                    name = get_selected_name();
                    if (get_selected_type() == DT_DIR) {
                        send_cd_message(socket_desc, name);
                    } else {
                        send_cat_message(socket_desc, name);
                        view_mode = 1;
                    }
                    free(name);
                }
                break;
        }
    } while (input_char != 'q');
}

void* receive_message_handler(void* socket_desc) {
    puts("----------into receive handler");
    int socket = *(int*) socket_desc;
    struct message_strct* input_message = NULL;
    do {
        free_message(input_message);
        input_message = receive_message(socket);
        switch (input_message->command) {
            case CD:
                strcpy(current_path, input_message->dir);
                send_ls_message(socket, current_path);
                break;
            case LS:
                last_ls_message = copy_message(input_message);
                selected_element = 0;
                print_directory(0);
                break;
            case CAT:
                print_file(input_message);
                break;
            case DOWNLOAD:
                save_file(input_message);
                print_directory(0);
                break;
            case UPDATE:
                send_ls_message(socket, current_path);
                break;
            case EXIT:
                endwin();
                puts("Server closed");
                close(socket);
                exit(0);
            case ERROR:
                // endwin and exit
                break;
            case HELLO:
            case UPLOAD:
            case HELP:
                break;
        }
    } while(input_message->command != EXIT);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct sockaddr_in sock_server_addr;
pthread_t receive_thread;

void init_socket(char* addr, int port, int* fd){
    sock_server_addr.sin_addr.s_addr = inet_addr(addr);
    sock_server_addr.sin_family = AF_INET;
    sock_server_addr.sin_port = htons(port);

    if ((*fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printw("Failed to create socket");
        return;
    }

    if (connect(*fd, (struct sockaddr*) &sock_server_addr, sizeof(sock_server_addr)) < 0) {
        perror("Connection error");
        return;
    }
}


void start_receiving(int fd){
    int* p_socket = malloc(1);
    *p_socket = fd;
    if (pthread_create(&receive_thread, NULL, receive_message_handler, (void*)p_socket) != 0) {
        perror("Couldn't create receiver thread");
        return;
    }
}

void start_tui(int fd){
    
    initscr();
    noecho();
    print_menu();

    puts("BEFORE LS");
    send_ls_message(fd, current_path);
    puts("AFTERq LS");
    start_tui_handler(fd);

    endwin();
}

void client_go(char* addr, int port) {
    
    char* server_addr;
    int server_port;
    int client_fd;

    server_addr = addr;
    server_port = port;

    setlocale(LC_ALL, "");
    init_socket(server_addr, server_port, &client_fd);
    make_handshake(client_fd);
    start_receiving(client_fd);
    if (current_path[0] != '/') return;
    start_tui(client_fd);


}