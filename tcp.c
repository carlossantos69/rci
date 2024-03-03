#include "tcp.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int entry_command(int fd,char* id, char* IP, char* TCP){
    char *message = (char*) malloc(strlen("ENTRY") + strlen(id) + strlen(IP) + strlen(TCP)+ 5);

    if (message == NULL){
        return 1;
    }
    strcpy(message, "ENTRY ");
    strcat(message, id);
    strcat(message, " ");
    strcat(message, IP);
    strcat(message, " ");
    strcat(message, TCP);
    strcat(message, "\n");

    write(fd, message, strlen(message));
    free(message);

    return 0;
}

int succ_command(int fd, char* id, char* IP, char* TCP){
    char *message = (char*) malloc(strlen("ENTRY") + strlen(id) + strlen(IP) + strlen(TCP)+ 5);

    if (message == NULL){
        return 1;
    }
    strcpy(message, "SUCC ");
    strcat(message, id);
    strcat(message, " ");
    strcat(message, IP);
    strcat(message, " ");
    strcat(message, TCP);
    strcat(message, "\n");

    write(fd, message, strlen(message));
    free(message);

    return 0;

}
