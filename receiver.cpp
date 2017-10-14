#define _BSD_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>

using namespace std;

// global variables
char ack[7];

void serialize_ack(uint32_t next_sequence_number, char advertised_window_size) {
	ack[0] = 0x6;
	memcpy(ack + 1, &next_sequence_number, sizeof(uint32_t));
	ack[5] = advertised_window_size;
	ack[6] = 0x0; // TODO:checksum
}

int main()
{
	int sock,bytes_received,i=1;
	uint32_t sin_size;
	char receive[30];
	struct hostent *host;
	struct sockaddr_in server_addr, sender_addr;
	host = gethostbyname("127.0.0.1");
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Socket not created");
		exit(1);
	}
	printf("Socket created");
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(1025);
	server_addr.sin_addr=*((struct in_addr *)host->h_addr);
	sin_size = sizeof(struct sockaddr_in);
	bzero(&(server_addr.sin_zero),8);
	// if(connect(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))==-1) {
	// 	perror("Connect");
	// 	exit(1);
	// }
	if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))==-1) {
		perror("Unable to bind");
		exit(1);
	}
	while(1) {
		bytes_received = recvfrom(sock, receive, sizeof(receive), 0, (sockaddr*)&sender_addr, &sin_size);
		char str[INET_ADDRSTRLEN];
		// inet_ntoa(AF_INET, &(sender_addr.sin_addr), str, INET_ADDRSTRLEN);
		char *ip = inet_ntoa(sender_addr.sin_addr);
		cout << "message from ip :" << string(ip) << ", port :" << ntohs(sender_addr.sin_port) << " -- "; 
		// bytes_received=recv(sock,receive,9,0);
		receive[bytes_received]='\0';
		if(receive[0] == 'e' && receive[1] == 'x' && receive[2] == 'i' && receive[3] == 't') {
			cout << "exited\n";
			close(sock);
			break;
		}
		else {
			if(strlen(receive) != 0) {
				// cout << "Frame " << i << " data " << recei << " received\n";
				uint32_t sequence_number = (uint32_t) receive[1];
				uint32_t next_sequence_number = sequence_number + 1;
				cout << "sequence_number :" << sequence_number << endl;
				printf("Frame %d data -%c- received\n",i,receive[6]);
				char advertised_window_size = 0x9;
				serialize_ack(111, advertised_window_size);
				sendto(sock, ack, 7, 0, (sockaddr*)&sender_addr, sizeof(sender_addr));
				// send(0,receive,strlen(receive),0);
			}
			else {
				send(0,"negative",10,0);
			}
		i++;
		}
	}
	close(sock);
	return(0);
}