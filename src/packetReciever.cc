#include "net.h"
#include "timer.h"
#include "packetReciever.h"

packetReciever::packetReciever(int sockfd, char* filename) {
	this->sockfd = sockfd;
	this->eof = false;
	this->packetsReceived = 0;

	file = fopen(filename, "wb");

	net_read(&packetSize, sizeof(int), sockfd);
	net_read(&range, sizeof(int), sockfd);

	sequenceNumber = 0;
}

void packetReciever::recieveFile() {
	timer totalTimer;
	totalTimer.start();
	while (!eof) {
		recievePacket();
		printf("\n");
	}
	double totalTime = totalTimer.end();
	printEndStats(totalTime);
}

void packetReciever::incrementSequenceNumber() {
	sequenceNumber++;
	sequenceNumber %= range;
	packetsReceived++;
}

void packetReciever::recievePacket() {
	int n;
	uint32_t src, dst;
	uint8_t *buffer = (uint8_t *) malloc(sizeof(uint8_t) * packetSize);

	net_read(&n, sizeof(int), sockfd);
	net_read(&src, sizeof(uint32_t), sockfd);
	net_read(&dst, sizeof(uint32_t), sockfd);
	net_read(buffer, sizeof(uint8_t) * packetSize, sockfd);

	//printf("%s\n", buffer);
	printf("Expected seq#: %d\n", sequenceNumber);

	char ipstr[IPSTRLEN];
	net_addrstr(src, ipstr, IPSTRLEN);

	printf("Packet %d recieved from %s.\n", n, ipstr);

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
	printf("Ack %d sent.\n", n);
}

void packetReciever::printEndStats(double totalTime) {
	printf("Packet size received %d\n", packetSize);
	printf("Packets received %d\n", packetsReceived);
	printf("Total elapsed time %f\n", totalTime);
}
