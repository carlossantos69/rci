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
#include "tcp.h"

#define MAX(A,B) ((A)>=(B)?(A):(B))

//Servidor TEJO
#define TEJO_IP "193.136.138.142"
#define TEJO_UDP "59000"

#define MAX_COMMAND_SIZE 100
#define MAX_ARGUMENT_COUNT 10
#define MAX_NODE_COUNT 100

#define BUFFER_SIZE 2000



int main(int argc, char *argv[]) {
    bool registado = false;
    bool SendSuccOnPred = false; 
    bool inRing = false;
    int fd_TCP, fd_UDP, fd; //File Descriptors
    int errcode, maxfd, counter, arg_count;

    char ID[3];
    char *IP, *TCP, *regIP, *regUDP;
    char succID[3], succIP[16], succTCP[6]; //Info of Sucessor    
    char second_succID[3], second_succIP[16], second_succTCP[6]; //Info of Sucessor
    char predID[3];
    int succFD = -1, predFD = -1; //Descritores

    char input[MAX_COMMAND_SIZE];
    char buffer[BUFFER_SIZE]; //Temporary read buffers
    char *command;
    char *arguments[MAX_NODE_COUNT*3];
    ssize_t n;
    struct addrinfo hints, *res, *TEJO_res;
    socklen_t addrlen;
    struct sockaddr_in addr;
    fd_set read_fds;

    char ring[4];

    print_help(); //Imprimit lista de comandos


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
        regIP = (char*) malloc(strlen(TEJO_IP) + 1);
        strcpy(regIP, TEJO_IP);
        if (!regIP) {
            printf("Erro ao alocar memória\n");
            exit(1);
        }
        regUDP = (char*) malloc(strlen(TEJO_UDP) + 1);
        if (!regUDP) {
            printf("Erro ao alocar memória\n");
            exit(1);
        }
        strcpy(regUDP, TEJO_UDP);
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

    errcode=getaddrinfo(regIP,regUDP,&hints,&TEJO_res);

    //Socket TCP
    fd_TCP = socket(AF_INET, SOCK_STREAM, 0);
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
        printf("Erro a dar bind UDP\n");
        exit(1);
    }
    freeaddrinfo(res);

    if (listen(fd_TCP, 5) == -1) {
        perror("listen");
        exit(1);
    }

    while (1) {
        FD_ZERO(&read_fds); //Remove all descriptors

        FD_SET(STDIN_FILENO, &read_fds); 
        FD_SET(fd_TCP, &read_fds); 

        maxfd = MAX(STDIN_FILENO, fd_TCP);

        //Só meter se tiver inicializado
        if (succFD != -1) {
            FD_SET(succFD, &read_fds); 
            maxfd = MAX(maxfd, succFD);
        }
        if(predFD != -1){            
            FD_SET(predFD, &read_fds); 
            maxfd = MAX(maxfd, predFD);
        }

        counter = select(maxfd+1, &read_fds, NULL, NULL, NULL);
        if (counter == -1) {
            printf("Error no select\n");
            exit(1);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) { //Input ready
            fgets(input, sizeof(input), stdin);

            command = strtok(input, " \t\n");
            arg_count = 0;
            char *token;
            while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < MAX_NODE_COUNT*3) {
                arguments[arg_count] = token;
                arg_count++;
            }
            arguments[arg_count] = NULL;

            if (strcmp(command,"leave") == 0 || strcmp(command, "l") == 0) { //leave (l)
                if (arg_count == 0 && inRing) {
                    if (predFD != -1) {
                        close(predFD);
                        predFD = -1;
                    }
                    if (succFD != -1) {
                        close(succFD);
                        succFD = -1;
                    }

                    if (registado) {
                        unreg_node(fd_UDP, TEJO_res, ring, ID);
                        inRing = false;
                        registado = false;
                    }

                    printf("Nó saiu do anel\n");
                   
                } else {
                    printf("Nó não registado ou erro de syntax\n");
                }
            }

            if (strcmp(command,"exit") == 0 || strcmp(command, "x") == 0) {
                if (predFD != -1) {
                    close(predFD);
                }
                if (succFD != -1) {
                    close(succFD);
                }
                if (registado) {
                    unreg_node(fd_UDP, TEJO_res, ring, ID);
                }
                break;
            }


            if (strcmp(command,"join") == 0 || strcmp(command, "j") == 0 ) { //join (j) ring id
                if (arg_count == 2 && strlen(arguments[0]) == 3 && strlen(arguments[1]) == 2 ) {
                    if (inRing) {
                        printf("Nó já está no anel\n");
                    } else {
                        strcpy(ring , arguments[0]);
                        strcpy(ID, arguments[1]);
                        get_nodeslist(fd_UDP, TEJO_res, ring);
                        addrlen = sizeof(addr);
                        n = recvfrom(fd_UDP, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen); // ler do Tejo
                        if (n == -1) {
                            printf("Erro a ler do socket UDP\n");
                            exit(1);
                        }

                        succFD = join_command(arguments, buffer, ring, fd_UDP, TEJO_res, ID, IP, TCP, succID, succIP, succTCP, second_succID, second_succIP, second_succTCP, predID, &registado);

                        //Entrar no anél e mandar comando para sucessor

                        inRing = true;
                    }
                } else {
                    printf("Sintax error\n");
                }
            }

            if (strcmp(command, "dj") == 0) {
                if (arg_count == 4 && strlen(arguments[0]) == 2 && strlen(arguments[1]) == 2 && strlen(arguments[3]) == 5) {
                    if (inRing) {
                        printf("Nó já está no anel\n");
    
                    } else {
                        strcpy(ID, arguments[0]);
                        strcpy(succID, arguments[1]);
                        strcpy(succIP, arguments[2]);
                        strcpy(succTCP, arguments[3]);


                        succFD = direct_join(ID, IP, TCP, succIP, succTCP, &hints);
                        inRing = true;
                    }

                } else {
                    printf("Sintax error\n"); 
                }
            }
            

            if (strcmp(command,"direct") == 0) {
                if (arg_count == 5) {
                    if (strcmp(arguments[0],"join") == 0 && strlen(arguments[1]) == 2 && strlen(arguments[2]) == 2 && strlen(arguments[4]) == 5) {
                        if (inRing) {
                            printf("Nó já está no anel\n");
                        } else {
                            strcpy(ID, arguments[1]);
                            strcpy(succID, arguments[2]);
                            strcpy(succIP, arguments[3]);
                            strcpy(succTCP, arguments[4]);

                            succFD = direct_join(ID, IP, TCP, succIP, succTCP, &hints);
                            inRing = true;
                        }
                    }
                    
                } else {
                    printf("Sintax error\n"); 
                }
            }


            if (strcmp(command, "show") == 0 && strcmp(arguments[0], "topology")== 0 ) {                  
                printf("Showing Topology: \n");
                if(inRing){
                    printf("O nó %s tem ip: %s  e porta: %s\n", ID ,IP, TCP);
                    if (registado) {
                        printf("O nó está registado no anel %s\n", ring);
                    }
                    printf("O seu sucessor %s tem ip: %s e porta: %s\n", succID, succIP, succTCP);
                    printf("O seu segundo sucessor %s tem ip: %s e porta %s\n", second_succID, second_succIP, second_succTCP); 
                    printf("O seu predecessor tem id %s \n", predID);
                }
                else{
                    printf("Nó não está no anel\n");
                }
                    
            }

            if(strcmp(command, "st")== 0){       
                printf("Showing Topology: \n");   
                if(inRing){
                    printf("O nó %s tem ip: %s  e porta: %s\n", ID ,IP, TCP);
                    if (registado) {
                        printf("O nó está registado no anel %s\n", ring);
                    }
                    printf("O seu sucessor %s tem ip: %s e porta: %s\n", succID, succIP, succTCP);                     
                    printf("O seu segundo sucessor %s tem ip: %s e porta %s\n", second_succID, second_succIP, second_succTCP);    
                    printf("O seu predecessor tem id %s \n", predID);                 
                }
                else{
                    printf("Nó não está no anel\n");
                }
            }


        }
        if (FD_ISSET(fd_TCP,&read_fds)) {

            addrlen = sizeof(addr);
            if ((fd = accept(fd_TCP, (struct sockaddr*) &addr, &addrlen)) == -1) {
                printf("Error accepting TCP connection\n");
                exit(1);
            }
            
            n = read(fd, buffer, BUFFER_SIZE);
            
            if (n== -1) {
                printf("Error reading TCP message\n");
                exit(1);
            }
            buffer[n] = '\0';

            printf("Received via TCP fd\n");
            write(1, buffer, n);

            command = strtok(buffer, " \t\n");
            arg_count = 0;
            char *token;
            while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < MAX_NODE_COUNT*3) {
                arguments[arg_count] = token;
                arg_count++;
            }
            arguments[arg_count] = NULL;

            if (strcmp(command, "ENTRY") == 0) {
                
                strcpy(predID, arguments[0]);

                if (strcmp(ID, succID) == 0) {
                    strcpy(second_succID, ID);
                    strcpy(second_succIP, IP);
                    strcpy(second_succTCP, TCP);
                    strcpy(succID, arguments[0]);
                    strcpy(succIP, arguments[1]);
                    strcpy(succTCP, arguments[2]);

                    succFD = socket(AF_INET, SOCK_STREAM, 0);
                    if(succFD == -1){
                        printf("Erro a criar a socket TCP\n");
                        exit(1);
                    }

                    errcode = getaddrinfo(arguments[1], arguments[2], &hints, &res);
                    if(errcode != 0) {
                        printf("Error a procurar IP do novo nó\n");
                        exit(1);
                    }

                    n = connect(succFD, res->ai_addr,  res->ai_addrlen);
                    if(n==-1) {
                        printf("Erro a estabelecer ligação com o novo nó");
                        exit(1);
                    }
                    freeaddrinfo(res);

                    succ_command(fd, arguments[0], arguments[1], arguments[2]);
                    pred_command(succFD, ID);

                } else if (strcmp(ID, second_succID) == 0) {
                    strcpy(second_succID, arguments[0]);
                    strcpy(second_succIP, arguments[1]);
                    strcpy(second_succTCP, arguments[2]);

                    entry_command(predFD, arguments[0], arguments[1], arguments[2]);
                    succ_command(fd, succID, succIP, succTCP);

                    
                } else {
                    entry_command(predFD, arguments[0], arguments[1], arguments[2]);
                    succ_command(fd, succID, succIP, succTCP);
                }


                if(predFD != -1){
                    close(predFD);
                    predFD = -1;
                }
                predFD = fd;
            }            

            if (strcmp(command, "PRED") == 0) {
                if (predFD != -1) {
                    close(predFD);
                }
                predFD = fd;
                strcpy(predID, arguments[0]);

                if(SendSuccOnPred){
                    succ_command(predFD, succID, succIP, succTCP);
                }
                SendSuccOnPred = false;                
            }

        }


        if (FD_ISSET(succFD, &read_fds)) {
            printf("Recebido do sucessor\n");

            n = read(succFD, buffer, BUFFER_SIZE);
            if (n == -1) {
                printf("Error reading TCP message sucessor\n");
                exit(1);
            } else if(n == 0) { // Sucessor saiu
                close(succFD);
                succFD = -1;

                strcpy(succID, second_succID);
                strcpy(succIP, second_succIP);
                strcpy(succTCP, second_succTCP);


                errcode = getaddrinfo(second_succIP, second_succTCP, &hints, &res);
                if (errcode !=0) {
                    printf("Erro a procurar o sucessor\n");
                    exit(1);
                }

                succFD = socket(AF_INET, SOCK_STREAM, 0);
                if (succFD == -1) {
                    printf("Erro a criar a socket TCP\n");
                    exit(1);
                }

                n = connect(succFD, res->ai_addr,  res->ai_addrlen);
                if(n==-1) {
                    printf("Erro a estabelecer ligação");
                    exit(1);
                }
                freeaddrinfo(res);

                pred_command(succFD, ID);
                succ_command(predFD, succID, succIP, succTCP);
            } else {

                buffer[n] = '\0'; //Experimentar

                write(1, buffer, n);

                command = strtok(buffer, " \t\n");
                arg_count = 0;
                char *token;
                while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < MAX_NODE_COUNT*3) {
                    arguments[arg_count] = token;
                    arg_count++;
                }
                arguments[arg_count] = NULL;


                if(strcmp(command, "SUCC") == 0){
                    strcpy(second_succID, arguments[0]);
                    strcpy(second_succIP, arguments[1]);
                    strcpy(second_succTCP, arguments[2]);
                }

                if(strcmp(command, "ENTRY") == 0) { //Entry vindo do sucessor
                    
                    if (succFD != -1) {
                        close(succFD);
                    }


                    succFD = socket(AF_INET, SOCK_STREAM, 0);
                    if(succFD == -1){
                        printf("Erro a criar a socket TCP\n");
                        exit(1);
                    }
                    
                    //Fechar conecção e conectar a arguments[1] arguments[2]
                    n=getaddrinfo(arguments[1], arguments[2], &hints, &res);
                    if(n!=0)/*error*/exit(1);

                    n=connect(succFD, res->ai_addr, res->ai_addrlen);
                    if(n==-1)/*error*/exit(1);
                    freeaddrinfo(res);


                    strcpy(second_succID, succID);
                    strcpy(second_succIP, succIP);
                    strcpy(second_succTCP, succTCP);
                    strcpy(succID, arguments[0]);
                    strcpy(succIP, arguments[1]);
                    strcpy(succTCP, arguments[2]);

                    pred_command(succFD, ID); //Informar o nó que entrou
                    if (predFD != -1) {
                        succ_command(predFD, arguments[0], arguments[1], arguments[2]);
                    }
                    
                }
            }

        }

        if (FD_ISSET(predFD, &read_fds)) {
            printf("Recebido do predecessor\n");

            n = read(predFD, buffer, BUFFER_SIZE);
            if (n == -1) {
                printf("Error reading TCP message\n");
                exit(1);
            } else if (n == 0) {
                //Meter a flag
                SendSuccOnPred = true;
                close(predFD);
                predFD = -1;
            } else {

                buffer[n] = '\0'; //Experimentar

                write(1, buffer, n);

                command = strtok(buffer, " \t\n");
                arg_count = 0;
                char *token;
                while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < MAX_NODE_COUNT*3) {
                    arguments[arg_count] = token;
                    arg_count++;
                }
                arguments[arg_count] = NULL;

                //Processar comando no futuro

            }
            
        }

    }    


    freeaddrinfo(TEJO_res);
    free(IP);
    free(TCP);
    free(regIP);
    free(regUDP);
    close(fd_TCP);
    close(fd_UDP);
}