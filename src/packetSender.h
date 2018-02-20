#ifndef _PACKET_SENDER_H_
#define _PACKET_SENDER_H_

class packetSender {
private:
	int sockfd;
	int packetSize;
	int range;
	char *src;
	char *dst;

	uint8_t *sequenceNumberList;
	uint8_t **data;

	int sequenceNumber;

	FILE *file;

public:
	packetSender(int sockfd, int packetSize, int range);
	void sendPacket();
	void sendAck();
};

#endif
