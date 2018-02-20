#include "net.h"
#include "packetSender.h"

packetSender::packetSender(int sockfd, int packetSize, int range, char* filename) {
	this.sockfd = sockfd;
	this.packetSize = packetSize;
	this.range = range;
	sequenceNumber = 0;
	
	sequenceNumberList = calloc(range, sizeof(uint8_t));
	
	data = malloc(sizeof(uint8_t *) * range);

	file = fopen(filename, "rb");
}

int packetSender::getSequenceNumber() {
	/*int i = 0;
	while (i < range && sequenceNumberList[i] > 0)
		i++;
	*/

	int i = sequenceNumber;

	sequenceNumber++;
	sequenceNumber %= range;

	return i;
}

void packetSender::sendPacket(int n) {
	net_write(&n, sizeof(int), sockfd);
	// TODO: send src, dst

	char *buffer = malloc(sizeof(uint8_t) * packetSize);
	fread(buffer, sizeof(uint8_t), packetSize, file);

	sequenceNumberList[n] = 1;
	data[n] = buffer;

	net_write(buffer, sizeof(uint8_t) * packetSize, sockfd);
}

void packetSender::sendAck(int n) {
	net_write(&n, sizeof(int), sockfd);
}

void packetSender::recievePacket() {
	while (sequenceNumberList[sequenceNumber]) {
		int n;
		net_read(&n, sizeof(int), sockfd);

		char *buffer = malloc(sizeof(uint8_t) * packetSize);
		net_read(buffer, sizeof(uint8_t) * packetSize, sockfd);

		if (!sequenceNumberList[n]) {
			sendAck(n);
		} 

		if (n == sequenceNumber) {
			fwrite(buffer, sizeof(uint8_t), packetSize, file);
		}

		free(buffer);
	}
}

void packetSender::recieveAck() {
	while (sequenceNumberList[sequenceNumber]) {
		int n;
		net_read(&n, sizeof(int), sockfd);
		
		if (n == sequenceNumber) {
			free(data[n]);
			sequenceNumberList[n] = 0;
		}
	}
}
