#include "join.h"
#include "tejo.h"

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


int join_command(char* ring, char* id, int fd, struct addrinfo *info, char* IP, char* TCP) {

    get_nodeslist(fd, info, ring);
    reg_node(fd,info, ring, id, IP, TCP);

    printf("Nó registado na rede de nós\n");

    return 0;
}

int leave_command(char* ring, char* id, int fd, struct addrinfo *info, char* IP, char* TCP){

    unreg_node(fd, info, ring, id);

    printf("Nó retirado da rede de nós\n");

    return 0;
}