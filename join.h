#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int join_command(char* ring, char* id, int fd, struct addrinfo *info, char* IP, char* TCP);

int leave_command(char* ring, char* id, int fd, struct addrinfo *info, char* IP, char* TCP);