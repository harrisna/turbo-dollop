#ifndef _PACKET_SENDER_H_
#define _PACKET_SENDER_H_

#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include <vector>

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
	std::vector<int> errors;

	bool hasOverrun;
	uint8_t overrun;

	bool eof;

	int sequenceNumber;

	FILE *file;

	int getSequenceNumber();
	void printEndStats(double totalTime);

public:
	packetSender(int sockfd, int packetSize, int range, 
		int windowSize, bool recieverWindow, 
		double timeout, char* filename, 
		long damPercent, std::vector<int> errors, int errorChoice);
	
	void encodePacket(int n, int windowOffset);
	void sendPacket(int n, int windowOffset);
	int recieveAck(double timeout);

	void sendFile();
};

#endif
