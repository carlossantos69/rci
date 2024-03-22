#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>

#define TABLE_SIZE 100

int entry_command(int fd, char* id, char* IP, char* TCP);

int succ_command(int fd, char* id, char* IP, char* TCP);

int pred_command(int fd, char* id);

int route_command(int fd, char* i, char* n, char* path);

void print_forwardingTable(char* forwardingTable[TABLE_SIZE][TABLE_SIZE], char* destination);

void print_shortestTable(char* shortestTable[TABLE_SIZE], char*destination);

void print_expeditionTable(char* expedition_table[TABLE_SIZE]);

int countElements(const char* buffer);
