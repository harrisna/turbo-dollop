#include "net.h"
#include "packetReciever.h"

packetReciever::packetReciever(int sockfd, char* filename) {
	this->sockfd = sockfd;
	this->eof = false;

	file = fopen(filename, "wb");

	net_read(&packetSize, sizeof(int), sockfd);
	net_read(&range, sizeof(int), sockfd);

	sequenceNumber = 0;
}

void packetReciever::recieveFile() {
	while (!eof) {
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

	printf("%s\n", buffer);

	if (n == sequenceNumber) {
		// decode the buffer
		int i = 0;
		if (overrun)
			i++;
		for (; i < packetSize; i++) {
			//fwrite(buffer, sizeof(uint8_t), packetSize, file);
			if (buffer[i] == 0x00) {
				eof = true;
				break;
			} else if (buffer[i] == 0x01) {
				// FIXME: if there is overrun on an escape character, we need to save that state
				if (i == packetSize - 1)
					overrun = true;
				else
					i++;
			}
			fputc(buffer[i], file);
		}
	}

	free(buffer);

	sendAck(sequenceNumber);
	incrementSequenceNumber();
}

void packetReciever::sendAck(int n) {
	net_write(&n, sizeof(int), sockfd);
}
