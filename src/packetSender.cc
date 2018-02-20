#include "net.h"
#include "packetSender.h"

packetSender::packetSender(int sockfd, int packetSize, int range, char* filename) {
	this->sockfd = sockfd;
	this->packetSize = packetSize;
	this->range = range;
	sequenceNumber = 0;
	
	sequenceNumberList = (uint8_t *) calloc(range, sizeof(uint8_t));
	
	data = (uint8_t **) malloc(sizeof(uint8_t *) * range);

	file = fopen(filename, "rb");

	net_write(&packetSize, sizeof(int), sockfd);
	net_write(&range, sizeof(int), sockfd);
}

void packetSender::sendFile() {
	while (1) {
		sendPacket(getSequenceNumber());
		recieveAck();
	}
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

	uint8_t *buffer = (uint8_t *) malloc(sizeof(uint8_t) * packetSize);
	fread(buffer, sizeof(uint8_t), packetSize, file);

	sequenceNumberList[n] = 1;
	data[n] = buffer;

	net_write(buffer, sizeof(uint8_t) * packetSize, sockfd);
}

void packetSender::recieveAck() {
//	while (sequenceNumberList[sequenceNumber]) {
		int n;
		net_read(&n, sizeof(int), sockfd);
		
		if (n == sequenceNumber) {
			free(data[n]);
			sequenceNumberList[n] = 0;
		}
	//}
}
