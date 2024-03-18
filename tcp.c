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

#define TABLE_SIZE 100

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
    char *message = (char*) malloc(strlen("SUCC") + strlen(id) + strlen(IP) + strlen(TCP)+ 5);

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


int pred_command(int fd, char* id){
    char *message = (char*) malloc(strlen("PRED") + strlen(id) + 3);

    if (message == NULL){
        return 1;
    }
    strcpy(message, "PRED ");
    strcat(message, id);
    strcat(message, "\n");


    write(fd, message, strlen(message));
    free(message);

    return 0;

}

int route_command(int fd, char* i, char* n, char* path) {
    //Form message
    char *message = (char*) malloc(strlen("ROUTE") + strlen(i) + strlen(n) + strlen(path) + 6);
    if (message == NULL) {
        return 1;
    }
    strcpy(message, "ROUTE ");
    strcat(message, i);
    strcat(message, " ");
    strcat(message, i);
    strcat(message, " ");
    strcat(message, path);
    strcat(message, "\n");

    //Send message via TCP
    write(fd, message, strlen(message));
    free(message);

    return 0;
}