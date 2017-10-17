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
#include <iomanip>
#include "ios_flag_saver.hpp"


using namespace std;

// global variable
int window_size;
int buffer_size;
string destination_ip;
int destination_port;
char fr[9];
char data[7];
fstream log;

int error_arg_usage() {
	cout << "Usage: ./send <filename> <windowsize> <buffersize> <destination_ip> <destination_port> \n";
	exit(1);
}

int min(int a, int b) {
	return a > b ? b : a;
}

char calculate_checksum() {
	char calculated_checksum = 0;
	for (int i = 0; i < 8; i++) {
		calculated_checksum += fr[i];
	}
	return calculated_checksum;
}

char check_checksum(char checksum) {
	char calculated_checksum = 0;
	for (int i = 0; i < 6; i ++) {
		calculated_checksum += data[i];
	}
	return checksum == calculated_checksum;
}

uint32_t get_sequence_number() {
	uint32_t sequence_number;
	memcpy(&sequence_number, data + 1, sizeof(uint32_t));
	return sequence_number;
}

void print_hex(char c) {
	IosFlagSaver iosfs(log);
	log << "0x" << hex << setw(2) << setfill('0') << static_cast<int>(c & 0xFF);
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
	fr[8] = calculate_checksum();
}

// function to send the file
// using a UDP socket
void send_file(string filename) {
	// setting up socket connection
	int sender_sock, receiver_sock, connected;
	char send_buffer[buffer_size + 1];
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
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
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
	if (file.is_open()) {
		cout << "File opened successfully" << endl;
	} else {
		cout << "File cannot be opened, exiting" << endl;
		exit(1);
	}
	// opening log file
	log.open("log/send_log.log", std::fstream::out | std::fstream::app);
	if (log.is_open()) {
		cout << "Log file opened successfully" << endl;
	} else {
		cout << "Log file cannot be opened, exiting" << endl;
		exit(1);
	}
	log << "---------------------------------------------------" << endl;
	log << "Trying to send file \"" << filename << "\" to " << destination_ip
		<< ":" << destination_port << endl;
	cout << "Trying to send file \"" << filename << "\" to " << destination_ip
		<< ":" << destination_port << endl;
	char ch;
	bool flag = true;
	bool stop = false;
	int buffer_index = 0;
	uint32_t sequence_number = 0;
	uint32_t next_sequence_number = 0;
	uint32_t last_sequence_received = -1;
	uint32_t min_window = 0;
	uint32_t max_window = min_window + window_size;
	uint32_t adverstised_window_size = window_size;
	uint32_t buffer_times = 0;
	bool check_buffer = true;
	bool finish_sent = false;
	bool timeout = true;
	bool accepted_once = false;
	while (1) {
		// while (file.get(send_buffer[buffer_index])) {
		while (flag || !stop) {
			if (buffer_index < buffer_size && flag) {
				char ch;
				if (file.get(ch)) {
					send_buffer[buffer_index] = ch;
					++buffer_index;
					// cout << "buffer ke " << buffer_index << " :" << ch << endl;
				} else {
					send_buffer[buffer_index] = EOF;
					++buffer_index;
					// cout << "buffer ke " << buffer_index << " :" << (char) send_buffer[buffer_index-1] << endl;
					flag = false;
				}
			}
			if (buffer_index >= buffer_size || !flag) {
				send_buffer[buffer_index] == '\0';
				// cout << "ada " << buffer_index << " di buffer\n";
				// cout << send_buffer << endl;
				// cout << "min_window :" << min_window << endl;
				// cout << "max_window :" << max_window << endl;
				int number_frame_sent = 0;
				for(int i = min_window % buffer_size; (i <= (max_window - 1) % buffer_size) && (i < buffer_index); i++) {
					number_frame_sent++;
					// cout << next_sequence_number << " " << buffer_size << " " << buffer_times << endl;
					if (next_sequence_number / buffer_size == buffer_times) {
						// cout << "---- true buffer -----" << endl;
						check_buffer =  true;
					} else {
						// cout << "---- wrong buffer -----" << endl;
						check_buffer = false;
						break;
					}
					serialize_frame(send_buffer[i], sequence_number);
					// if end of file
					if (send_buffer[i] == EOF) {
						// if it is really end of file not just 0x00 data
						if (file.peek() == -1) {
							// end of file
							fr[0] = 0x00; // differs EOF and 0x00 data
							fr[8] = fr[8] + 0x00 - 0x01;
							// if the remaining item is EOF
							if (min_window % buffer_size == i) {
								finish_sent = send_buffer[(max_window - 1) % buffer_size] == EOF;
								if (sendto(receiver_sock, fr, 9, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr))) {
									log << "Send data from buffer with index " << i << " with data = ";
									print_hex(fr[6]);
									log << "(sequence number = " << sequence_number;
									log << ")" << endl;
									++sequence_number;
								}
							}
							// all data is sent if the last in the buffer is the EOF
						}
					} else {
						if (sendto(receiver_sock, fr, 9, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr))) {
							log << "Send data from buffer with index " << i << " with data = ";
							print_hex(fr[6]);
							log << "(sequence number = " << sequence_number;
							log << ")" << endl;
							++sequence_number;
						}
					}
				}
				int number_ack_received = 0;
				while (true) {
					if (number_ack_received == (number_frame_sent)) {
						min_window = last_sequence_received + 1;
						sequence_number = min_window;
						max_window = min_window + min(window_size, adverstised_window_size);
						max_window = min(max_window, buffer_index + min_window - (min_window % buffer_size));
						break;
					}
					if (recvfrom(receiver_sock, data, sizeof(data), 0, (sockaddr *)&receiver_addr, &sin_size) >= 0) {
						number_ack_received++;
						if (strlen(data) > 0) {
							if (check_checksum(data[6])) {
								log << "Got ACK " << get_sequence_number() << endl;
								accepted_once = true;
								// cout << get_sequence_number() << " - " << next_sequence_number << endl;
								if (next_sequence_number < get_sequence_number())
									next_sequence_number = get_sequence_number();
								adverstised_window_size = data[5];
								last_sequence_received = next_sequence_number - 1;
								if (!check_buffer) break;
							} else {
								log << "Got ACK with wrong checksum" << endl;
							}
						} else {
							break;
						}
					} else {
						if (finish_sent) {
							log << "File \"" << filename << "\" fully sent to "
								<< destination_ip << ":" << destination_port
								<< " but didn't receive last ACK" << endl;
							cout << "File \"" << filename << "\" fully sent to "
								<< destination_ip << ":" << destination_port
								<< " but didn't receive last ACK" << endl;
							exit(1);
						}
						min_window = last_sequence_received + 1;
						sequence_number = min_window;
						max_window = min_window + min(adverstised_window_size, window_size);
						max_window = min(max_window, buffer_index + min_window - (min_window % buffer_size));
						// cout << "ack timeout" << endl;
						timeout = true;
						break;
					}
				}
				if (check_buffer) {
					// cout << "last_sequence_received :" << last_sequence_received << "\n";
					// reset buffer
					if ((last_sequence_received + 1) % buffer_size == 0 && accepted_once){
						buffer_index = 0;
						buffer_times++;
						accepted_once = false;
						// cout << "add 1" << endl;
						// cout << "resetting buffer \n";
						// cout << "min_window :" << min_window << endl;
						// cout << "max_window :" << max_window << endl;
						max_window = min_window + min(adverstised_window_size, window_size);
					}
					// cout << "buffer_size :" << buffer_size << ", buffer_index :" << buffer_index << endl;
					if (!flag && (last_sequence_received + 1 - buffer_index) % buffer_size == 0) {
						log << "File \"" << filename << "\" successfully sent to "
							<< destination_ip << ":" << destination_port << endl;
						cout << "File \"" << filename << "\" successfully sent to "
							<< destination_ip << ":" << destination_port << endl;
						stop = true;
						exit(1);
					}
				} else {
					if (!timeout) {
						buffer_index = 0;
						buffer_times++;
						accepted_once = false;
					}
					// cout << "add 2" << endl;
				}
			}
		}
		// // send the remaining item in the buffer
		// for(int i = 0; i < buffer_index; i++) {
		// 	serialize_frame(send_buffer[i], sequence_number);
		// 	if (sendto(receiver_sock, fr, 9, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr))) {
		// 		cout << "---->";
		// 		cout << fr[6] << " -- seq number: " << sequence_number;
		// 		cout << endl;
		// 		++sequence_number;
		// 	}
		// }
		// bool loop = true;
		// while (loop) {
		// 	if (recvfrom(receiver_sock, data, sizeof(data), 0, (sockaddr *)&receiver_addr, &sin_size) >= 0) {
		// 		if (strlen(data) > 0) {
		// 			cout << "Got ACK " << (uint32_t)(data[1]) << endl;
		// 			uint32_t next_sequence_number = (uint32_t) (data[1]);
		// 			if (next_sequence_number == min_window)
		// 				last_sequence_received = next_sequence_number - 1;
		// 		} else {
		// 			break;
		// 		}
		// 	} else {
		// 		break;
		// 	}
		// }
		// serialize_frame(EOF, sequence_number);
		// sendto(receiver_sock, fr, 9, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr));
	}
	close(sender_sock);
}