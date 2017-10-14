// sender code

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <thread>
#include "socket_address.cpp"

using namespace std;

// global variable
int window_size;
int buffer_size;
string destination_ip;
int destination_port;
char fr[9];

int error_arg_usage() {
	cout << "Usage: ./send <filename> <windowsize> <buffersize> <destination_ip> <destination_port> \n";
	exit(1);
}

void foo(bool* loop) {
	try {
		*loop = false;
	} catch(int) {
		perror("error threading");
	}
}

// prototype
void serialize_frame(char* fr, char data, uint32_t sequence_number);
void send_file(string filename);

int main(int argc, char*argv[]) {
	struct hostent* hp;
	struct in_addr *addr;
	string filename;
	int c;
	int i;

	// checking arguments
	if (argc < 6) {
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
		destination_ip = argv[4];
		destination_port = atoi(argv[5]);
		if (destination_port <= 0) {
			throw 5;
		}
	} catch (int argNumber) {
		cout << "Error at argument number " << argNumber << endl;
	}

	// sending file
	send_file(filename);

	return 0;
}

// packeting data into frame
// add the sequence number
// and also calculate the checksum
void serialize_frame(char data, uint32_t sequence_number) {
	fr[0] = 0x1;
	memcpy(fr + 1, &sequence_number, sizeof(uint32_t));
	fr[5] = 0x2;
	fr[6] = data;
	fr[7] = 0x3;
	fr[8] = 0x0; //TODO: checksum
}

// function to send the file
// using a UDP socket
void send_file(string filename) {
	// setting up socket connection
	int sender_sock, receiver_sock, connected;
	char send_data[buffer_size];
	char data[7];
	unsigned int sin_size;
	int void_pointer = 1;
	// if((sender_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	// 	perror("Socket not created");
	// 	exit(1);
	// }
	// if (setsockopt(sender_sock, SOL_SOCKET, SO_REUSEADDR, &void_pointer, sizeof(int)) == -1) {
	// 	perror("setsockopt error");
	// 	exit(1);
	// }
	if((receiver_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Socket not created");
		exit(1);
	}
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(receiver_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(tv)) < 0) {
	    perror("Error");
	}
	// if (setsockopt(receiver_sock, SOL_SOCKET, SO_REUSEADDR, &void_pointer, sizeof(int)) == -1) {
	// 	perror("setsockopt error");
	// 	exit(1);
	// }
	int sender_port = 1026;
	string sender_ip = "127.0.0.1";
	sockaddr_in receiver_addr, sender_addr;
	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_port = htons(destination_port);
	receiver_addr.sin_addr.s_addr = inet_addr(destination_ip.c_str());

	sender_addr.sin_family = AF_INET;
	sender_addr.sin_port = htons(sender_port);
	// if (bind(receiver_sock,(struct sockaddr *)&receiver_addr,sizeof(struct sockaddr))==-1) {
	// sender_addr.sin_addr.s_addr = inet_addr(sender_ip.c_str());
	// 	exit(1);
	// }
	// if (bind(sender_sock,(struct sockaddr *)&sender_addr,sizeof(struct sockaddr))==-1) {
	// 	perror("Unable to bind");
	// 	perror("Unable to bind");
	// 	exit(1);
	// }
	// if (listen(sock, 5) == -1) {
	// 	perror("Listen");
	// 	exit(1);
	// }
	sin_size = sizeof(struct sockaddr_in);
	// connected = accept(sock,(struct sockaddr *)&receiver_addr,&sin_size);
	// reading files
	ifstream file(filename.c_str());
	if (file.is_open())
		cout << "Opened file" << endl;
	char ch;
	while (1) {
		int buffer_index = 0;
		uint32_t sequence_number = 0;
		uint32_t last_sequence_received = 0;
		uint32_t min_window = 0;
		uint32_t max_window = min_window + window_size - 1;
		while (file.get(send_data[buffer_index])) {
			// cout << send_data[buffer_index];
			++buffer_index;
			// if the buffer is full
			if (buffer_index >= buffer_size) {
				// cout << "=\n----- buffer full -----\n";
				// sending the data
				for(int i = 0; i < buffer_size; i++) {
					serialize_frame(send_data[i], sequence_number);
					// if (send(connected, fr, 9, 0)) {
					if (sendto(receiver_sock, fr, 9, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr))) {
						cout << "---->";
						cout << fr[6] << " -- seq number: " << sequence_number;
						cout << endl;
						++sequence_number;
					}
					// if (data[])

				}
				bool loop = true;
				while (loop) {
					if (recvfrom(receiver_sock, data, sizeof(data), 0, (sockaddr *)&receiver_addr, &sin_size) >= 0) {
						if (strlen(data) > 0) {
							cout << "Got ACK " << (uint32_t)(data[1]) << endl;
							uint32_t next_sequence_number = (uint32_t) (data[1]);
							if (next_sequence_number == min_window)
								last_sequence_received = next_sequence_number - 1;
						} else {
							break;
						}
					} else {
						receiver_sock.open();
						break;
					}
				}
				// reset buffer
				buffer_index = 0;
			}
			// sendto(sock,)
		}
		// send the remaining item in the buffer
		for(int i = 0; i < buffer_index; i++) {
			serialize_frame(send_data[i], sequence_number);
			if (sendto(receiver_sock, fr, 9, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr))) {
				cout << "---->";
				cout << fr[6] << " -- seq number: " << sequence_number;
				cout << endl;
				++sequence_number;
			}
			// recv(sock, data, 7, 0);
			// if (strlen(data) != 0) {
			// 	cout << "Got ACK" << endl;
			// }
			// if (data[])

		}
		fr[0] = 'e';
		fr[1] = 'x';
		fr[2] = 'i';
		fr[3] = 't';
		sendto(receiver_sock, fr, 9, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr));
	}
	close(sender_sock);
}