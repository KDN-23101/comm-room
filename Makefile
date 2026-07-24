CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall

all: server client winserver winclient

server: server.cpp
	$(CXX) $(CXXFLAGS) server.cpp -o server -lssl -lcrypto

client: client.cpp
	$(CXX) $(CXXFLAGS) client.cpp -o client -lssl -lcrypto

server_win: winserver.cpp
	$(CXX) $(CXXFLAGS) winserver.cpp -o winserver.exe -lws2_32 -lssl -lcrypto

client_win: winclient.cpp
	$(CXX) $(CXXFLAGS) winclient.cpp -o winclient.exe -lws2_32 -lssl -lcrypto

clean:
	rm -f server client winserver.exe winclient.exe
