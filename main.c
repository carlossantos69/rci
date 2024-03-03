#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "tejo.h"
#include "join.h"
#include "help.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

//Servidor TEJO
#define REGIP_DEFAULT "193.136.138.142"
#define REGUDP_DEFAULT "59000"

#define MAX_COMMAND_SIZE 100
#define MAX_ARGUMENT_COUNT 10
#define MAX_NODE_COUNT 100

#define BUFFER_SIZE 2000



int main(int argc, char *argv[]) {
    bool registered = false;
    int fd_TCP, fd_UDP; //File Descriptors
    int errcode, maxfd, counter, arg_count, nodes_number; //Auxiliary variables
    char *IP, *TCP, *regIP, *regUDP;
    char input[MAX_COMMAND_SIZE];
    char buffer[BUFFER_SIZE]; //Temporary read buffers
    char *command;
    char succID[3], succIP[16], succTCP[6]; //Info of Sucessor
    char *arguments[MAX_NODE_COUNT*3];
    ssize_t n;
    struct addrinfo hints, *res, *TEJO_res;
    socklen_t addrlen;
    struct sockaddr_in addr;
    fd_set read_fds; //Select() FD List

    char ring[4];
    char id[3];

    print_help();


    if (argc != 3 && argc != 5) {
        printf("Número de argumentos errado\n");
        exit(1);
        
    }

    IP = (char*) malloc(strlen(argv[1]) + 1);
    if (!IP) {
        printf("Erro ao alocar memória\n");
        exit(1);
    }
    strcpy(IP, argv[1]);

    TCP = (char*) malloc(strlen(argv[2]) + 1);
    if (!TCP) {
        printf("Erro ao alocar memória\n");
        exit(1);
    }
    strcpy(TCP, argv[2]);

    if (argc == 5) {
        regIP = (char*) malloc(strlen(argv[3]) + 1);
        if (!regIP) {
            printf("Erro ao alocar memória\n");
            exit(1);
        }
        strcpy(regIP, argv[3]);

        regUDP = (char*) malloc(strlen(argv[4]) + 1);
        if (!regUDP) {
            printf("Erro ao alocar memória\n");
            exit(1);
        }
        strcpy(regUDP, argv[4]);
    } else {
        regIP = (char*) malloc(strlen(REGIP_DEFAULT) + 1);
        strcpy(regIP, REGIP_DEFAULT);
        if (!regIP) {
            printf("Erro ao alocar memória\n");
            exit(1);
        }
        regUDP = (char*) malloc(strlen(REGUDP_DEFAULT) + 1);
        if (!regUDP) {
            printf("Erro ao alocar memória\n");
            exit(1);
        }
        strcpy(regUDP, REGUDP_DEFAULT);
    }

    //Socket TCP
    fd_TCP = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if (fd_TCP == -1) {
        printf("Erro a criar socket TCP\n");
        exit(1);
    } //error

    
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_STREAM; //TCP socket
    hints.ai_flags=AI_PASSIVE;

    errcode = getaddrinfo(NULL, TCP, &hints, &res);
    if(errcode != 0) {
        printf("Erro a procurar endereço IP\n");
        exit(1);
    }

    n = bind(fd_TCP, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        printf("Erro a dar bind TCP\n");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(fd_TCP, 5) == -1) {
        perror("listen");
        exit(1);
    }

    //Conecção UDP com o servidor tejo
    fd_UDP = socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if(fd_UDP == -1) {
        printf("Erro a criar socket UDP\n");
        exit(1);
    }
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM; //UDP socket

    //Get Tejo Adress
    errcode=getaddrinfo(regIP,regUDP,&hints,&TEJO_res);

    while (1) {
        FD_ZERO(&read_fds); //Remove all descriptors

        FD_SET(STDIN_FILENO, &read_fds); 
        FD_SET(fd_TCP, &read_fds); 
        FD_SET(fd_UDP, &read_fds);

        maxfd = MAX(MAX(STDIN_FILENO, fd_TCP), fd_UDP);

        counter = select(maxfd+1, &read_fds, NULL, NULL, NULL);
        if (counter == -1) {
            printf("Error no select\n");
            exit(1);
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds)) { //STDIN ready
            fgets(input, sizeof(input), stdin);
            printf("User command: %s\n", input);

            //Process command and divide into arguments
            command = strtok(input, " \t\n");
            arg_count = 0;
            char *token;
            while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < MAX_NODE_COUNT*3) {
                arguments[arg_count] = token;
                arg_count++;
            }
            arguments[arg_count] = NULL;

            if (strcmp(command, "ALL") == 0) { //DEBUG ONLY, SHOW ALL NODES
                viewAll(fd_UDP, TEJO_res);
            }

            //Evaluate command
            if (strcmp(command,"NODES") == 0) {
                if (arg_count == 1 && strlen(arguments[0]) == 3) {
                    if (get_nodeslist(fd_UDP, TEJO_res, arguments[0])) {
                        printf("Erro a enviar mensagem UDP\n");
                        exit(1);
                    }
                } else {
                    printf("Sintax error\n");
                }
            }

            if (strcmp(command,"REG") == 0) {
                if (arg_count == 2 && strlen(arguments[0]) == 3 && strlen(arguments[1]) == 2) {
                    if (reg_node(fd_UDP, TEJO_res, arguments[0], arguments[1], IP, TCP)) {
                        printf("Erro a enviar mensagem UDP\n");
                        exit(1);
                    }
                } else {
                    printf("Sintax error\n");
                }
            }

            if (strcmp(command,"UNREG") == 0) {
                if (arg_count == 2 && strlen(arguments[0]) == 3 && strlen(arguments[1]) == 2) {
                    if (unreg_node(fd_UDP, TEJO_res, arguments[0], arguments[1])) {
                        printf("Erro a enviar mensagem UDP\n");
                        exit(1);
                    }
                } else {
                    printf("Sintax error\n");
                }
            }

            if (strcmp(command,"join") == 0 || strcmp(command, "j") == 0 ) { //join (j) ring id
                if (arg_count == 2 && strlen(arguments[0]) == 3 && strlen(arguments[1]) == 2 ) {
                    if(registered){
                        printf("Nó já registado\n");
                    }else{
                        strcpy(ring , arguments[0]);
                        strcpy(id, arguments[1]);
                        get_nodeslist(fd_UDP, TEJO_res, ring);
                        addrlen = sizeof(addr);
                        n = recvfrom(fd_UDP, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen); // ler do Tejo
                        if (n == -1) {
                            printf("Erro a ler do socket UDP\n");
                            exit(1);
                        }

                        command = strtok(buffer, " \t\n");
                        arg_count = 0;
                        char *token;
                        token = strtok(NULL, " \t\n"); //Eliminate first argument (its ring id)
                        while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < 300) {
                            arguments[arg_count] = token;
                            arg_count++;
                        }
                        arguments[arg_count] = NULL;

                        printf("%s\n", command);

                        for (int i=0; i < arg_count; i++)
                        {
                            printf("%s\n", arguments[i]);
                        }

                        if (strcmp(command, "NODESLIST") == 0) {
                            nodes_number = 0; //Nodes which are in the list
                            bool used[100] = { false }; //ID's which are in use on the ring

                            for (int i=0; i<arg_count-1; i=i+3) {
                                printf("Processing argument at index %d: %s\n", i, arguments[i]);
                                int id = atoi(arguments[i]);
                                if (id >= 0 && id < 100) { // Check bounds before accessing used array
                                    used[id] = true;
                                    nodes_number++;
                                } else {
                                    printf("Invalid ID: %d\n", id);
                                    // Decide what to do if ID is invalid, such as exiting the loop or handling the error
                                }
                            }
                            

                            if (used[atoi(id)]) {
                                printf("%s is currently in use. ", id);
                                for (int i=0; i<100; ++i) {
                                    if (!used[i]) {
                                        sprintf(buffer, "%d", i);
                                        strcpy(id, buffer);
                                        if (strlen(id) == 1) {
                                            id[1] = id[0];
                                            id[0] = '0';
                                            id[2] = '\0';
                                        }
                                        break;
                                    }
                                }
                                printf("%s will be used instead\n", id);
                            } else {
                                printf("%s is available and will be used\n", id);
                            }

                            if (nodes_number > 0) { //Do not do copy if first node to join
                                strcpy(succID, arguments[0]);
                                strcpy(succIP, arguments[1]);
                                strcpy(succTCP, arguments[2]);
                            }
                        } 
                        reg_node(fd_UDP, TEJO_res , ring, id, IP, TCP);

                        addrlen = sizeof(addr);
                        n = recvfrom(fd_UDP, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
                        if (n == -1) {
                            printf("Erro a ler do socket UDP\n");
                            exit(1);
                        }

                        if(memcmp(buffer, "OKREG", strlen("OKREG"))==0){
                            printf("SUCESS");
                        }else{
                            printf("falha");
                        }

                    }
                } else {
                    printf("Sintax error\n");
                }
            }

            if(strcmp(command, "dj") == 0){
                if (arg_count == 4 && strlen(arguments[0]) == 3) {
                    strcpy(id, arguments[0]);
                    strcpy(succID, arguments[1]);
                    strcpy(succIP, arguments[2]);
                    strcpy(succTCP, arguments[3]);
                } else {
                    printf("Sintax error\n"); 
                }
            }

            if (strcmp(command,"direct") == 0) {
                if (arg_count == 5) {
                    if (strcmp(arguments[0],"join") == 0 && strlen(arguments[1]) == 3) {
                        strcpy(id, arguments[1]);
                        strcpy(succID, arguments[2]);
                        strcpy(succIP, arguments[3]);
                        strcpy(succTCP, arguments[4]);
                        //directjoin_command(id,succID,succIP,succTCP)
                    } else {
                        printf("Sintax error\n");
                    }
                } else {
                    printf("Sintax error\n"); 
                }
            }

        

            if (strcmp(command,"leave") == 0 || strcmp(command, "l") == 0) { //leave (l)
            
                if (arg_count == 0 && registered) {
                    leave_command(ring, id, fd_UDP, TEJO_res , IP, TCP);
                    registered = false;
                } else {
                    printf("Nó não registado ou erro de syntax\n");
                }
            }

            if (strcmp(command,"exit") == 0 || strcmp(command, "x") == 0) {
                break;
            }
 
        }
        if (FD_ISSET(fd_TCP,&read_fds)) {
            printf("Socket TCP Preparado\n");

        }
        if (FD_ISSET(fd_UDP,&read_fds)) {
            printf("Socket UDP Preparado\n");

            if(memcmp(buffer, "OKREG", strlen("OKREG")) == 0){
                registered = true;
            }
            if(memcmp(buffer, "OKUNREG", strlen("OKUNREG")) == 0){
                registered = false;
            }



            write(1, buffer, n);
            printf("\n");
        }
    }    

    freeaddrinfo(res);
    freeaddrinfo(TEJO_res);
    free(IP);
    free(TCP);
    free(regIP);
    free(regUDP);
    close(fd_TCP);
    close(fd_UDP);
    //Fechar sockets
}