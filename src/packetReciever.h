#ifndef _PACK_RECV_H_
#define _PACK_RECV_H_

#include <stdio.h>
#include <stdint.h>

class packetReciever {
private:
	int sockfd;
	int packetSize;
	int range;
	char *src;
	char *dst;

	int sequenceNumber;

	bool overrun;
	bool eof;

	FILE *file;

	void incrementSequenceNumber();

public:
	packetReciever(int sockfd, char* filename);
	void recievePacket();
	void sendAck(int n);
	void recieveFile();
};

#endif
