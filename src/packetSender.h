#ifndef _PACKET_SENDER_H_
#define _PACKET_SENDER_H_

#include <stdio.h>
#include <stdint.h>
#include <cstring>

#include "timer.h"

class packetSender {
private:
	int sockfd;
	int packetSize;
	int range;
	int windowSize;
	int windowOffset;
	double timeout;
	uint32_t src;
	uint32_t dst;
	char* filename;
	int packetsSent;
	long damPercent;
	int errorChoice;
	int* errorBuffer;
	int encoded;	// last encoded packet
	uint8_t **data;
	timer *rtTimer;
	bool *recieved;

	bool hasOverrun;
	uint8_t overrun;

	bool eof;

	int sequenceNumber;

	FILE *file;

	int getSequenceNumber();
	void printEndStats(double totalTime);

public:
	packetSender(int sockfd, int packetSize, int range, int windowSize, char* filename, long damPercent, int* errorBuffer, int
	errorChoice);
	void encodePacket(int n, int windowOffset);
	void sendPacket(int n, int windowOffset);
	int recieveAck(double timeout);

	void sendFile();
};

#endif
