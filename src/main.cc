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
		int range, pktsz, protocol, option;
		char dam, packDrop, ackDrop;
		long damPercent = 0;
		long packDropPercent = 0;
		long ackDropPercent = 0;
		int errorChoice = 0;
		int packDropErrorChoice = 0;
		int ackDropErrorChoice = 0;
		std::vector<int> errors;
		std::vector<int> packetDrops;
		std::vector<int> ackDrops;

		printf("Enter a sequence number range:\n");
		scanf("%d", &range);
		printf("\nEnter packet size (in bytes):\n");
		scanf("%d", &pktsz);

		printf("\nSelect protocol:\n1. Stop and Wait\n2. Go Back N\n3. Selective Repeat\n");
		scanf("%d", &protocol);

		int windowSize = 1;
		bool recieverWindow = false;
		switch(protocol) {
			case 3:
				recieverWindow = true;
			case 2:
				// TODO: invalidate input
				printf("\nEnter window size\n");
				scanf("%d", &windowSize);
		}

		int timeout;
		printf("\nEnter timeout in milliseconds (<= 0 for dynamic):\n");
		scanf("%d", &timeout);

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
				scanf("%d", &specificDamage);
				if(specificDamage != -1) {
					errors.push_back(specificDamage);
				}
				while(specificDamage != -1) {
					scanf("%d", &specificDamage);
					if(specificDamage != -1) {
						errors.push_back(specificDamage);
					}
				}
				errorChoice = 2;
			}
		} 

		printf("Would you like to drop packets? (y/n)\n");
		scanf(" %c", &packDrop);
		if(packDrop == 'y') {
			printf("Would you like to drop a percent of packets (1) or specific packet numbers(2)?\n");
			int packDropChoice;
			scanf("%d", &packDropChoice); 
			if(packDropChoice == 1) {
				printf("Enter percent of packets you would like dropped:\n");
				scanf("%ld", &packDropPercent);
				packDropErrorChoice = 1;
			}
			else{
				printf("Enter the packet numbers you'd like to drop. Enter -1 to finish.\n");
				int packetToDrop;
				scanf("%d", &packetToDrop);
				if(packetToDrop != -1) {
					packetDrops.push_back(packetToDrop);
				}
				while(packetToDrop != -1) {
					scanf("%d", &packetToDrop);
					if(packetToDrop != -1){
						packetDrops.push_back(packetToDrop);
					}
				}
				packDropErrorChoice = 2;
			}
		}
		printf("Would you like to drop acks? (y/n)\n");
		scanf(" %c", &ackDrop);
		if(ackDrop == 'y') {
			printf("Would you like to drop a percent of acks (1) or specific ack numbers(2)?\n");
			int ackDropChoice;
			scanf("%d", &ackDropChoice);
			if(ackDropChoice == 1) {
				printf("Enter percent of acks you would like to drop:\n");	
				scanf("%ld", &ackDropPercent);
				ackDropErrorChoice = 1;
			}
			else{
				printf("Enter the ack numbers you'd like to drop. Enter -1 to finish.\n");
				int ackToDrop;
				scanf("%d", &ackToDrop);
				if(ackToDrop != -1) {
					ackDrops.push_back(ackToDrop);
				}
				while(ackToDrop != -1) {
					scanf("%d", &ackToDrop);
					if(ackToDrop != -1) {
						ackDrops.push_back(ackToDrop);
					}
				}
				ackDropErrorChoice = 2;
			}
		}

		int servfd = startServer();
		int clifd = acceptClient(servfd);

		packetSender s = packetSender(clifd, pktsz, range, 
			windowSize, recieverWindow, 
			timeout, argv[1], 
			damPercent, errors, errorChoice,
			packetDrops, ackDrops,
			packDropPercent, packDropErrorChoice,
			ackDropPercent, ackDropErrorChoice);
			
		s.sendFile();
	}
}
