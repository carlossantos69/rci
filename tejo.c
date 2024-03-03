#include "tejo.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int get_nodeslist (int fd, struct addrinfo *info, char* ring) { //NODES "ring"
    char *message = (char*) malloc(strlen("NODES") + strlen(ring) + 3);
    if (message == NULL) {
        return 1;
    }
    strcpy(message, "NODES ");
    strcat(message, ring);
    strcat(message, "\n");

    return (sendUDP(fd, info, message));
}

int reg_node(int fd, struct addrinfo *info, char* ring, char* id, char* IP, char* TCP) { //REG ring id IP TCP
    char *message = (char*) malloc(strlen("REG") + strlen(ring) + strlen(id) + strlen(IP) + strlen(TCP) + 6);
    if (message == NULL) {
        return 1;
    }
    strcpy(message, "REG ");
    strcat(message, ring);
    strcat(message, " ");
    strcat(message, id);
    strcat(message, " ");
    strcat(message, IP);
    strcat(message, " ");
    strcat(message, TCP);
    strcat(message, "\n");

    return (sendUDP(fd, info, message));
}

int unreg_node(int fd, struct addrinfo *info, char* ring, char* id) {
    char *message = (char*) malloc(strlen("UNREG") + strlen(ring) + strlen(id) + 4);
    if (message == NULL) {
        return 1;
    }
    strcpy(message, "UNREG ");
    strcat(message, ring);
    strcat(message, " ");
    strcat(message, id);
    strcat(message, "\n");

    return (sendUDP(fd, info, message));

    return 0;
}


int sendUDP(int fd, struct addrinfo *info, char* message) {
    int n = sendto(fd, message, strlen(message), 0, info->ai_addr,info->ai_addrlen);
    if(n == -1) {
        return 1;
    }
    free(message);
    return 0;
}

