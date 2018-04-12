#ifndef _PACKET_SENDER_H_
#define _PACKET_SENDER_H_

#include <stdio.h>
#include <stdint.h>
#include <cstring>

class packetSender {
private:
	int sockfd;
	int packetSize;
	int range;
	int windowSize;
	uint32_t src;
	uint32_t dst;
	char* filename;
	int packetsSent;

	int encoded;	// last encoded packet
	uint8_t **data;

	bool hasOverrun;
	uint8_t overrun;

	bool eof;

	int sequenceNumber;

	FILE *file;

	int getSequenceNumber();
	void printEndStats(double totalTime);

public:
	packetSender(int sockfd, int packetSize, int range, char* filename);
	void encodePacket(int n);
	void sendPacket(int n);
	int recieveAck();

	void sendFile();
};

#endif
