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
#include "route.h"

#define MAX(A,B) ((A)>=(B)?(A):(B))

#define TABLE_SIZE 100

//Servidor TEJO
#define TEJO_IP "193.136.138.142"
#define TEJO_UDP "59000"

#define MAX_COMMAND_SIZE 150
#define MAX_ARGUMENT_COUNT 10
#define MAX_NODE_COUNT 100

#define BUFFER_SIZE 50000



int main(int argc, char *argv[]) {
    bool registado = false;
    bool SendSuccOnPred = false; 
    bool inRing = false;
    int fd_TCP, fd_UDP, fd; //File Descriptors
    int errcode, maxfd, counter, arg_count;
    char* forwarding_table[TABLE_SIZE][TABLE_SIZE];
    char* shortest_table[TABLE_SIZE];
    char* expedition_table[TABLE_SIZE];
    char* lineBreak;

    char ID[3];
    char *IP, *TCP, *regIP, *regUDP;
    char succID[3], succIP[16], succTCP[6]; //Info of Sucessor    
    char second_succID[3], second_succIP[16], second_succTCP[6]; //Info of Sucessor
    char nextID[3]; // next ID to send message
    char predID[3];
    int succFD = -1, predFD = -1, nextFD = -1;//Descritores

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

    for (int i=0; i<TABLE_SIZE; ++i) {
        for(int j=0; j<TABLE_SIZE; ++j) {
            forwarding_table[i][j] = NULL;
        }
            shortest_table[i] = NULL;
            expedition_table[i] = NULL;
    }


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
        printf("Erro a dar bind TCP\n");
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
            memset(buffer, 0, sizeof(buffer));
            fgets(input, sizeof(input), stdin);

            parse_input(input, &command, arguments, &arg_count);

            if (strcmp(command,"leave") == 0 || strcmp(command, "l") == 0) { //leave (l)
                if (arg_count == 0 && inRing) {
                    if (predFD != -1) {
                        close(predFD);
                    }
                    predFD = -1;
                    if (succFD != -1) {
                        close(succFD);
                    }
                    succFD = -1;

                    if (registado) {
                        unreg_node(fd_UDP, TEJO_res, ring, ID);
                        addrlen = sizeof(addr);
                        int n = recvfrom(fd_UDP, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
                        if (n == -1) {
                            printf("Erro a ler do socket UDP\n");
                            exit(1);
                        }
                        
                    }

                    inRing = false;
                    registado = false;
                    //freeTables(forwarding_table, shortest_table, expedition_table);
                    
                    printf("Nó saiu do anel\n");
                   
                } else {
                    printf("Nó não registado ou erro de syntax\n");
                }
            }
            else if (strcmp(command,"exit") == 0 || strcmp(command, "x") == 0) {
                if (predFD != -1) {
                    close(predFD);
                }
                if (succFD != -1) {
                    close(succFD);
                }
                if (registado) {
                    unreg_node(fd_UDP, TEJO_res, ring, ID);
                }
                free(command);
                break;
            }
            else if (strcmp(command,"join") == 0 || strcmp(command, "j") == 0 ) { //join (j) ring id
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
                        //Entrar no anél registando se no servidor de nós
                        succFD = join_command(arguments, buffer, ring, fd_UDP, TEJO_res, ID, IP, TCP, succID, succIP, succTCP, second_succID, second_succIP, second_succTCP, predID, &registado);

                        inRing = true;

                        int i= atoi(ID);

                        shortest_table[i] = ID;
                    }
                } else {
                    printf("Sintax error\n");
                }
            }
            else if (strcmp(command, "dj") == 0) {
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

                        int i= atoi(ID);
                        shortest_table[i] = ID;

                    }

                } else {
                    printf("Sintax error\n"); 
                }
            }            

            else if (strcmp(command,"direct") == 0) {
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

                            int i= atoi(ID);
                            shortest_table[i] = ID;

                        }
                    }
                    
                } else {
                    printf("Sintax error\n"); 
                }
            }


            else if (strcmp(command, "show") == 0 && strcmp(arguments[0], "topology")== 0 ) {                  
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

            else if (strcmp(command, "show") == 0 && strcmp(arguments[0], "routing")== 0 ){
                if (arg_count == 2 && strlen(arguments[1]) == 2) {
                    print_forwardingTable(forwarding_table, arguments[1]);
                    } else {
                            printf("Syntax error: show routing (st) dest\n");
                }
            }

            else if (strcmp(command, "show") == 0 && strcmp(arguments[0], "path") == 0){
                if (arg_count == 2 && strlen(arguments[1]) == 2) {
                    print_shortestTable(shortest_table, arguments[1]);
                } else {
                    printf("Syntax error: show path (sp) dest\n");
                }
            }

            else if (strcmp(command, "show") == 0 && strcmp(arguments[0], "forwarding") == 0) {
                if (arg_count == 1) {
                    print_expeditionTable(expedition_table);
                } else {
                    printf("Syntax error: show forwarding (sf)\n");
                }
            }

            else if(strcmp(command, "st")== 0){       
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
            else if (strcmp(command, "sr") == 0) { // Show Routing
                if (arg_count == 1 && strlen(arguments[0]) == 2) {
                    print_forwardingTable(forwarding_table, arguments[0]);
                } else {
                    printf("Syntax error: show routing (sr) dest\n");
                }
            } else if (strcmp(command, "sp") == 0) { // Show Path
                if (arg_count == 1 && strlen(arguments[0]) == 2) {
                    print_shortestTable(shortest_table, arguments[0]);
                } else {
                    printf("Syntax error: show path (sp) dest\n");
                }
            } else if (strcmp(command, "sf") == 0) { // Show Forwarding
                print_expeditionTable(expedition_table);
            } else if(strcmp(command, "message") == 0 || strcmp(command, "m") == 0){
                    if(inRing){
                        if(strcmp(arguments[0], ID) !=0){
                            if(strlen(arguments[1]) < 128){
                                //printf("STRING LENGHT IS %ld", strlen(arguments[1]));
                                if(strcmp(searchNextID(expedition_table, arguments[0]), "ERROR")){
                                strcpy(nextID,searchNextID(expedition_table, arguments[0]));
                                    if(nextID != NULL){
                                        nextFD = find_socket_fd(nextID, predFD, predID, succFD, succID);
                                        if(nextFD != -1){
                                            if(send_chat_message(nextFD, ID, arguments[0], arguments[1]) > 1){
                                                printf("Sent message to %s\n", arguments[0]);
                                            }else{
                                                printf("Error sending message\n");
                                            }
                                        }
                                    }
                                }else{
                                    printf("Could not find node to send message\n");
                                }
                            }else{
                                printf("Message exceeds maximum size. Limit is 128 characters\n");
                            }
                        }else{
                            printf("Cannot send message to your own node\n");
                        }
                    }
            }

            else{
                printf("Syntax error or command not found\n");
                print_help();
            }
            
            free(command);
        }
        if (FD_ISSET(fd_TCP, &read_fds)) {
            memset(buffer, 0, sizeof(buffer));
            addrlen = sizeof(addr);
            if ((fd = accept(fd_TCP, (struct sockaddr*) &addr, &addrlen)) == -1) {
                printf("Error accepting TCP connection\n");
                exit(1);
            }

            n = read(fd, buffer, BUFFER_SIZE);
            buffer[n] = '\0';            
            if (n== -1) {
                printf("Error reading TCP message\n");
                exit(1);
            }

            //printf("Print do buffer: %s\n", buffer);


            char *lineBreak = strchr(buffer, '\n');
            while (lineBreak != NULL) {
                *lineBreak = '\0';

                //printf("Processing command via new connection: %s\n", buffer);

                parse_input(buffer, &command, arguments, &arg_count);

                if (strcmp(command, "ENTRY") == 0) {

                    if(strcmp(succID, ID) != 0){
                        if(!isConnected(predID,succID, NULL)){
                        removeColumn(forwarding_table, shortest_table, expedition_table, ID, predID, succFD, predFD);
                        }
                    }           

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
                        route_propagation(succFD, ID, shortest_table);

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
                        if(!isConnected(arguments[0],succID, NULL)){
                            removeColumn(forwarding_table, shortest_table, expedition_table, ID, arguments[0], succFD, -1);
                        }
                        close(predFD);
                        predFD = -1;
                    }
                    predFD = fd;
                    route_propagation(predFD, ID, shortest_table);
                }            

                if (strcmp(command, "PRED") == 0) {
                    if(!isConnected(predID, succID, NULL)){
                        removeColumn(forwarding_table, shortest_table, expedition_table, ID, predID, succFD, -1);
                    }
                    if (predFD != -1) { // Fechar caso haja, 2 nós
                        close(predFD);
                        predFD = -1;
                    }
                    predFD = fd;
                    strcpy(predID, arguments[0]);

                    if(SendSuccOnPred){
                        succ_command(predFD, succID, succIP, succTCP);
                        SendSuccOnPred = false; 
                    }
  

                    route_propagation(predFD, ID, shortest_table);             
                }
                
                if (strcmp(command, "ROUTE") == 0){
                    //printf("BUFFER DO ROUTE: %s", buffer);
                    if(strcmp(succID, ID) !=0){
                        if(RouteHandler(forwarding_table, shortest_table, expedition_table, buffer, ID)){
                        n = atoi(arguments[1]);
                        if(succFD != -1){
                            route_command(succFD, ID, arguments[1], shortest_table[n] );
                            //printf("Sent Route %s, %s, %s to sucessor\n", ID, arguments[1], shortest_table[n]);
                        }

                        if(predFD != -1){
                            route_command(predFD, ID, arguments[1], shortest_table[n] );
                            //printf("Sent Route %s, %s, %s to predecessor\n", ID, arguments[1], shortest_table[n]);
                        }
                    }
                    }
                }

                memmove(buffer, lineBreak + 1, strlen(lineBreak + 1) + 1);
                lineBreak = strchr(buffer, '\n');
            }

        

        }


        if (FD_ISSET(succFD, &read_fds)) {
            //printf("Recebido do sucessor\n");
            memset(buffer, 0, sizeof(buffer));
            n = read(succFD, buffer, BUFFER_SIZE);
            if (n == -1) {
                printf("Error reading TCP message sucessor\n");
                exit(1);
            } else if(n == 0) { // Sucessor saiu
                printf("Sucessor left\n");

                removeColumn(forwarding_table, shortest_table, expedition_table, ID, arguments[0], -1, -1);
                if(predFD != -1){
                    route_command(predFD, ID, succID, NULL);
                }
                removeColumn(forwarding_table, shortest_table, expedition_table, ID, succID, -1, predFD);

                strcpy(succID, second_succID);
                strcpy(succIP, second_succIP);
                strcpy(succTCP, second_succTCP);

                if (close(succFD) == -1) {
                    printf("Error closing connection to sucessor\n");
                    exit(1);
                }
                succFD = -1;


                if(strcmp(succID, ID) != 0){
                    if(predFD != -1){
                        succ_command(predFD, succID, succIP, succTCP);
                    }

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
                    route_propagation(succFD, ID, shortest_table);
                }else if(strcmp(succID, ID) == 0){

                    forwarding_tableChange(forwarding_table, predID, predID, NULL);
                    if (refreshShortestTable(forwarding_table, shortest_table, predID)) {
                        refreshExpeditionTable(shortest_table, expedition_table, predID);
                    }
                    strcpy(predID, succID);
                }

            } else {
                //printf("DEBUG: %s\n", buffer);
                buffer[n] = '\0';
                lineBreak = strchr(buffer, '\n');
                while (lineBreak != NULL) {
                    *lineBreak = '\0';

                    //printf("Processing command via sucessor: %s\n", buffer);

                    parse_input(buffer, &command, arguments, &arg_count);

                    //printf("COMANDO %s\n", command);

                    if(strcmp(command, "CHAT") == 0){
                        if(strcmp(ID, arguments[1]) == 0){
                            printf("Received a message from %s: %s\n", arguments[0], arguments[2]);
                        }
                        else{
                            strcpy(nextID ,searchNextID(expedition_table, arguments[1]));
                            if(nextID != NULL){
                                nextFD = find_socket_fd(nextID, predFD, predID, succFD, succID);
                                if(nextFD != -1){
                                    if(send_chat_message(nextFD, arguments[0], arguments[1], arguments[2]) > 1){
                                        //printf("Sent message to %s\n", arguments[0]);
                                    }else{
                                        printf("Error sending message\n");
                                    }
                                }
                            }                            
                        }
                    }
                    if(strcmp(command, "SUCC") == 0){
                        strcpy(second_succID, arguments[0]);
                        strcpy(second_succIP, arguments[1]);
                        strcpy(second_succTCP, arguments[2]);
                    }

                    if(strcmp(command, "ENTRY") == 0) { //Entry vindo do sucessor

                        if(predFD != -1  && strcmp(succID, ID) != 0 && strcmp(second_succID, ID) != 0){
                            route_command(predFD, ID, succID, NULL);
                        }
                        
                        if (succFD != -1) {   
                            if(!isConnected(succID, NULL, predID)){
                                removeColumn(forwarding_table, shortest_table, expedition_table, ID, succID, -1, predFD);
                            }                   
                            close(succFD);
                            succFD = -1;
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

                        route_propagation(succFD, ID, shortest_table);
                        
                    }
                    if (strcmp(command,"ROUTE") == 0) { //Received route command
                        //TODO: finish code
                        //printf("BUFFER: %s", buffer);
                        if(strcmp(succID, ID) !=0){
                            if(isConnected(arguments[0], succID, predID)){
                                //printf("BUFFER DO ROUTE: %s\n", buffer);
                                if (RouteHandler(forwarding_table, shortest_table, expedition_table, buffer, ID)) {
                                    n = atoi(arguments[1]);
                                    if (predFD != -1) {
                                        //Send ROUTE to predecessor
                                        route_command(predFD, ID, arguments[1], shortest_table[n]);
                                        //printf("Sent Route %s, %s, %s to predecessor\n", ID, arguments[1], shortest_table[n]);
                                    }
                                    if (succFD != -1) {
                                        //Send ROUTE to sucessor
                                        route_command(succFD, ID, arguments[1], shortest_table[n]);
                                        //printf("Sent Route %s, %s, %s to sucessor\n", ID, arguments[1], shortest_table[n]);
                                    }
                                    //TODO: send to chords
                                }
                            }
                        }

                    }

                    // Find next line break
                    memmove(buffer, lineBreak + 1, strlen(lineBreak + 1) + 1);
                    lineBreak = strchr(buffer, '\n');
                }
                

        }
        }

        if (FD_ISSET(predFD, &read_fds)) {
            //printf("Recebido do predecessor\n");
            memset(buffer, 0, sizeof(buffer));
            n = read(predFD, buffer, BUFFER_SIZE);
            if (n == -1) {
                printf("Error reading TCP message\n");
                exit(1);
            } else if (n == 0) {
                printf("Predecessor left\n");



                //Meter a flag
                if(succFD != -1){
                    route_command(succFD, ID, predID, NULL);
                }

                for (int i=0; i<TABLE_SIZE; ++i) {
                    if (forwarding_table[atoi(predID)][i] != NULL) {
                        free(forwarding_table[atoi(predID)][i]);
                        forwarding_table[atoi(predID)][i] = NULL;
                    }
                }

                removeColumn(forwarding_table, shortest_table, expedition_table, ID, predID, succFD, -1);

                SendSuccOnPred = true;
                if(close(predFD) == -1){
                    printf("Error closing predecessor connection\n");
                    exit(1);
                }
                predFD = -1;
                
            } else {
                
                buffer[n] = '\0';
                lineBreak = strchr(buffer, '\n');
                
                while (lineBreak != NULL) {
                    *lineBreak = '\0';

                    //printf("Processing command via predecessor: %s\n", buffer);

                    parse_input(buffer, &command, arguments, &arg_count);                        



                    //Processar comando no futuro
                    if (strcmp(command,"ROUTE") == 0) { //Received route command
                        //TODO: finish code
                        //printf("BUFFER: %s", buffer);
                        if(strcmp(succID, ID) !=0){
                            if(isConnected(arguments[0], succID, predID)){ 
                            //printf("BUFFER DO ROUTE: %s\n", buffer);
                                if (RouteHandler(forwarding_table, shortest_table, expedition_table, buffer, ID)) {
                                    n = atoi(arguments[1]);
                                    if (predFD != -1) {
                                        //Send ROUTE to predecessor
                                        route_command(predFD, ID, arguments[1], shortest_table[n]);
                                        //printf("Sent Route %s, %s, %s to predecessor\n", ID, arguments[1], shortest_table[n]);
                                    }
                                    if (succFD != -1) {
                                        //Send ROUTE to sucessor
                                        route_command(succFD, ID, arguments[1], shortest_table[n]);
                                        //printf("Sent Route %s, %s, %s to sucessor\n", ID, arguments[1], shortest_table[n]);
                                    }
                                    //TODO: send to chords
                                }
                            }
                        }
                        

                    }

                    if(strcmp(command, "CHAT") == 0){
                        if(strcmp(ID, arguments[1]) == 0){
                            printf("Received a message from %s: %s\n", arguments[0], arguments[2]);
                        }
                        else{
                            strcpy(nextID ,searchNextID(expedition_table, arguments[1]));
                            if(nextID != NULL){
                                nextFD = find_socket_fd(nextID, predFD, predID, succFD, succID);
                                if(nextFD != -1){
                                    if(send_chat_message(nextFD, arguments[0], arguments[1], arguments[2]) > 1){
                                        //printf("Sent message to %s\n", arguments[0]);
                                    }else{
                                        printf("Error sending message\n");
                                    }
                                }
                            }                            
                        }
                    }
                    memmove(buffer, lineBreak + 1, strlen(lineBreak + 1) + 1);
                    // Find next line break
                    lineBreak = strchr(buffer + 1, '\n');
                }
            
        }

    }    


}
    freeaddrinfo(TEJO_res);
    //freeTables(forwarding_table, shortest_table, expedition_table);
    free(IP);
    free(TCP);
    free(regIP);
    free(regUDP);
    close(fd_TCP);
    close(fd_UDP);
}