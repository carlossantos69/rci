#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>

int join_command(char** arguments, char* buffer, char* ring, int fd_UDP, struct addrinfo *TEJO_res, char* ID, char* IP, char* TCP, char* succID, char* succIP, char* succTCP, char* second_succID, char* second_succIP, char* second_succTCP, char* predID, bool *registado);
int direct_join(char* ID, char* IP, char* TCP, char* succIP, char* succTCP, struct addrinfo *res);