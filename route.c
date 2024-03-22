#include "route.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "tcp.h"

#define TABLE_SIZE 100
#define MAX_NODE_COUNT 100

int RouteHandler(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* command, char* destination){
    char* origin, *final_dest, *path, *new_path;
    bool is_valid_command = true;
    bool has_changes = false;

    //printf("COMANDO: %s", command);
    int n = countElements(command);

    if(n==2){
        char* token = strtok(command, " ");
        token = strtok(NULL, " ");
        origin = strdup(token);
        token = strtok(NULL, " ");
        final_dest = strdup(token);

        forwarding_tableChange(forwarding_table, final_dest, origin, NULL);
        if(refreshShortestTable(forwarding_table, shortest_table, final_dest)){
            has_changes = true;
            refreshExpeditionTable(shortest_table, expedition_table, final_dest);
        }
        return has_changes;
    }else if(n ==3){
        char* token = strtok(command, " ");
        token = strtok(NULL, " ");
        origin = strdup(token);
        token = strtok(NULL, " ");
        final_dest = strdup(token);
        token = strtok(NULL, " ");
        path = strdup(token);

        if (strncmp(origin, path, 2) != 0) {
            is_valid_command = false;
            //printf("Invalid ROUTE command. Origin mismatch. Ignoring.\n");
        }
        if (final_dest[0] != path[strlen(path) - 2] || final_dest[1] != path[strlen(path) -1]) {
            is_valid_command = false;
            //printf("Invalid ROUTE command. Final destination mismatch with. Ignoring.\n");
        }
        
        char original_path[strlen(path) + 1]; // Create a copy of the original path
        strcpy(original_path, path);

        // Checking if the given destination is not already in the path
        token = strtok(path, "-");
        while (token != NULL) {
            //printf("Comparing %s : %s\n", token, destination);
            if (strcmp(token, destination) == 0) {
                is_valid_command = false;
                break;
            }
            token = strtok(NULL, "-");
        }

        strcpy(path, original_path);

        // Processing valid command
        if (is_valid_command) {
            //printf("Valid route.\n");

            // Constructing new path
            new_path = (char*)malloc(strlen(destination) + strlen("-") + strlen(path) + 2);
            if (new_path == NULL) {
                printf("Memory allocation error for path");
                exit(1);
            }
            strcpy(new_path, destination);
            strcat(new_path, "-");
            strcat(new_path, path);

            forwarding_tableChange(forwarding_table, final_dest, origin, new_path);
            if (refreshShortestTable(forwarding_table, shortest_table, final_dest)) {
                has_changes = true;
                refreshExpeditionTable(shortest_table, expedition_table, final_dest);
            }
            free(new_path);
        } else {
            //printf("Invalid route.\n");
        }
    }
    // Extracting the command details

    // printf("Origin: %s | Final Destination: %s | Destination: %s\n", origin, final_dest, destination);
    // printf("Path: %s\n", path);

    // Checking command validity

    free(origin);
    free(final_dest);
    free(path);
    return has_changes;
}

//TODO - Atualizar esta função
int refreshShortestTable(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* index) {
    int index_int = atoi(index);
    bool updated = false;
    int minSize = 0;
    int best_pos;
    
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        if(forwarding_table[index_int][i] != NULL){
            if((strlen(forwarding_table[index_int][i]) < minSize) || minSize == 0){
                minSize = strlen(forwarding_table[index_int][i]);
                best_pos = i; 
            }
        }
    }


    if (minSize == 0) { //Line is all empty, send message with NULL example: ROUTE 30 15<LF>
        if (shortest_table[index_int] != NULL) {
            shortest_tableChange(shortest_table, index, NULL);
            updated = true;
            //printf("UPDATED THIS FUCKING TABLE\n");
        }
    } else {
        if (shortest_table[index_int] == NULL) {
            shortest_tableChange(shortest_table, index, forwarding_table[index_int][best_pos]);
            updated = true;
        } else {
            if (strcmp(shortest_table[best_pos], forwarding_table[index_int][best_pos]) != 0) { //Only change if its new best path
                shortest_tableChange(shortest_table, index, forwarding_table[index_int][best_pos]);
                updated = true;
            }
        }
    }

    return updated; 
    
}


