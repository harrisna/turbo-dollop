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
		int range, pktsz, option;
		char dam;
		long damPercent;
		int errorChoice = 0;
		std::vector<int> errors;

		printf("Enter a sequence number range:\n");
		scanf("%d", &range);
		printf("\nEnter packet size (in bytes):\n");
		scanf("%d", &pktsz);
		//printf("\nSelect protocol:\n
		printf("Would you like to damage packets? (y/n)");
		scanf(" %c", &dam);
		if(dam == 'y'){
			printf("Would you like to damage a percent of packets (1) or specific packet numbers(2)?\n"); 
			scanf("%d", &option);
			if(option == 1) {
				printf("Enter percent of packets you would like damaged:\n");
				scanf("%ld", &damPercent);
				errorChoice = 1;
			}
			else {
				printf("Enter the packet numbers you'd like damaged. Enter -1 to finish\n");
				int specificDamage;
				int i = 0;
				scanf("%d", &specificDamage);
				while(specificDamage != -1) {
					errors.push_back(scanf("%d", &specificDamage));
				}
				errorChoice = 2;
			}
		} else {
			errorChoice = 0;
		}
		int servfd = startServer();
		int clifd = acceptClient(servfd);

		packetSender s = packetSender(clifd, pktsz, range, 1, argv[1], damPercent, errors, errorChoice);
		s.sendFile();
	}
}
