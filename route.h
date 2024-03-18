#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TABLE_SIZE 100

int RouteHandler(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* command, char* destination);

void expedition_tableChange(char* expedition_table[TABLE_SIZE], char* index, char* input);

void freeRoutingTables(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE]);

void shortest_tableChange(char* shortest_table[TABLE_SIZE], char* index, char* input);

void forwarding_tableChange(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* index1, char* index2, char* input);

void refreshExpeditionTable(char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* index);

int refreshShortestTable(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char*shortest_table[TABLE_SIZE], char* index);