void refreshExpeditionTable(char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* index) {
    int i = atoi(index);
    
    if(shortest_table[i] == NULL){
        expedition_tableChange(expedition_table, index, NULL);
    }
    else if (strlen(shortest_table[i]) == 2) {
        expedition_tableChange(expedition_table, index, NULL);
    } else {
        // Parse the shortest path to find the last destination
        char* path = shortest_table[i];
        
        char dest[3];
        dest[0] = path[3];
        dest[1] = path[4];
        dest[3] = '\0';

        //printf("Last destination: %s\n", dest);

        // Update the expedition_table with the last destination
        expedition_tableChange(expedition_table, index, dest);
    }
}

// Change entry in Forwarding Table given indexes and new text
void forwarding_tableChange(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* index1, char* index2, char* input) {
    int i = atoi(index1);
    int j = atoi(index2);

    if (forwarding_table[i][j] != NULL) {
        free(forwarding_table[i][j]);
        forwarding_table[i][j] = NULL;
    }
    
    if(input != NULL){
        forwarding_table[i][j] = (char*) malloc(strlen(input) + 1);
        if (forwarding_table[i][j] == NULL) {
            printf("Memory allocation error\n");
            exit(1);
        }
        strcpy(forwarding_table[i][j], input);
    }


    //printf("Changed forwarding table\n");
}

// Change entry in shortest Path Table given index and new text
// If input is null, entry is cleared
void shortest_tableChange(char* shortest_table[TABLE_SIZE], char* index, char* input) {
    int i = atoi(index);

    //printf("PRINTING INPUT: %s\n", input);

    if (shortest_table[i] != NULL) {
        free(shortest_table[i]);
    }

    if (input != NULL) {
        shortest_table[i] = strdup(input);
        if (shortest_table[i] == NULL) {
            printf("Memory allocation error\n");
            exit(1);
        }
    } else {
        shortest_table[i] = NULL;
    }
    //printf("Changed shortest path table\n");
}



// Change entry in expedition_table_table Table given index and new text
void expedition_tableChange(char* expedition_table[TABLE_SIZE], char* index, char* input) {
    int i = atoi(index);

    if (expedition_table[i] != NULL) {
        free(expedition_table[i]);
    }

    if (input != NULL) {
        // Allocate memory for expedition_table[i]
        expedition_table[i] = (char*)malloc(strlen(input) + 1); // +1 for null terminator

        if (expedition_table[i] != NULL) {
            // Copy input to expedition_table[i]
            strcpy(expedition_table[i], input);
        } else {
            // Handle memory allocation failure
            printf("Memory allocation error\n");
            exit(1);
        }
    } else {
        // Set expedition_table[i] to NULL if input is NULL
        expedition_table[i] = NULL;
    }
    //printf("Changed expedition table\n");
}

void route_propagation(int fd, char* source, char* shortest_path[TABLE_SIZE]) {
    char destination[4]; // Allocate space for 3 digits and null terminator

    //printf("Broadcasting routes\n");

    for (int i = 0; i < TABLE_SIZE; ++i) {
        if (shortest_path[i] != NULL) {
            // Convert integer index to string
            if (i < 10) {
                sprintf(destination, "0%d", i); // Pad with one leading zero
            } else {
                sprintf(destination, "%d", i); // No leading zeros needed
            }

            //printf("Destination: %s\n", destination);
            // Route to destination
            route_command(fd, source, destination, shortest_path[i]);
            // TODO: Send to chords

            // Print the route being broadcasted
            //printf("SENDING ROUTE %s %s %s\n", source, destination, shortest_path[i]);
        }
    }

}

void freeTables(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE]) {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        for (int j = 0; j < TABLE_SIZE; ++j) {
            if (forwarding_table[i][j] != NULL) {
                free(forwarding_table[i][j]);
            }
        }
        if (shortest_table[i] != NULL) {
            free(shortest_table[i]);
        }
        if (expedition_table[i] != NULL) {
            free(expedition_table[i]);
        }
    }
}

