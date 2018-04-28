#ifndef _PACKET_SENDER_H_
#define _PACKET_SENDER_H_

#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include <vector>

#include "timer.h"

class packetSender {
private:
	// constants
	int sockfd;
	int packetSize;
	int range;
	int windowSize;
	double timeout;

	// constant strings
	uint32_t src;
	uint32_t dst;
	char* filename;

	// error
	long damPercent;
	int errorChoice;
	int* errorBuffer;

	// arrays
	uint8_t **data;
	timer *rtTimer;
	bool *recieved;
	std::vector<int> errors;
	std::vector<int> packetDrops;
	std::vector<int> ackDrops;

	// state
	bool hasOverrun;
	uint8_t overrun;
	int packetsSent;
	int packetsResent;
	int sequenceNumber;
	bool eof;

	FILE *file;

	int getSequenceNumber();
	void printEndStats(double totalTime);

	void encodePacket(int n, int windowOffset);
	void sendPacket(int n, int windowOffset);
	int recieveAck(double timeout);

public:
	packetSender(int sockfd, int packetSize, int range, 
		int windowSize, bool recieverWindow, 
		double timeout, char* filename, 
		long damPercent, std::vector<int> errors, int errorChoice,
		std::vector<int> packetDrops, std::vector<int> ackDrops);

	void sendFile();
};

#endif
