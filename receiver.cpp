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
#include <fstream>

using namespace std;

// global variables
char ack[7];
char receive[9];

int error_arg_usage() {
	cout << "Usage: ./recv <filename> <window_size> <buffer_size> <port> \n";
	exit(1);
}

int min(int a, int b) {
	return a > b ? b : a;
}

char calculate_checksum() {
	char calculated_checksum = 0;
	for (int i = 0; i < 6; i++) {
		calculated_checksum += ack[i];
	}
	return calculated_checksum;
}

bool check_checksum(char checksum) {
	char calculated_checksum = 0;
	for (int i = 0; i < 8; i ++) {
		calculated_checksum += receive[i];
	}
	return checksum == calculated_checksum;
}

uint32_t get_sequence_number() {
	uint32_t sequence_number;
	memcpy(&sequence_number, receive + 1, sizeof(uint32_t));
	return sequence_number;
}

// prototype
void serialize_ack(uint32_t next_sequence_number, char advertised_window_size);

int main(int argc, char** argv)
{
	int sock,bytes_received,i=1;
	uint32_t sin_size;
	struct hostent *host;
	struct sockaddr_in server_addr, sender_addr;
	string filename;
	int window_size;
	int buffer_size;
	int port;

	// checking arguments
	if (argc < 5) {
		// not enough arguments
		error_arg_usage();
	}
	try {
		// get the arguments
		filename = argv[1];
		window_size = atoi(argv[2]);
		if (window_size <= 0) {
			throw 2;
		}
		buffer_size = atoi(argv[3]);
		if (buffer_size <= 0) {
			throw 3;
		}
		port = atoi(argv[4]);
		if (port <= 0) {
			throw 4;
		}
	} catch (int argNumber) {
		cout << "Error at argument number " << argNumber << endl;
	}

	host = gethostbyname("127.0.0.1");
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Socket not created");
		exit(1);
	}
	printf("Socket created");
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(port);
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
	char receive_buffer[buffer_size];
	uint32_t next_sequence_number = 0;
	uint32_t last_frame_received = -1;
	uint32_t largest_frame = last_frame_received + window_size;
	cout << "\n";
	ofstream file(filename.c_str());
	int buffer_index = 0;
	while(1) {
		bytes_received = recvfrom(sock, receive, sizeof(receive), 0, (sockaddr*)&sender_addr, &sin_size);
		char str[INET_ADDRSTRLEN];
		// inet_ntoa(AF_INET, &(sender_addr.sin_addr), str, INET_ADDRSTRLEN);
		char *ip = inet_ntoa(sender_addr.sin_addr);
		// cout << "message from ip :" << string(ip) << ", port :" << ntohs(sender_addr.sin_port) << " -- "; 
		// bytes_received=recv(sock,receive,9,0);
		receive[bytes_received]='\0';
		if(receive[6] == EOF && check_checksum(receive[8])) {
			char advertised_window_size = min(window_size, buffer_size - buffer_index);
			serialize_ack(get_sequence_number() + 1, advertised_window_size);
			sendto(sock, ack, 7, 0, (sockaddr*)&sender_addr, sizeof(sender_addr));
			// flush the buffer to output file
			for (int i = 0; i < buffer_index; i++) {
				file << receive_buffer[i];
			}
			cout << "exited\n";
			close(sock);
			break;
		}
		else {
			if(strlen(receive) != 0) {
				// cout << "Frame " << i << " data " << recei << " received\n";
				if (check_checksum(receive[8])) {
					uint32_t sequence_number = get_sequence_number();
					if (sequence_number == last_frame_received + 1) {
						next_sequence_number = sequence_number + 1;
						last_frame_received = sequence_number;
						// file << receive[6];
						if (buffer_index < buffer_size) {
							receive_buffer[buffer_index] = receive[6];
							buffer_index++;
						}
						if (buffer_index >= buffer_size){ // flush the buffer to output file
							for (int i = 0; i < buffer_size; i++) {
								file << receive_buffer[i];
							}
							buffer_index = 0;
						}
						largest_frame++;
						printf("Frame %d data -%c- received\n",i,receive[6]);
					} else { // get not the next expected frame
						if (sequence_number <= largest_frame) {
							if (sequence_number - last_frame_received + buffer_index < buffer_size) {
								receive_buffer[sequence_number - last_frame_received + buffer_index] = receive[6];
								printf("Frame %d data -%c- received\n",i,receive[6]);
							}
						} else {
							printf("Frame %d data -%c- rejected\n",i,receive[6]);
						}
					}
					cout << "sequence_number :" << sequence_number << endl;
					char advertised_window_size = min(window_size, buffer_size - buffer_index);
					serialize_ack(next_sequence_number, advertised_window_size);
					sendto(sock, ack, 7, 0, (sockaddr*)&sender_addr, sizeof(sender_addr));
				} else { // wrong checksum, error in sending
					printf("Frame %d data -%c- wrong checksum\n",i,receive[6]);
					uint32_t sequence_number = get_sequence_number();
					char advertised_window_size = min(window_size, buffer_size - buffer_index);
					serialize_ack(last_frame_received + 1, advertised_window_size);
					sendto(sock, ack, 7, 0, (sockaddr*)&sender_addr, sizeof(sender_addr));
				}
				// send(0,receive,strlen(receive),0);
			}
			else {
				send(0,"negative",10,0);
			}
		i++;
		}
	}
	file.close();
	close(sock);
	return(0);
}

void serialize_ack(uint32_t next_sequence_number, char advertised_window_size) {
	ack[0] = 0x6;
	memcpy(ack + 1, &next_sequence_number, sizeof(uint32_t));
	ack[5] = advertised_window_size;
	ack[6] = calculate_checksum();
}