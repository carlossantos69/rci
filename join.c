#include "join.h"
#include "tejo.h"
#include "tcp.h"

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUFFER_SIZE 2000


int join_command(char** arguments, char* buffer, char* ring, int fd_UDP, struct addrinfo *TEJO_res, char* ID, char* IP, char* TCP, char* succID, char* succIP, char* succTCP, char* second_succID, char* second_succIP, char* second_succTCP, char* predID, bool *registado) {
    //Ver nós
    //Escolher nó
    char* command;
    int nodes_number = 0, succFD;
    socklen_t addrlen;
    struct sockaddr_in addr;
    
    struct addrinfo hints;

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_STREAM; //TCP socket
    hints.ai_flags=AI_PASSIVE;

    command = strtok(buffer, " \t\n");
    int arg_count = 0;
    char *token;
    token = strtok(NULL, " \t\n"); //Eliminate first argument (its ring id)
    while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < 300) {
        arguments[arg_count] = token;
        arg_count++;
    }
    arguments[arg_count] = NULL;

    //Ver lista de nós e escolher o sucessor e o ID
    if (strcmp(command, "NODESLIST") == 0) {
        nodes_number = 0; //Nodes which are in the list
        bool used[100] = { false }; //ID's which are in use on the ring

        for (int i=0; i<arg_count-1; i=i+3) {
            int id = atoi(arguments[i]);
            if (id >= 0 && id < 100) { // Check bounds before accessing used array
                used[id] = true;
                nodes_number++;
            } else {
                printf("ID inválido: %s\n", ID);
                // Decide what to do if ID is invalid, such as exiting the loop or handling the error
            }
        }
        

        if (used[atoi(ID)]) {
            printf("%s já está a ser usado. ", ID);
            for (int i=0; i<100; ++i) {
                if (!used[i]) {
                    sprintf(buffer, "%d", i);
                    strcpy(ID, buffer);
                    if (strlen(ID) == 1) {
                        ID[1] = ID[0];
                        ID[0] = '0';
                        ID[2] = '\0';
                    }
                    break;
                }
            }
            printf("%s vai ser usado\n", ID);
        }

        if (nodes_number > 0) {
            //Escolhe o primeiro nó da lista
            strcpy(succID, arguments[0]);
            strcpy(succIP, arguments[1]);
            strcpy(succTCP, arguments[2]);
        } else { // primeiro no
            strcpy(succID, ID);
            strcpy(succIP, IP);
            strcpy(succTCP, TCP);
            strcpy(second_succID,ID);
            strcpy(second_succIP, IP);
            strcpy(second_succTCP, TCP);
            strcpy(predID, ID);
        }



    }

    if(nodes_number == 0){
        printf("Primeiro nó a juntar-se.\n");
    } else {
        succFD = direct_join(ID, IP, TCP, succIP, succTCP, &hints);
        return succFD;
    }

    reg_node(fd_UDP,TEJO_res, ring, ID, IP, TCP);

    addrlen = sizeof(addr);
    int n = recvfrom(fd_UDP, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {
        printf("Erro a ler do socket UDP\n");
        exit(1);
    }

    printf("Nó registado na rede de nós\n");

    if(memcmp(buffer, "OKREG", strlen("OKREG"))==0){
        *registado = true;
        printf("Registado\n");
    }else{
        exit(1);
    }

    return -1;

}


int direct_join(char* ID, char* IP, char* TCP, char* succIP, char* succTCP, struct addrinfo *res) {

    struct addrinfo hints;
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        printf("Erro a criar a socket TCP\n");
        return(-1); //Erro
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_STREAM; //TCP socket
    hints.ai_flags=AI_PASSIVE;

    int errcode = getaddrinfo(succIP, succTCP, &hints, &res);
    if(errcode != 0) {
        printf("Error searching sucessor IP\n");
        return(-1); //Erro
    }

    int n = connect(fd, res->ai_addr,  res->ai_addrlen);
    if(n == -1) {
        printf("Erro a estabelecer ligação");
        return(-1); //Erro
    }
    freeaddrinfo(res);
    

    entry_command(fd, ID, IP, TCP);

    return fd;
}