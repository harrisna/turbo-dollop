#include <algorithm>

#include "net.h"
#include "timer.h"
#include "packetSender.h"
#include "rand.h"

packetSender::packetSender(int sockfd, int packetSize, int range, 
		int windowSize, bool recieverWindow, 
		double timeout, char* filename, 
		long damPercent, std::vector<int> errors, int errorChoice) {
	this->sockfd = sockfd;
	this->packetSize = packetSize;
	this->range = range;
	this->windowSize = windowSize;
	this->windowOffset = 0;
	this->timeout = timeout;
	this->hasOverrun = false;
	this->eof = false;
	this->filename = filename;
	this->damPercent = damPercent;
	this->errorChoice = errorChoice;
	sequenceNumber = 0;
	this->errors = errors;

	this->packetsSent = 0;
	
	encoded = 0;
	data = (uint8_t **) malloc(sizeof(uint8_t *) * windowSize);
	for (int i = 0; i < windowSize; i++) {
		data[i] = (uint8_t *) malloc(sizeof(uint8_t) * packetSize);
	}
	rtTimer = (timer *) malloc(sizeof(timer) * windowSize);
	recieved = (bool *) malloc(sizeof(bool) * windowSize);
	// set all packets as recieved
	memset(recieved, true, sizeof(bool) * windowSize);

	file = fopen(filename, "rb");

	this->src = net_getlocaladdr(sockfd);
	this->dst = net_getaddr(sockfd);

	net_write(&packetSize, sizeof(int), sockfd);
	net_write(&range, sizeof(int), sockfd);
	if (recieverWindow) {
		net_write(&windowSize, sizeof(int), sockfd);
	} else {
		int tmp = 1;
		net_write(&tmp, sizeof(int), sockfd);
	}

	// now ping to generate a dynamic timeout value:
	timer pingTimer;
	pingTimer.start();

	for (int i = 0; i < 10; i++) {
		uint8_t png = 0xff;
		net_write(&png, sizeof(uint8_t), sockfd);
	}

	if (this->timeout <= 0.0)
		this->timeout = (pingTimer.end() / 10.0) * 2.0;	// double avg for safety
}

