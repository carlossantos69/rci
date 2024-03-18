#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TABLE_SIZE 100

int RouteHandler(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* buffer, char* j);

void expedition_tableChange(char* expedition_table[TABLE_SIZE], char* index, char* input);

void freeRoutingTables(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE]);

void shortest_tableChange(char* shortest_table[TABLE_SIZE], char* index, char* input);

void forwarding_tableChange(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* index1, char* index2, char* input);

void expedition_tableUpdate(char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* index);

int shortest_tableUpdate(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char*shortest_table[TABLE_SIZE], char* index);