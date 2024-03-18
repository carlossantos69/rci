#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int entry_command(int fd, char* id, char* IP, char* TCP);

int succ_command(int fd, char* id, char* IP, char* TCP);

int pred_command(int fd, char* id);

int route_command(int fd, char* i, char* n, char* path);