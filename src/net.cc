#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT_NO 9765

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


int main(int argc, char** argv) {
	int sockfd;
	if (argc > 1) {
		sockfd = startClient(argv[1]);	
		int n;
		net_read(&n, sizeof(int), sockfd);
		printf("%d\n", n);
		n = 420;
		net_write(&n, sizeof(int), sockfd);
		net_read(&n, sizeof(int), sockfd);
		close(sockfd);
	} else {
		sockfd = startServer();
		int clifd = acceptClient(sockfd);
		int n = 69;
		net_write(&n, sizeof(int), clifd);
		net_read(&n, sizeof(int), clifd);
		printf("%d\n", n);
		net_write(&n, sizeof(int), clifd);
		close(clifd);
		close(sockfd);
	}

	return 0;
}

