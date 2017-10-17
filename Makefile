CXXFLAGS = -std=c++11

all: sendfile recvfile
	mkdir -p log

sendfile:
	g++ -o sendfile src/sender.cpp -std=c++11

recvfile:
	g++ -o recvfile src/receiver.cpp -std=c++11

clean:
	-rm sendfile recvfile