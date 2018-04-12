#include "net.h"
#include "timer.h"
#include "packetSender.h"

packetSender::packetSender(int sockfd, int packetSize, int range, char* filename) {
	this->sockfd = sockfd;
	this->packetSize = packetSize;
	this->range = range;
	this->windowSize = 1;	// TODO: assign other values
	this->hasOverrun = false;
	this->eof = false;
	this->filename = filename;
	sequenceNumber = 0;

	this->packetsSent = 0;
	
	encoded = 0;
	data = (uint8_t **) malloc(sizeof(uint8_t *) * windowSize);
	for (int i = 0; i < windowSize; i++) {
		data[i] = (uint8_t *) malloc(sizeof(uint8_t) * packetSize);
	}

	file = fopen(filename, "rb");

	this->src = net_getlocaladdr(sockfd);
	this->dst = net_getaddr(sockfd);

	net_write(&packetSize, sizeof(int), sockfd);
	net_write(&range, sizeof(int), sockfd);
}

// TODO: pass/open file here
void packetSender::sendFile() {
	timer totalTimer;
	totalTimer.start();

	int sws = windowSize;	// sending window size
	int lar = 0;	// last acknowledgement recieved
	int lfs = 0;	// last frame sent
	
	while (!eof) {
		timer rtTimer;
		rtTimer.start();

		// TODO: fix timers

		// range [lar, lfs] has already been encoded, encode lfs + sws - lar times
		// if lar is greater than lfs, we have to go back around
		int toBeEncoded = lfs + sws - ((lfs >= lar) ? lar : lar - sws - 1);
		//int toBeEncoded = lfs + sws - lar;

		// TODO: remove me
		printf("needed: %d, sequence number: %d, window size: %d, lar: %d, lfs: %d\n", toBeEncoded, sequenceNumber, windowSize, lar, lfs);
		for (int i = 0; i < toBeEncoded && !eof; i++) {
			encodePacket((sequenceNumber + i) % range);
			// TODO: remove me
			printf("%d\n", i);
		}

		for (int i = 0; i < windowSize; i++) {
			sendPacket((sequenceNumber + i) % range);
		}

		lfs = (sequenceNumber + sws - 1) % range;

		for (int i = 0; i < windowSize; i++)
			lar = recieveAck();

		sequenceNumber = lar + 1 % range;
	
		// here, find the remaining number of encoded packets and idx of first unencoded packet

		double rtt = rtTimer.end();
		printf("RTT: %fms\n\n", rtt);
	}

	double totalTime = totalTimer.end();
	fclose(file);
	printEndStats(totalTime);
}

void packetSender::encodePacket(int n) {
	uint8_t *buffer = data[n % windowSize];

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
	printf("buffer: %s\n", buffer);
}

void packetSender::sendPacket(int n) {
	net_write(&n, sizeof(int), sockfd);
	net_write(&src, sizeof(uint32_t), sockfd);
	net_write(&dst, sizeof(uint32_t), sockfd);

	net_write(data[n % windowSize], sizeof(uint8_t) * packetSize, sockfd);

	char ipstr[IPSTRLEN];
	net_addrstr(dst, ipstr, IPSTRLEN);

	printf("Packet %d sent to %s.\n", n, ipstr);
	packetsSent++;
}

int packetSender::recieveAck() {
	int n;
	net_read(&n, sizeof(int), sockfd);

	printf("Ack %d recieved.\n", n);

	return n;
}
void packetSender::printEndStats(double totalTime) {
	printf("Packet size: %d bytes\n", packetSize);
	printf("Number of packets sent: %d\n", packetsSent);
	printf("Total Time: %fms\n", totalTime);
	printf("Throughput: %f (Mbps)\n",((packetSize*packetsSent)*8)/totalTime);
	char *md5sum = (char *)malloc((strlen(filename) + 7) * sizeof(char));
	sprintf(md5sum, "md5sum %s", filename);
	system(md5sum);
	free(md5sum);
}