int isConnected(char* destination_id, char* successor_id, char* predecessor_id) {
    // Check if the destination ID matches either the successor ID or the predecessor ID
    if (successor_id && strcmp(destination_id, successor_id) == 0) {
        return 1; // Connected to successor
    }
    if (predecessor_id && strcmp(destination_id, predecessor_id) == 0) {
        return 1; // Connected to predecessor
    }
    return 0; // Not connected to successor or predecessor
}

void removeColumn(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* myid, char* close_id, int succ_fd, int pred_fd) {
    int close_id_int = atoi(close_id);
    char line[3];
    // Remove the whole column of entries related to close_id (= NULL) from forwarding_table
    for (int i = 0; i < TABLE_SIZE; ++i) {
        if(forwarding_table[i][close_id_int] != NULL){
            sprintf(line, "%d", i);
            if (strlen(line) == 1) {
                char temp[3];
                temp[0] = '0';
                temp[1] = line[0];
                temp[2] = '\0';

                strcpy(line, temp);
            }

            //printf("DEBUG LINE: %s\n", line);
            forwarding_tableChange(forwarding_table, line, close_id, NULL);
            // Refresh shortest_table and expedition_table
            if (refreshShortestTable(forwarding_table, shortest_table, close_id) == 1) {
                refreshExpeditionTable(shortest_table, expedition_table, close_id);
                if(shortest_table[i] != NULL){
                    if (pred_fd != -1) {
                        route_command(pred_fd, myid, line, shortest_table[i]);
                    }
                    if (succ_fd != -1) {
                        route_command(succ_fd, myid, line, shortest_table[i]);
                    }
                }
            }
        }
    }

}


char* searchNextID(char* expeditionTable[TABLE_SIZE], char* dest){
    int id = atoi(dest);
    
    if(expeditionTable[id] != NULL){
        return(expeditionTable[id]);
    }else{
        return("ERROR");
    }
}

int find_socket_fd(char* destination_ID, int predecessor_fd, char* predecessor_ID, int successor_fd, char* successor_ID) {
    if (strcmp(destination_ID, predecessor_ID) == 0) {
        return predecessor_fd;
    }
    if (strcmp(destination_ID, successor_ID) == 0) {
        return successor_fd;
    }
    return -1; // ID not found 
}

void parse_input(char *input, char **command, char *arguments[], int *arg_count) {
    char *token;
    char *temp_input = strdup(input);

    token = strtok(input, " \t\n");
    *command = strdup(token);

    *arg_count = 0;

    if (strcmp(*command, "message") == 0 || strcmp(*command, "m") == 0) {
        // For "message" command, store the first token as arguments[0]
        token = strtok(NULL, " \t\n");
        if (token != NULL) {
            arguments[0] = strdup(token);
            // Concatenate everything else as arguments[1]
            token = strtok(NULL, "\n");
            if (token != NULL) {
                arguments[1] = strdup(token);
                *arg_count = 2;
            }
        }
    } else if(strcmp(*command, "CHAT") == 0){
        // For "CHAT" command, store the first two tokens as arguments[0] and arguments[1]
        token = strtok(NULL, " \t\n");
        if (token != NULL) {
            arguments[0] = strdup(token);
            token = strtok(NULL, " \t\n");
            if (token != NULL) {
                arguments[1] = strdup(token);
                // Concatenate everything else as arguments[2]
                token = strtok(NULL, "\n");
                if (token != NULL) {
                    arguments[2] = strdup(token);
                    *arg_count = 3;
                }
            }
        }
    }   
    else {
        // For other commands, tokenize normally
        while ((token = strtok(NULL, " \t\n")) != NULL && *arg_count < MAX_NODE_COUNT * 3) {
            arguments[*arg_count] = strdup(token);
            (*arg_count)++;
        }
        arguments[*arg_count] = NULL;
    }

    strcpy(input, temp_input);
    free(temp_input);
}