// TODO: pass/open file here
void packetSender::sendFile() {
	timer totalTimer;
	totalTimer.start();

	int sws = windowSize;	// sending window size
	int lar = 0;	// last acknowledgement recieved
	int lfs = 0;	// last frame sent

	int adv = windowSize;	// number of packets to encode
	bool flushed = false;	// true if all recieved array is true, meaning all packets have been successfully sent

	while (!(eof && flushed)) {
		// range [lar, lfs] has already been encoded, encode sws - (lfs - lar) times
		// if lar is greater than lfs, we have to go back around
		//int toBeEncoded = sws - (lfs - ((lfs >= lar) ? lar : lar - range)); // if this is greater than one, we didn't time out

		bool damagePacket = false;
		if (errorChoice == 1 || errorChoice == 2) {
			damagePacket = true;
		}

		// FIXME: if eof, actually finish
		// we need some way to know that something is the last packet
		if (adv) {
			printf("needed: %d, sequence number: %d, window size: %d, lar: %d, lfs: %d\n", adv, sequenceNumber, windowSize, lar, lfs);

			// shift all data over
			for (int i = adv; i < windowSize; i++) {
				memcpy(data[i - adv], data[i], sizeof(uint8_t) * packetSize);
				rtTimer[i - adv] = rtTimer[i];
				recieved[i - adv] = recieved[i];
				recieved[i] = true;
			}

			// zero moved data
			/*for (int i = windowSize - adv; i < windowSize; i++) {
				recieved[i] = true;
			}*/

			for (int i = windowSize - adv; i < windowSize && !eof; i++) {
				printf("encoding packet #%d (buffer[%d])\n", sequenceNumber + i, i);
				encodePacket((sequenceNumber + i) % range, i);
				sendPacket((sequenceNumber + i) % range, i);

				lfs = (sequenceNumber + i) % range;
			}

			adv = 0;

			for (int i = 0; i < windowSize; i++)
				printf("buffer[%d]: %s\n", i, data[i]);
		} else {
			/*for (int i = 0; i < windowSize; i++) {
				// if a packet hasn't been recieved and is past its timeout, resend it
				if (rtTimer[i].peek() > timeout && !recieved[i]) {
					printf("RESENT buffer[%d]: %s\n", i, data[i]);
					sendPacket((sequenceNumber + i) % range, i);
				}
			}*/
		}

		for (int i = 0; i < windowSize; i++) {
			// if a packet hasn't been recieved and is past its timeout, resend it
			if (rtTimer[i].peek() > timeout && !recieved[i]) {
				printf("RESENT buffer[%d]: %s\n", i, data[i]);
				sendPacket((sequenceNumber + i) % range, i);
			}
		}

		printf("lfs = %d\n", lfs);

		// find closest timer to timeout
		double oldest = 0.0;
		for (int i = 0; i < windowSize; i++) {
			if (!recieved[i]) {
				double telapsed = rtTimer[i].peek();
				if (telapsed < timeout)
					oldest = std::max(telapsed, oldest);
			}
		}

		int ack = recieveAck(timeout - oldest);

		if (ack != -1) {
			// find largest ack, advancing lar
			printf("sequenceNumber increased: %d lar: %d\n", sequenceNumber, lar);
			adv = 0;
			while (adv < windowSize && recieved[adv]) {
				adv++;
			}

			sequenceNumber += adv;
			sequenceNumber %= range;

			printf("sequenceNumber increased: %d lar: %d\n", sequenceNumber, lar);
		}

		flushed = true;
		for (int i = 0; i < windowSize; i++) {
			printf("recieved[%d] = %d\n", i, recieved[i]);
			if (!recieved[i]) {
				flushed = false;
				//break;
			}
		}
	}

	double totalTime = totalTimer.end();
	fclose(file);
	printEndStats(totalTime);
}

void packetSender::encodePacket(int n, int windowOffset) {
	recieved[windowOffset] = false;
	uint8_t *buffer = data[windowOffset];

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
	//printf("buffer: %s\n", buffer);
}

void packetSender::sendPacket(int n, int windowOffset) {
	rtTimer[windowOffset].start();

	net_write(&n, sizeof(int), sockfd);
	net_write(&src, sizeof(uint32_t), sockfd);
	net_write(&dst, sizeof(uint32_t), sockfd);
	if(errorChoice == 0) {
		net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
	}
	else if(errorChoice == 1) {
		long randomNumber = randL(100);

		if(randomNumber < damPercent) {
			data[windowOffset][0] ^= 0x01;
			net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
			data[windowOffset][0] ^= 0x01;
		}
		else {
			net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
		}
	}
	else {

	}

	printf("buffer: %s\n", data[windowOffset]);

	uint16_t sum = cksum((uint16_t*) data[windowOffset], packetSize / 2);
	printf("checksum: %d\n", sum);
	net_write(&sum, sizeof(uint16_t), sockfd);

	char ipstr[IPSTRLEN];
	net_addrstr(dst, ipstr, IPSTRLEN);

	printf("Packet %d sent to %s.\n", n, ipstr);
	packetsSent++;
}

int packetSender::recieveAck(double timeout) {
	printf("TESTING???\n");
	if (net_wait(timeout, sockfd)) {
		int n;
		net_read(&n, sizeof(int), sockfd);

		printf("Ack %d recieved.\n", n);

		if (n < sequenceNumber)
			n += range;

		if (n >= sequenceNumber && n <= sequenceNumber + windowSize) {
			double rtt = rtTimer[n - sequenceNumber].end();
			printf("RTT: %fms\n\n", rtt);

			recieved[n - sequenceNumber] = true;
		}

		return n % range;
	}
	printf("ACK timeout\n");
	return -1;
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
