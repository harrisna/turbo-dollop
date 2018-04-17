#include <stdlib.h>
#include <stdio.h>

#include "net.h"
#include "packetReciever.h"
#include "packetSender.h"

int main(int argc, char **argv) {
	if (argc > 2) {
		packetReciever r = packetReciever(startClient(argv[2]), argv[1]);
		r.recieveFile();
	} else {
		int range, pktsz;
		long damPercent;

		printf("Enter a sequence number range:\n");
		scanf("%d", &range);
		printf("\nEnter packet size (in bytes):\n");
		scanf("%d", &pktsz);
		printf("Enter percent of packets you would like damaged:\n");
		scanf("%ld", &damPercent);

		int servfd = startServer();
		int clifd = acceptClient(servfd);

		packetSender s = packetSender(clifd, pktsz, range, argv[1], damPercent);
		s.sendFile();
	}
}
