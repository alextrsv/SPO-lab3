#ifndef LAB3_SERVER_H
#define LAB3_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>
#include "message.h"

#define MAX_BACKLOG 128
#define EXIT_CHAR 'q'

void start_server(char* root_dir, int port);

#endif