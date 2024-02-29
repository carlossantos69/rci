#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int get_nodeslist (int fd, struct addrinfo *info, char *ring);

int reg_node(int fd, struct addrinfo *info, char* ring, char* id, char* IP, char* TCP);

int unreg_node(int fd, struct addrinfo *info, char* ring, char* id);

int sendUDP(int fd, struct addrinfo *info, char* message);

int viewAll(int fd, struct addrinfo *info);