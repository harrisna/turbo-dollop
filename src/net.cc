#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#define PORT_NO 9765

// to set timeout - 
// struct timeval tv;
// tv.tv_sec = timeout_in_seconds;
// tv.tv_usec = 0;
// setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

int startServer() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Error in socket");
		exit(1);
	}

	struct sockaddr_in serv_addr;

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT_NO);

	if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Bind error");
		exit(1);
	}

	listen(sockfd, 5);

	return sockfd;
}

int startClient(char* host) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Error in socket");
		exit(1);
	}	

	struct sockaddr_in serv_addr;
	
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT_NO);

	struct hostent *server;
	server = gethostbyname(host);
	if (server == NULL) {
		fprintf(stderr, "Error: host not found: %s\n", host);
		exit(1);
	}
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Connect error");
		exit(1);
	}

	return sockfd;
}

int acceptClient(int servSocket) {
	int sockfd;
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	
	printf("Waiting for client...\n");
	sockfd = accept(servSocket, (struct sockaddr*) &cli_addr, &clilen);

	if (sockfd < 0) {
		perror("Error accepting client");
		exit(1);
	}
	
	return sockfd;
}

// error checking wrappers for read and write

void net_write(void* x, size_t sz, int sockfd) {
	if (write(sockfd, x, sz) < 0) {
		perror("Error writing to socket");
		exit(1);
	}
}

void net_read(void* x, size_t sz, int sockfd) {
	ssize_t bytesLeft = sz;
	while (bytesLeft > 0) {
		ssize_t num = read(sockfd, (uint8_t *) x + (sz - bytesLeft), bytesLeft);

		if (num < 0) {
			perror("Error reading from socket");
			exit(1);
		}
		
		bytesLeft -= num;
	}
}

int net_wait(double timeout, int sockfd) {
	fd_set read_fds, write_fds, err_fds;
	FD_ZERO(&read_fds);	
	FD_ZERO(&write_fds);	
	FD_ZERO(&err_fds);	
	FD_SET(sockfd, &read_fds);

	struct timeval tv;
	tv.tv_sec = 1;
	//tv.tv_usec = timeout / 1000000.0;
	tv.tv_usec = 0;

	printf("%ld %d\n", tv.tv_sec, tv.tv_usec);
	//exit(1);

	return select(sockfd + 1, &read_fds, &write_fds, &err_fds, &tv);
}

uint32_t net_getaddr(int sockfd) {
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	
	getpeername(sockfd, (struct sockaddr*) &cli_addr, &clilen);

	struct in_addr addr = cli_addr.sin_addr;

	return addr.s_addr;
}

uint32_t net_getlocaladdr(int sockfd) {
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	
	getsockname(sockfd, (struct sockaddr*) &cli_addr, &clilen);

	struct in_addr addr = cli_addr.sin_addr;

	return addr.s_addr;
}

void net_addrstr(uint32_t addr, char *str, size_t len) {
	struct in_addr staddr;
	staddr.s_addr = addr;

	//char str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &staddr, str, len);
	//printf("%s\n", str);
}

uint16_t cksum(uint16_t *buf, int count) {
	uint32_t sum = 0;
	while (count--) {
		sum += *buf++;
		if (sum & 0xFFFF0000) {
			/* carry occurred, so wrap around */
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(sum & 0xFFFF);
}
