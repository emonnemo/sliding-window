CXXFLAGS = -std=c++11

all: send recv

send:
	g++ -o sendfile sender.cpp -std=c++11

recv:
	g++ -o recvfile receiver.cpp -std=c++11

clean:
	-rm sendfile recvfile