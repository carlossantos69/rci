#include "join.h"
#include "tejo.h"
#include "tcp.h"

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


int join_command(char* ring, char* id, int fd, struct addrinfo *info, char* IP, char* TCP) {
    //Ver nós
    //Escolher nó


    //direct_join()
    //register


    get_nodeslist(fd, info, ring);
    reg_node(fd,info, ring, id, IP, TCP);

    printf("Nó registado na rede de nós\n");

    return 0;
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