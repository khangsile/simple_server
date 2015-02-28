CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread

OBJS = DieWithError.o HandleTCPClient.o TCPEchoServer.o CreateTCPServerSocket.o AcceptTCPConnection.o

all: tcpserver

tcpserver: $(OBJS)
	$(CC) $(OBJS) -o tcpserver $(LDFLAGS)

TCPEchoServer.o: TCPEchoServer.c
	$(CC) $(CFLAGS) -c TCPEchoServer.c

HandleTCPClient.o: HandleTCPClient.c
	$(CC) $(CFLAGS) -c HandleTCPClient.c

DieWithError.o: DieWithError.c
	$(CC) $(CFLAGS) -c DieWithError.c

TCPEchoServer-Fork.o: TCPEchoServer-Fork.c
	$(CC) $(CFLAGS) -c TCPEchoServer-Fork.c

CreateTCPServerSocket.o: CreateTCPServerSocket.c
	$(CC) $(CFLAGS) -c CreateTCPServerSocket.c

AcceptTCPConnection.o: AcceptTCPConnection.c
	$(CC) $(CFLAGS) -c AcceptTCPConnection.c

clean:
	rm -f *~ *.o tcpserver core