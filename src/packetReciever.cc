#include "net.h"
#include "packetReciever.h"

packetReciever::packetReciever(int sockfd, char* filename) {
	this->sockfd = sockfd;

	file = fopen(filename, "wb");

	net_read(&packetSize, sizeof(int), sockfd);
	net_read(&range, sizeof(int), sockfd);

	sequenceNumber = 0;
}

void packetReciever::recieveFile() {
	while (1) {
		recievePacket();
	}
}

void packetReciever::incrementSequenceNumber() {
	sequenceNumber++;
	sequenceNumber %= range;
}

void packetReciever::recievePacket() {
	int n;
	uint8_t *buffer = (uint8_t *) malloc(sizeof(uint8_t) * packetSize);

	net_read(&n, sizeof(int), sockfd);
	net_read(buffer, sizeof(uint8_t) * packetSize, sockfd);

	if (n == sequenceNumber) 
		fwrite(buffer, sizeof(uint8_t), packetSize, file);

	free(buffer);

	sendAck(sequenceNumber);
	incrementSequenceNumber();
}

void packetReciever::sendAck(int n) {
	net_write(&n, sizeof(int), sockfd);
}
