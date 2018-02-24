#ifndef _PACK_RECV_H_
#define _PACK_RECV_H_

#include <stdio.h>
#include <stdint.h>
#include <cstring>

class packetReciever {
private:
	int sockfd;
	int packetSize;
	int range;
	int packetsReceived;

	int sequenceNumber;

	bool overrun;
	bool eof;
	char* filename;

	FILE *file;
	void printEndStats(double totalTime);
	void incrementSequenceNumber();

public:
	packetReciever(int sockfd, char* filename);
	void recievePacket();
	void sendAck(int n);
	void recieveFile();
};

#endif
