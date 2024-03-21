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
#include <stdbool.h>

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
    char *message = (char*) malloc(strlen("ROUTE") + strlen(i) + strlen(n) + strlen(path) + 5);
    if (message == NULL) {
        return 1;
    }

    strcpy(message, "ROUTE ");
    strcat(message, i);
    strcat(message, " ");
    strcat(message, n);
    strcat(message, " ");
    strcat(message, path);
    strcat(message, "\n");

    //Send message via TCP
    write(fd, message, strlen(message));
    free(message);

    return 0;
}


void print_forwardingTable(char* forwardingTable[TABLE_SIZE][TABLE_SIZE], char* destination){

    int dest_index = atoi(destination);
    bool is_empty = true;

    printf("Routing to %s\n", destination);

    for (int i = 0; i < TABLE_SIZE; ++i) {
        char* next_hop = forwardingTable[dest_index][i];
        if (next_hop != NULL) {
            is_empty = false;
            printf(" %03d : %s\n", i, next_hop); // Use %03d to print leading zeros
        }
    }

    if (is_empty) {
        printf("Table is empty\n");
    }
}

void print_shortestTable(char* shortestTable[TABLE_SIZE], char*destination){
    int dest_index = atoi(destination);
    printf("\nPath to %s: ", destination);
    if (shortestTable[dest_index] == NULL) {
        printf(" - \n");
    } else {
        printf("%s\n", shortestTable[dest_index]);
    }

}

void print_expeditionTable(char* expedition_table[TABLE_SIZE]) {
    char index_str[4]; // Allocate space for 3 digits and null terminator
    bool printed = false;

    printf("Forwarding: source | destination\n");

    for (int i = 0; i < TABLE_SIZE; ++i) {
        if (expedition_table[i] != NULL) {
            // Convert integer index to string
            if (i < 10) {
                sprintf(index_str, "0%d", i); // Pad with two leading zeros
            } else {
                sprintf(index_str, "%d", i); // No leading zeros needed
            }

            // Print source and next hop
            printf("                %s | %s\n", index_str, expedition_table[i]);
            printed = true;
        }
    }
    if (!printed) {
        printf("No forwarding entries found.");
    }
    printf("\n");
}

