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
		int servfd = startServer();

		packetSender s = packetSender(acceptClient(servfd), 10, 10, argv[1]);
		s.sendFile();
	}
}
