#ifndef _PACKET_SENDER_H_
#define _PACKET_SENDER_H_

#include <stdio.h>
#include <stdint.h>

class packetSender {
private:
	int sockfd;
	int packetSize;
	int range;
	uint32_t src;
	uint32_t dst;

	uint8_t *sequenceNumberList;
	uint8_t **data;
	bool hasOverrun;
	uint8_t overrun;

	bool eof;

	int sequenceNumber;

	FILE *file;

	int getSequenceNumber();

public:
	packetSender(int sockfd, int packetSize, int range, char* filename);
	void sendPacket(int n);
	void recieveAck();

	void sendFile();
};

#endif
