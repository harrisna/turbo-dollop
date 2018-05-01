#include <algorithm>

#include "net.h"
#include "timer.h"
#include "packetSender.h"
#include "rand.h"

packetSender::packetSender(int sockfd, int packetSize, int range, 
		int windowSize, bool recieverWindow, 
		double timeout, char* filename, 
		long damPercent, std::vector<int> errors, int errorChoice,
		std::vector<int> packetDrops, std::vector<int> ackDrops,
		long packDropPercent, int packDropErrorChoice,
		long ackDropPercent, int ackDropErrorChoice) {
	this->sockfd = sockfd;
	this->packetSize = packetSize;
	this->range = range;
	this->windowSize = windowSize;
	this->timeout = timeout;
	this->filename = filename;

	this->damPercent = damPercent;
	this->errorChoice = errorChoice;
	this->errors = errors;
	this->packetDrops = packetDrops;
	this->ackDrops = ackDrops;
	this->packDropPercent = packDropPercent;
	this->packDropErrorChoice = packDropErrorChoice;
	this->ackDropPercent = ackDropPercent;
	this->ackDropErrorChoice = ackDropErrorChoice;

	this->packetsSent = 0;
	this->packetsResent = 0;
	sequenceNumber = 0;
	this->eof = false;
	this->hasOverrun = false;
	
	data = (uint8_t **) malloc(sizeof(uint8_t *) * windowSize);
	for (int i = 0; i < windowSize; i++) {
		data[i] = (uint8_t *) malloc(sizeof(uint8_t) * packetSize);
	}
	rtTimer = (timer *) malloc(sizeof(timer) * windowSize);
	recieved = (bool *) malloc(sizeof(bool) * windowSize);
	// set all packets as recieved
	for (int i = 0; i < windowSize; i++) {
		recieved[i] = true;
	}

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
		net_read(&png, sizeof(uint8_t), sockfd);
	}

	if (this->timeout <= 0.0) {
		this->timeout = (pingTimer.end() / 10.0) * 2.0;	// double avg for safety
		this->timeout = std::max(this->timeout, 1.0);
	}
}

void packetSender::sendFile() {
	timer totalTimer;
	totalTimer.start();

	int adv = windowSize;	// number of packets to encode/advance the window
	bool flushed = false;	// true if all recieved array is true, meaning all packets have been successfully sent

	while (!(eof && flushed)) {
		if (adv) {
			//printf("needed: %d, sequence number: %d, window size: %d\n", adv, sequenceNumber, windowSize);

			// shift all data over
			for (int i = adv; i < windowSize; i++) {
				memcpy(data[i - adv], data[i], sizeof(uint8_t) * packetSize);
				rtTimer[i - adv] = rtTimer[i];
				recieved[i - adv] = recieved[i];
				recieved[i] = true;
			}

			for (int i = windowSize - adv; i < windowSize && !eof; i++) {
				encodePacket((sequenceNumber + i) % range, i);
				sendPacket((sequenceNumber + i) % range, i);
				printf("Packet %d sent\n", (sequenceNumber + i) % range);
				packetsSent++;
			}

			adv = 0;	// now that we've advanced the window, reset adv
		} 

		for (int i = 0; i < windowSize; i++) {
			// if a packet hasn't been recieved and is past its timeout, resend it
			if (rtTimer[i].peek() > timeout && !recieved[i]) {
				printf("Packet %d *****Timed Out *****\n", (sequenceNumber + i) % range);
				sendPacket((sequenceNumber + i) % range, i);
				printf("Packet %d Re-transmitted.\n", (sequenceNumber + i) % range);
				packetsResent++;
			}
		}

		// find closest timer to timeout
		double oldest = 0.0;
		for (int i = 0; i < windowSize; i++) {
			if (!recieved[i]) {
				double telapsed = rtTimer[i].peek();
				if (telapsed < timeout)
					oldest = std::max(telapsed, oldest);
			}
		}
		
		// if we received a valid ack, check if we can advance the window
		if (recieveAck(timeout - oldest) != -1) {
			adv = 0;
			while (adv < windowSize && recieved[adv]) {
				adv++;
			}

			sequenceNumber += adv;
			sequenceNumber %= range;

			// print current window
			printf("Current window = [");
			for (int i = 0; i < windowSize - 1; i++) 
				printf("%d, ", (sequenceNumber + i) % range);
			printf("%d]\n", (sequenceNumber + windowSize - 1) % range);
		}

		flushed = true;
		for (int i = 0; i < windowSize; i++) {
			if (!recieved[i]) {
				flushed = false;
				break;
			}
		}
	}

	printf("Session successfully terminated\n\n");

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
}

