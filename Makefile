CXXFLAGS = -std=c++11

all: sendfile recvfile
	mkdir -p log

sendfile:
	g++ -o sendfile sender.cpp -std=c++11

recvfile:
	g++ -o recvfile receiver.cpp -std=c++11

clean:
	-rm sendfile recvfile