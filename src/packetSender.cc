#include "net.h"
#include "timer.h"

#include "packetSender.h"

packetSender::packetSender(int sockfd, int packetSize, int range, char* filename) {
	this->sockfd = sockfd;
	this->packetSize = packetSize;
	this->range = range;
	this->hasOverrun = false;
	this->eof = false;
	sequenceNumber = 0;
	
	sequenceNumberList = (uint8_t *) calloc(range, sizeof(uint8_t));
	
	data = (uint8_t **) malloc(sizeof(uint8_t *) * range);

	file = fopen(filename, "rb");

	net_write(&packetSize, sizeof(int), sockfd);
	net_write(&range, sizeof(int), sockfd);
}

// TODO: pass/open file here
void packetSender::sendFile() {
	timer totalTimer;
	totalTimer.start();
	
	while (!eof) {
		timer rtTimer;
		rtTimer.start();

		sendPacket(getSequenceNumber());
		recieveAck();

		double rtt = rtTimer.end();
		printf("RTT: %fms\n", rtt);
	}

	double totalTime = totalTimer.end();
	printf("Total Time: %fms\n", totalTime);
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

	// encode buffer - insert escape bytes
	int i = 0;

	if (hasOverrun) {
		buffer[i] = overrun;
		hasOverrun = false;
		i++;
	}

	for (; i < packetSize; i++) {
		int c = fgetc(file);

		buffer[i] = c;

		if (c == EOF) {
			buffer[i] = 0x00;
			eof = true;
			break;
		}

		if (buffer[i] == 0x00 || buffer[i] == 0x01) {
			if (i != packetSize - 1) {
				buffer[i+1] = buffer[i];
			} else {
				overrun = buffer[i];
				hasOverrun = true;
			}

			buffer[i] = 0x01;
			i++;
		}
	}

	printf("%s\n", buffer);

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
