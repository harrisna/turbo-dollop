#include "net.h"
#include "timer.h"
#include "packetReciever.h"
#include "rand.h"

packetReciever::packetReciever(int sockfd, char* filename) {
	this->sockfd = sockfd;
	this->eof = false;
	this->filename = filename;

	this->overrun = false;

	this->lastPacket = 0;
	this->packetsReceived = 0;
	this->retransmitsReceived = 0;

	file = fopen(filename, "wb");

	net_read(&packetSize, sizeof(int), sockfd);
	net_read(&range, sizeof(int), sockfd);
	net_read(&windowSize, sizeof(int), sockfd);

	for (int i = 0; i < 10; i++) {
		uint8_t png;
		net_read(&png, sizeof(uint8_t), sockfd);
		net_write(&png, sizeof(uint8_t), sockfd);
	}

	data = (uint8_t **) malloc(sizeof(uint8_t *) * windowSize);
	recieved = (bool *) malloc(sizeof(bool *) * windowSize);
	for (int i = 0; i < windowSize; i++) {
		recieved[i] = false;
	}

	sequenceNumber = 0;
}

void packetReciever::recieveFile() {
	timer totalTimer;
	totalTimer.start();

	bool flushed = false;	// true if the recieved array is all false, meaning no packets are buffered

	// FIXME: if eof received but not prev packet, we'll finish early
	while (!(eof && flushed)) {
		// recieve pkt
		recievePacket();

		printf("Current window = [");
		for (int i = 0; i < windowSize - 1; i++) {
			printf("%d, ", (sequenceNumber + i) % range);
		}
		printf("%d]\n", (sequenceNumber + windowSize - 1) % range);

		// check if we have flushed the array
		flushed = true;
		for (int i = 0; i < windowSize; i++) {
			if (recieved[i]) {
				flushed = false;
				break;
			}
		}
		//printf("eof = %d, flushed = %d\n", eof, flushed);
		
		//printf("\n");
	}

	double totalTime = totalTimer.end();
	fclose(file);
	printEndStats(totalTime);
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

	//printf("Expected seq#: %d\n", sequenceNumber);

	char ipstr[IPSTRLEN];
	net_addrstr(src, ipstr, IPSTRLEN);

	//printf("Packet %d recieved from %s.\n", n, ipstr);
	printf("Packet %d received\n", n);

	bool packetGood = false;


	// increment retransmitted packets (we cheat here, if we find that the packet was successfully sent, we increment original packets and decrement this)
	retransmitsReceived++;
	lastPacket = n;

	int idx = n - sequenceNumber;
	if (idx < 0 && idx + range < windowSize)
		idx += range;

	if (idx >= 0 && idx < windowSize) {
		packetGood = (sum == cksum((uint16_t*) buffer, packetSize / 2));

		if (packetGood) {
			printf("Checksum OK\n");
			data[idx] = buffer;
			recieved[idx] = true;

			sendAck(n);

			if (n == sequenceNumber) {
				int adv = 0;
				while (adv < windowSize && recieved[adv]) {
					// allocate decoded buffer
					uint8_t *decoded = (uint8_t *) calloc(packetSize, sizeof(uint8_t));	
					int di = 0;

					// decode the buffer
					int i = 0;
					if (overrun) {
						decoded[di] = data[adv][0];
						overrun = false;
						i++;
						di++;
					}

					for (; i < packetSize; i++) {
						if (data[adv][i] == 0x00) {
							eof = true;
							break;
						} else if (data[adv][i] == 0x01) {
							if (i == packetSize - 1)
								overrun = true; // FIXME: this doesn't work out of order!!
							else
								i++;	// skip escape character
						}
						decoded[di] = data[adv][i];
						di++;
					}

					fwrite(decoded, sizeof(uint8_t), di, file);	// TODO: test if this works correctly on binaries

					free(decoded);
					data[adv] = NULL;
					recieved[adv] = false;

					adv++;
					sequenceNumber++;
					sequenceNumber %= range;

					packetsReceived++;
					retransmitsReceived--;
				}

				// shift data over
				for (int j = adv; j < windowSize; j++) {
					data[j - adv] = data[j];
					data[j] = NULL;
					recieved[j - adv] = recieved[j];
					recieved[j] = false;
				}

				fflush(file);
			}
		} else {
			printf("Checksum failed\n");
		}
	} else if (idx < 0 && idx >= -windowSize) {
		sendAck(n);
	}
}

void packetReciever::sendAck(int n) {
	net_write(&n, sizeof(int), sockfd);
	printf("Ack %d sent\n", n);
}

void packetReciever::printEndStats(double totalTime) {
	printf("Last packet seq# received: %d\n", lastPacket);
	printf("Number of original packets received: %d\n", packetsReceived);
	printf("Number of retransmitted packets received: %d\n", retransmitsReceived);
	//printf("Total elapsed time %fms\n", totalTime);
	char *md5sum = (char *)malloc((strlen(filename) + 7) * sizeof(char));
	sprintf(md5sum, "md5sum %s", filename);
	system(md5sum);
	free(md5sum);
}