void packetSender::sendPacket(int n, int windowOffset) {
	rtTimer[windowOffset].start();

	if(packDropErrorChoice == 1){
		long packDropRand = randL(100);
		if(packDropRand < packDropPercent) {
			return;
		}
	}
	else if(packDropErrorChoice == 2){
		if(packetDrops.size() != 0) {
			std::vector <int> :: iterator i;
			int count = 1;
			for(i = packetDrops.begin(); i != packetDrops.end(); ++i) {
				if(*i == n) {
					packetDrops.erase(packetDrops.begin() + count - 1);
					return;
				}
				count++;
			}
		}
	}

	net_write(&n, sizeof(int), sockfd);
	net_write(&src, sizeof(uint32_t), sockfd);
	net_write(&dst, sizeof(uint32_t), sockfd);

	if(errorChoice == 0) {
		net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
	}
	else if(errorChoice == 1) {
		long randomNumber = randL(100);

		if(randomNumber < damPercent) {
			// toggle first byte lsb
			data[windowOffset][0] ^= 0x01;
			net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
			data[windowOffset][0] ^= 0x01;
		}
		else {
			net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
		}
	}
	else {
		bool damaged = false;
		if(errors.size() != 0){
			std::vector <int> :: iterator j;
			int count = 1;
			for(j = errors.begin(); j != errors.end(); ++j) {
				if(*j == n) {
					damaged = true;
					errors.erase(errors.begin() + count-1);
					goto out;
				}
				count++;
			}
		}
		out:
		if(damaged) {
			// toggle first byte lsb
			data[windowOffset][0] ^= 0x01;
			net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
			data[windowOffset][0] ^= 0x01;
		}
		else {
			net_write(data[windowOffset], sizeof(uint8_t) * packetSize, sockfd);
		}
	}

	//printf("buffer: %s\n", data[windowOffset]);

	uint16_t sum = cksum((uint16_t*) data[windowOffset], packetSize / 2);
	//printf("checksum: %d\n", sum);
	net_write(&sum, sizeof(uint16_t), sockfd);

	char ipstr[IPSTRLEN];
	net_addrstr(dst, ipstr, IPSTRLEN);

	//printf("Packet %d sent to %s.\n", n, ipstr);
	//packetsSent++;
}

int packetSender::recieveAck(double timeout) {
	if (net_wait(timeout, sockfd)) {
		int n;

		// if read cannot find anything and we're at eof, receiver probably closed, so end the session
		if (net_read(&n, sizeof(int), sockfd) == -1 && eof) {
			printf("receiver connection closed; assuming last ack dropped\n");
			recieved[0] = true;
			return sequenceNumber;
		}

		//Ack drop functionality
		if(ackDropErrorChoice == 1){
			long ackDropRand = randL(100);
			if(ackDropRand < ackDropPercent){
				return -1;	
			}
		}
		else if (ackDropErrorChoice == 2){
			if(ackDrops.size() != 0) {
				std::vector <int> :: iterator i;
				int count = 1;
				for(i = ackDrops.begin(); i != ackDrops.end(); ++i) {
					if(*i == n) {
						printf("Dropping packet %d\n", n);
						ackDrops.erase(ackDrops.begin() + count - 1);
						return -1;
					}
					count++;
				}
			}
		}
		printf("Ack %d received ", n);

		if (n < sequenceNumber)
			n += range;

		if (n >= sequenceNumber && n <= sequenceNumber + windowSize) {
			double rtt = rtTimer[n - sequenceNumber].end();
			printf("(RTT: %fms)\n", rtt);

			recieved[n - sequenceNumber] = true;
		}

		return n % range;
	}

	return -1;
}
void packetSender::printEndStats(double totalTime) {
	//printf("Packet size: %d bytes\n", packetSize);
	printf("Number of original packets sent: %lu\n", packetsSent);
	printf("Number of retransmitted packets: %lu\n", packetsResent);
	printf("Total elapsed time: %fms\n", totalTime);
	printf("Total throughput (Mbps): %f\n",((packetSize*(packetsSent+packetsResent))*8)/totalTime);
	printf("Effective throughput: %f\n",((packetSize*packetsSent)*8)/totalTime);
	char *md5sum = (char *)malloc((strlen(filename) + 7) * sizeof(char));
	sprintf(md5sum, "md5sum %s", filename);
	system(md5sum);
	free(md5sum);
}
