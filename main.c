#include "server.h"

void print_options() {
    puts("\t-s\trunning the program in server mode");
    puts("\t\t-d\tdefines the working directory");
    puts("\t-c\trunning the program in client mode");
    puts("\t\t-a\tdefines ip address of server");
    puts("\t-p\tdefines port of server");
}


int main(int argc, char** argv) {
   
    char is_client_mode = 0;
    char is_server_mode = 0;
    char workdir_path[PATH_MAX];
    char server_address[16];
    int server_port;


    int c;

    while ((c = getopt(argc, argv, "hcsa:p:d:")) != -1) {
        switch (c) {
            case 'c': is_client_mode = 1; break;
            case 's': is_server_mode = 1; break;
            case 'a': strcpy(server_address, optarg); break;
            case 'p': server_port = strtol(optarg, NULL, 10); break;
            case 'd': strcpy(workdir_path, optarg); break;
            default: print_options(); exit(0);
        }
    }

    if (is_client_mode && is_server_mode) {
        print_options();
        exit(0);
    }
    if (is_client_mode && !is_server_mode) {
        client_go(server_address, server_port);
    } else if (is_server_mode && !is_client_mode) {
        server_go(workdir_path, server_port);
    }
    return 0;
}