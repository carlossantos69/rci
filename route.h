#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>

#define TABLE_SIZE 100

int RouteHandler(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* command, char* destination);

void expedition_tableChange(char* expedition_table[TABLE_SIZE], char* index, char* input);

void freeTables(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE]);

void shortest_tableChange(char* shortest_table[TABLE_SIZE], char* index, char* input);

void forwarding_tableChange(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* index1, char* index2, char* input);

void refreshExpeditionTable(char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* index);

int refreshShortestTable(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char*shortest_table[TABLE_SIZE], char* index);

void route_propagation(int fd, char* source, char* shortest_path[TABLE_SIZE]);

void removeColumn(char* forwarding_table[TABLE_SIZE][TABLE_SIZE], char* shortest_table[TABLE_SIZE], char* expedition_table[TABLE_SIZE], char* myid, char* close_id, int succ_fd, int pred_fd);

int isConnected(char* destination_id, char* successor_id, char* predecessor_id);
