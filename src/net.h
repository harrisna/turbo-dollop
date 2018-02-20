extern int startServer();
extern int startClient(char* host);
extern int acceptClient(int socket);
extern void net_write(void* x, size_t sz, int socket);
extern void net_read(void* x, size_t sz, int socket);
