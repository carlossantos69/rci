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
    int fd_TCP, fd_UDP, fd_succ, newfd; //File Descriptors
    int errcode, maxfd, counter, arg_count, nodes_number; //Auxiliary variables
    char *IP, *TCP, *regIP, *regUDP;
    char input[MAX_COMMAND_SIZE];
    char buffer[BUFFER_SIZE]; //Temporary read buffers
    char *command;
    char succID[3], succIP[16], succTCP[6]; //Info of Sucessor
    char *arguments[MAX_NODE_COUNT*3];
    ssize_t n;
    struct addrinfo hints, *res, *TEJO_res, *succ_res;
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
            //printf("User command: %s\n", input);

            //Process command and divide into arguments
            command = strtok(input, " \t\n");
            arg_count = 0;
            char *token;
            while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < MAX_NODE_COUNT*3) {
                arguments[arg_count] = token;
                arg_count++;
            }
            arguments[arg_count] = NULL;

            if (strcmp(command,"leave") == 0 || strcmp(command, "l") == 0) { //leave (l)
            
                if (arg_count == 0 && registered) {
                    leave_command(ring, id, fd_UDP, TEJO_res , IP, TCP);
                    addrlen = sizeof(addr);
                    n = recvfrom(fd_UDP, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
                    if (n == -1) {
                        printf("Erro a ler do socket UDP\n");
                        exit(1);
                    }

                    if (memcmp(buffer, "OKUNREG", strlen("OKUNREG"))==0) {
                        printf("SUCESS\n");
                            registered = false;
                    } else {
                        printf("erro\n");
                    }
                   
                } else {
                    printf("Nó não registado ou erro de syntax\n");
                }
            }

            if (strcmp(command,"exit") == 0 || strcmp(command, "x") == 0) {
                break;
            }


            if (strcmp(command,"join") == 0 || strcmp(command, "j") == 0 ) { //join (j) ring id
                if (arg_count == 2 && strlen(arguments[0]) == 3 && strlen(arguments[1]) == 2 ) {
                    if (registered) {
                        printf("Nó já registado\n");
                    } else {
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
                            } else { // first node succ is himself
                                strcpy(succID, id);
                                strcpy(succIP, IP);
                                strcpy(succTCP, TCP);
                            }
                        }


                        //Entrar no anél e mandar comando para sucessor
                        if(strcmp(succIP, IP) == 0 && strcmp(succTCP, TCP) == 0){
                            printf("Primeiro nó a juntar-se.\n");
                        } else {
                            fd_succ = socket(AF_INET, SOCK_STREAM, 0);
                            if(fd_succ == -1){
                                printf("Erro a criar a socket TCP\n");
                                exit(1);
                            }


                            errcode = getaddrinfo(succIP, succTCP, &hints, &succ_res);
                            if(errcode != 0) {
                                printf("Error searching sucessor IP\n");
                                exit(1);
                            }

                            n = connect(fd_succ, succ_res->ai_addr,  succ_res->ai_addrlen);
                            if(n==-1) {
                                printf("Erro a estabelecer ligação");
                                exit(1);
                            }
                            entry_command(fd_succ,id, IP, TCP);
                        }

                        //Registar no anél
                        reg_node(fd_UDP, TEJO_res , ring, id, IP, TCP);
                        addrlen = sizeof(addr);
                        n = recvfrom(fd_UDP, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
                        if (n == -1) {
                            printf("Erro a ler do socket UDP\n");
                            exit(1);
                        }

                        if(memcmp(buffer, "OKREG", strlen("OKREG"))==0){
                            printf("SUCESS\n");
                            registered = true;
                        }else{
                            printf("erro\n");
                            exit(1);
                        }



                    }
                } else {
                    printf("Sintax error\n");
                }
            }

            if (strcmp(command, "dj") == 0) {
                if (arg_count == 4 && strlen(arguments[0]) == 2 && strlen(arguments[1]) == 2 && strlen(arguments[3]) == 5) {
                    strcpy(id, arguments[0]);
                    strcpy(succID, arguments[1]);
                    strcpy(succIP, arguments[2]);
                    strcpy(succTCP, arguments[3]);


                    if(strcmp(succIP, IP) == 0 && strcmp(succTCP, TCP) == 0){
                        printf("Primeiro nó a juntar-se, não se conecta a si mesmo.\n");
                    }

                    errcode = getaddrinfo(succIP, succTCP, &hints, &succ_res);
                    if (errcode !=0) {
                        printf("Erro a procurar o sucessor\n");
                        exit(1);
                    }

                    fd_succ = socket(AF_INET, SOCK_STREAM, 0);
                    if (fd_succ == -1) {
                        printf("Erro a criar a socket TCP\n");
                        exit(1);
                    }

                    n = connect(fd_succ, res->ai_addr,  res->ai_addrlen);
                    if(n==-1) {
                        printf("Erro a estabelecer ligação");
                        exit(1);
                    }

                    entry_command(fd_succ,succID, succIP, succTCP);

                    addrlen = sizeof(addr);
                    if ((newfd = accept(fd_TCP, (struct sockaddr*) &addr, &addrlen)) == -1) {
                        printf("Error accepting TCP connection\n");
                        exit(1);
                    }
                    //Esperar pela mensagem SUCC PRED ENTRY CHORD
                    //Dar set como File Descriptor do antecessor sucessor chord etc
                    n = read(newfd, buffer, BUFFER_SIZE);
                    if (n== -1) {
                        printf("Error reading TCP message\n");
                        exit(1);
                    }
                    printf("Received via TCP newfd\n");
                    write(1, buffer, n);

                } else {
                    printf("Sintax error\n"); 
                }
            }
            

            if (strcmp(command,"direct") == 0) {
                if (arg_count == 5) {
                    if (strcmp(arguments[0],"join") == 0 && strlen(arguments[1]) == 2 && strlen(arguments[2]) == 2 && strlen(arguments[4]) == 5) {
                        strcpy(id, arguments[1]);
                        strcpy(succID, arguments[2]);
                        strcpy(succIP, arguments[3]);
                        strcpy(succTCP, arguments[4]);

                        //if(strcmp(succIP, IP) == 0 && strcmp(succTCP, TCP) == 0){
                        //printf("Primeiro nó a juntar-se, não se conecta a si mesmo.\n");
                        //}
                    }

                    errcode = getaddrinfo(succIP, succTCP, &hints, &succ_res);
                    if(errcode !=0){
                        printf("Erro a procurar o sucessor\n");
                        exit(1);
                    }

                    fd_succ = socket(AF_INET, SOCK_STREAM, 0);
                    if(fd_succ == -1){
                        printf("Erro a criar a socket TCP\n");
                        exit(1);
                    }

                    n = connect(fd_succ, res->ai_addr,  res->ai_addrlen);
                    if(n==-1) {
                        printf("Erro a estabelecer ligação");
                        exit(1);
                    }

                    entry_command(fd_succ,succID, succIP, succTCP);
                    
                } else {
                    printf("Sintax error\n"); 
                }
            }


            if (strcmp(command, "show") == 0 && strcmp(arguments[0], "topology")== 0 ) {
                if(registered){
                    if(strcmp(id,succID) == 0 && strcmp(IP,succIP)==0 && strcmp(TCP,succTCP)==0){
                        printf("Nó %s regsitado com ip: %s  e na porta: %s\n", id ,IP, TCP);
                        printf("Nó sem sucessor, porque é o primeiro nó\n");
                    }else{
                        printf("O nó %s tem ip: %s  e está na porta: %s\n", id ,IP, TCP);
                        printf("O seu sucessor %s tem ip: %s e está na porta: %s\n", succID, succIP, succTCP);
                    }
                }
                else{
                    printf("Nó não registado\n");
                }
                    
            }

            if(strcmp(command, "st")== 0){          
                if(registered){
                    printf("O nó %s tem ip: %s  e está na porta: %s\n", id ,IP, TCP);
                    printf("O seu sucessor %s tem ip: %s e está na porta: %s\n", succID, succIP, succTCP);                    
                }
                else{
                    printf("Nó não registado\n");
                }
            }







        }
        if (FD_ISSET(fd_TCP,&read_fds)) {
            printf("Socket TCP Preparado\n");

            addrlen = sizeof(addr);
            if ((newfd = accept(fd_TCP, (struct sockaddr*) &addr, &addrlen)) == -1) {
                printf("Error accepting TCP connection\n");
                exit(1);
            }

            //Esperar pela mensagem SUCC PRED ENTRY CHORD
            //Dar set como File Descriptor do antecessor sucessor chord etc

            n = read(newfd, buffer, BUFFER_SIZE);
            if (n== -1) {
                printf("Error reading TCP message\n");
                exit(1);
            }
            buffer[n] = '\0'; //Experimentar

            printf("Received via TCP newfd\n");
            write(1, buffer, n);

            command = strtok(buffer, " \t\n");
            arg_count = 0;
            char *token;
            while ((token = strtok(NULL, " \t\n")) != NULL && arg_count < MAX_NODE_COUNT*3) {
                arguments[arg_count] = token;
                arg_count++;
            }
            arguments[arg_count] = NULL;

            if(strcmp(command, "ENTRY") == 0){
                strcpy(succID,arguments[0]);
                strcpy(succIP,arguments[1]);
                strcpy(succTCP,arguments[2]);  

                fd_succ = socket(AF_INET, SOCK_STREAM, 0);
                if(fd_succ == -1){
                    printf("Erro a criar a socket TCP\n");
                    exit(1);
                }

                errcode = getaddrinfo(succIP, succTCP, &hints, &succ_res);
                if(errcode != 0) {
                    printf("Error searching sucessor IP\n");
                    exit(1);
                }

                n = connect(fd_succ, succ_res->ai_addr,  succ_res->ai_addrlen);
                if(n==-1) {
                    printf("Erro a estabelecer ligação");
                    exit(1);
                }

                pred_command(fd_succ, id);
                succ_command(fd_succ, succID, succIP, succTCP);

            }

        }


        if (FD_ISSET(fd_UDP,&read_fds)) {
            //printf("Socket UDP Preparado\n");

            //TODO: CHECKAR SE VEM MESAGEM DO TEJO
            //Nunca é suposto vir nada

            //write(1, buffer, n);
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