#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define MAX_BUF_SIZE 1024

class Server{
protected:
	int sockFd;
public:
	Server(int port);
	
	int getPort();
	virtual void start() = 0;
	virtual void stop();
};

Server::Server(int port){
	struct sockaddr_in serverAddr;
	
	/* First call to socket() function */
	sockFd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockFd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	/* Initialize socket structure */
	memset((char *)&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	/* Now bind the host address using bind() call.*/
	if (bind(sockFd, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	/*
	* Now start listening for the clients, here
	* process will go in sleep mode and will wait
	* for the incoming connection
	*/
	listen(sockFd, 5);
}

int Server::getPort(){
	struct sockaddr_in sa;
	unsigned int saLen;
	
	saLen = sizeof(sa);
	if(getsockname(sockFd, (struct sockaddr*)&sa, &saLen) < 0){
		perror("ERROR in getsockname");
		return -1;
	}
	
	return (int)ntohs(sa.sin_port);
}

void Server::stop(){
	close(sockFd);
}