#include <stdlib.h>
#include <stdint.h>

#include <arpa/inet.h>

#define IPSTRLEN INET_ADDRSTRLEN

extern int startServer();
extern int startClient(char* host);
extern int acceptClient(int socket);
extern void net_write(void* x, size_t sz, int socket);
extern int net_read(void* x, size_t sz, int socket);
extern int net_wait(double timeout, int sockfd);
extern uint32_t net_getaddr(int socket);
extern uint32_t net_getlocaladdr(int socket);
extern void net_addrstr(uint32_t addr, char *str, size_t len);

uint16_t cksum(uint16_t *buf, int count);
