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

#define TABLE_SIZE 100

int RouteHandler(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* buffer, char* j){

    char* token, *i, *n, *path, *result;
    bool isValid = true;
    bool isChanged = false;


    token = strtok(buffer, " ");
    token = strtok(NULL, " ");

    //Divide i n, 𝜙 in different strings
    i = strdup(token);
    token = strtok(NULL, " ");
    n = strdup(token);
    token = strtok(NULL, " ");
    path = strdup(token);

    printf("Debug: i:%s n:%s j:%s\n", i, n, j);
    printf("DEBUG: Path: %s\n", path);

    if (strncmp(i, path, 2) != 0) {
        isValid = false;
        printf(" [ERROR]: Invalid ROUTE command, i != path start, ignoring\n");
    }
    if (n[0] != path[strlen(path)-2] || n[1] != path[strlen(path)-1]) {
        isValid = false;
        printf(" [ERROR]: Invalid ROUTE command, n != path end, ignoring\n");
    }
    token = strtok(token, "-");
    while (token != NULL) {
        // Compare each part with the given string
        printf("Comparing %s : %s\n", token, j);
        if (strcmp(token, j) == 0) {
            isValid = false;
            break;
        }
        token = strtok(NULL, "-");
    }

    
    //Se j não é um nó de sigma, caminho de j para n atravez de i é: concatenacao de j com sigma, j-sigma
    if (isValid) {
        printf("Debug: Valid path\n");

        //concatenação de 𝑗 com 𝜙 (𝑗−𝜙)
        result = (char*) malloc(strlen(j) + strlen("-") + strlen(path) + 2);
        if (result == NULL) {
            printf("Error allocating memory for path");
            exit(1);
        }
        strcpy(result, j);
        strcat(result, "-");
        strcat(result, path);

        forwarding_tableChange(forwarding_table, n, i, result);
        if (shortest_tableUpdate(forwarding_table, shortest_table, n)) {
            isChanged = true;
            expedition_tableUpdate(shortest_table, expedition_table, n);
        }
        free(result);
    } else {
        printf("Debug: Invalid path\n");
    }

    free(i);
    free(n);
    free(path);
    return(isChanged);
}


int shortest_tableUpdate(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* index) {
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


void expedition_tableUpdate(char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* index) {
    int i = atoi(index);

    if (strlen(shortest_table[i]) == 2) {
        expedition_tableChange(expedition_table, index, NULL);
    } else {
        // Parse the shortest path to find the last destination
        char* path = shortest_table[i];
        char* token = strtok(path, "-");
        char* lastDestination = NULL;

        // Traverse through the tokens to find the last one
        while (token != NULL) {
            lastDestination = token;
            token = strtok(NULL, "-");
        }

        // Update the expedition_table with the last destination
        expedition_tableChange(expedition_table, index, lastDestination);
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
}

// Change entry in shortest Path Table given index and new text
// If input is null, entry is cleared
void shortest_tableChange(char* shortest_table[TABLE_SIZE], char* index, char* input) {
    int i = atoi(index);

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
}

// Change entry in expedition_table_table Table given index and new text
void expedition_tableChange(char* expedition_table[TABLE_SIZE], char* index, char* input) {
    int i = atoi(index);

    if (expedition_table[i] != NULL) {
        free(expedition_table[i]);
    }

    if (input != NULL) {
        expedition_table[i] = strdup(input);
        if (expedition_table[i] == NULL) {
            printf("Memory allocation error\n");
            exit(1);
        }
    } else {
        expedition_table[i] = NULL;
    }
}

void freeRoutingTables(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE]) {
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



