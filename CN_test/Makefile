CFLAG = -Wall -O2 -g
CLIB = -lpthread
CC = g++

all: server client

server: server.o tcp.o unit.o
	$(CC) -o server server.o tcp.o unit.o $(CFLAG) $(CLIB)
server.o: server.cpp
	$(CC) -c server.cpp $(CFLAG) $(CLIB)

client: client.o tcp.o unit.o
	$(CC) -o client client.o tcp.o unit.o $(CFLAG) $(CLIB)
client.o: client.cpp
	$(CC) -c client.cpp $(CFLAG) $(CLIB)

tcp.o: tcp.cpp
	$(CC) -c tcp.cpp $(CFLAG) $(CLIB)

unit.o: unit.cpp
	$(CC) -c unit.cpp $(CFLAG) $(CLIB)

clean:
	rm -rf *.o server client
