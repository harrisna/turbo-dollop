#include "net.h"
#include "timer.h"
#include "packetReciever.h"
#include "rand.h"

packetReciever::packetReciever(int sockfd, char* filename) {
	this->sockfd = sockfd;
	this->eof = false;
	this->packetsReceived = 0;
	this->filename = filename;

	this->overrun = false;

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
	fclose(file);
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
	uint16_t sum;

	net_read(&n, sizeof(int), sockfd);
	net_read(&src, sizeof(uint32_t), sockfd);
	net_read(&dst, sizeof(uint32_t), sockfd);
	net_read(buffer, sizeof(uint8_t) * packetSize, sockfd);
	net_read(&sum, sizeof(uint16_t), sockfd);

	printf("%s\n", buffer);
	printf("Expected seq#: %d\n", sequenceNumber);

	char ipstr[IPSTRLEN];
	net_addrstr(src, ipstr, IPSTRLEN);

	printf("Packet %d recieved from %s.\n", n, ipstr);

	bool packetGood = false;

	// allocate decoded buffer
	uint8_t *decoded = (uint8_t *) calloc(packetSize, sizeof(uint8_t));	
	int di = 0;

	if (n == sequenceNumber) {
		// decode the buffer
		int i = 0;
		if (overrun) {
			//fputc(buffer[0], file);
			decoded[di] = buffer[i];
			overrun = false;
			i++;
			di++;
		}

		for (; i < packetSize; i++) {
			if (buffer[i] == 0x00) {
				eof = true;
				break;
			} else if (buffer[i] == 0x01) {
				if (i == packetSize - 1)
					overrun = true;
				else
					i++;
			}
			//fputc(buffer[i], file);
			decoded[di] = buffer[i];
			di++;
		}

		printf("checksum: %d\n", cksum((uint16_t*) buffer, packetSize / 2));
		packetGood = (sum == cksum((uint16_t*) buffer, packetSize / 2));
		printf("packetgood: %d\n", packetGood);
	}

	if (packetGood) {
		fwrite(decoded, sizeof(uint8_t), di, file);	// TODO: test if this works correctly on binaries
		sendAck(sequenceNumber);
		incrementSequenceNumber();
	} else {
		// resend prev ack
		eof = false;
		sendAck(((sequenceNumber + range) - 1) % range);
	}

	free(buffer);
}

void packetReciever::sendAck(int n) {
	net_write(&n, sizeof(int), sockfd);
	printf("Ack %d sent.\n", n);
}

void packetReciever::printEndStats(double totalTime) {
	printf("Packet size received: %d bytes\n", packetSize);
	printf("Packets received: %d\n", packetsReceived);
	printf("Total elapsed time %fms\n", totalTime);
	char *md5sum = (char *)malloc((strlen(filename) + 7) * sizeof(char));
	sprintf(md5sum, "md5sum %s", filename);
	system(md5sum);
	free(md5sum);
}
