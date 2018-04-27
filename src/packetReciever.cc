#include "net.h"
#include "timer.h"
#include "packetReciever.h"
#include "rand.h"

packetReciever::packetReciever(int sockfd, char* filename) {
	this->sockfd = sockfd;
	this->eof = false;
	this->packetsReceived = 0;
	this->filename = filename;

	//this->windowSize = 1;

	data = (uint8_t **) malloc(sizeof(uint8_t *) * windowSize);
	recieved = (bool *) malloc(sizeof(bool *) * windowSize);
	memset(recieved, 0, sizeof(bool) * windowSize);

	this->overrun = false;

	file = fopen(filename, "wb");

	net_read(&packetSize, sizeof(int), sockfd);
	net_read(&range, sizeof(int), sockfd);
	net_read(&windowSize, sizeof(int), sockfd);

	for (int i = 0; i < 10; i++) {
		uint8_t png;
		net_read(&png, sizeof(uint8_t), sockfd);
	}

	sequenceNumber = 0;
}

void packetReciever::recieveFile() {
	timer totalTimer;
	totalTimer.start();

	//int rws = windowSize;	// window size
	int rws = 1;
	int lfr = 0;		// last frame recieved
	int laf = 0;		// last acceptable frame (last ack sent)

	bool flushed = false;	// true if the recieved array is all false, meaning no packets are buffered

	// FIXME: if eof received but not prev packet, we'll finish early
	while (!(eof && flushed)) {
		printf("eof: %d flushed: %d\n", eof, flushed);
		
		// recieve pkt
		recievePacket();

		// check if we have flushed the array
		flushed = true;
		for (int i = 0; i < windowSize; i++) {
			if (recieved[i]) {
				flushed = false;
				break;
			}
		}
		
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

	printf("Expected seq#: %d\n", sequenceNumber);
	printf("%s\n", buffer);

	char ipstr[IPSTRLEN];
	net_addrstr(src, ipstr, IPSTRLEN);

	printf("Packet %d recieved from %s.\n", n, ipstr);

	bool packetGood = false;

	// allocate decoded buffer
	uint8_t *decoded = (uint8_t *) calloc(packetSize, sizeof(uint8_t));	
	int di = 0;

	// FIXME: what about wraps?
	int idx = n - sequenceNumber;
	if (idx < 0 && idx + range < windowSize)
		idx += range;

	if (idx >= 0 && idx < windowSize) {
		// decode the buffer
		int i = 0;
		if (overrun) {
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
			decoded[di] = buffer[i];
			di++;
		}

		printf("checksum: %d\n", cksum((uint16_t*) buffer, packetSize / 2));
		packetGood = (sum == cksum((uint16_t*) buffer, packetSize / 2));

		if (packetGood) {
			data[idx] = decoded;
			recieved[idx] = true;
			sendAck(n);
			if (n == sequenceNumber) {
				int i = 0;
				while (i < windowSize && recieved[i]) {
					fwrite(data[i], sizeof(uint8_t), di, file);	// TODO: test if this works correctly on binaries
					free(data[i]);
					recieved[i] = false;

					i++;
					incrementSequenceNumber();
				}

				// shift data over
				for (int j = 0; j < windowSize - i; j++) {
					data[j] = data[j + i];
					recieved[j] = recieved[j + i];
					data[j + i] = NULL;
					recieved[j + i] = false;
				}
			}
		} else {
			eof = false;	// if we recieved a bad packet with a zero, reset eof flag
		}
	} else if (idx < 0 && idx >= -windowSize) {
		sendAck(n);
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
