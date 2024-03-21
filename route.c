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

int RouteHandler(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* command, char* destination){
    char* origin, *final_dest, *path, *new_path;
    bool is_valid_command = true;
    bool has_changes = false;

    //printf("COMANDO: %s", command);

    // Extracting the command details
    char* token = strtok(command, " ");
    token = strtok(NULL, " ");
    origin = strdup(token);
    token = strtok(NULL, " ");
    final_dest = strdup(token);
    token = strtok(NULL, " ");
    path = strdup(token);

    // printf("Origin: %s | Final Destination: %s | Destination: %s\n", origin, final_dest, destination);
    // printf("Path: %s\n", path);

    // Checking command validity
    if (strncmp(origin, path, 2) != 0) {
        is_valid_command = false;
        printf("Invalid ROUTE command. Origin mismatch. Ignoring.\n");
    }
    if (final_dest[0] != path[strlen(path) - 2] || final_dest[1] != path[strlen(path) -1]) {
        is_valid_command = false;
        printf("Invalid ROUTE command. Final destination mismatch with. Ignoring.\n");
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
        printf("Valid route.\n");

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
        printf("Invalid route.\n");
    }

    free(origin);
    free(final_dest);
    free(path);
    return has_changes;
}


int refreshShortestTable(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* index) {
    int i = atoi(index);
    bool updated = false;
    bool forwarding_tableNULL = true;
    char* bestPath = NULL;
    size_t minSize = 0;

    if (shortest_table[i] == NULL) { // Special case: shortest path table is empty -> Find first non-null entry in forward table
        for (int j = 0; j < TABLE_SIZE; ++j) {
            if (forwarding_table[i][j] != NULL) {
                shortest_tableChange(shortest_table, index, forwarding_table[i][j]);
                updated = true;
                //printf("PRINT TABELA: %s\n", forwarding_table[i][j]);
                break;
            }
        }
        if (updated) {
            return 1;
        }
    }

    bestPath = strdup(shortest_table[i]);
    if (bestPath == NULL) {
        // Handle memory allocation failure
        return -1;
    }
    minSize = strlen(bestPath);

    for (int j = 0; j < TABLE_SIZE; ++j) {
        if (forwarding_table[i][j] != NULL) {
            forwarding_tableNULL = false;
            size_t pathLength = strlen(forwarding_table[i][j]);
            if (pathLength < minSize) {
                minSize = pathLength;
                strcpy(bestPath, forwarding_table[i][j]);
                updated = true;
            }
        }
    }

    if (forwarding_tableNULL) { // Special Case: forward table line is all NULL, clear shortest_table
        shortest_tableChange(shortest_table, index, NULL);
        free(bestPath);
        return 1;
    }

    if (updated) {
        shortest_tableChange(shortest_table, index, bestPath);
        free(bestPath);
        return 1;
    } else {
        free(bestPath);
        return 0;
    }
}


void refreshExpeditionTable(char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* index) {
    int i = atoi(index);

    if (strlen(shortest_table[i]) == 2) {
        expedition_tableChange(expedition_table, index, NULL);
    } else {
        // Parse the shortest path to find the last destination
        char* path = shortest_table[i];
        
        char dest[3];
        dest[0] = path[3];
        dest[1] = path[4];
        dest[2] = '\0';

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
    }

    forwarding_table[i][j] = strdup(input);
    if (forwarding_table[i][j] == NULL) {
        printf("Memory allocation error\n");
        exit(1);
    }

    printf("Changed forwarding table\n");
}

// Change entry in shortest Path Table given index and new text
// If input is null, entry is cleared
void shortest_tableChange(char* shortest_table[TABLE_SIZE], char* index, char* input) {
    int i = atoi(index);

    printf("PRINTING INPUT: %s\n", input);

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
    printf("Changed shortest path table\n");
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
    printf("Changed expedition table\n");
}

void route_propagation(int fd, char* source, char* shortest_path[TABLE_SIZE]) {
    char destination[4]; // Allocate space for 3 digits and null terminator

    printf("Broadcasting routes\n");

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
            printf("SENDING ROUTE %s %s %s\n", source, destination, shortest_path[i]);
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



