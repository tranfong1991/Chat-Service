#pragma once

#include "Server.h"
#include <map>
#include <vector>
#include <iostream>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

using namespace std;

struct ChatRoomThreadParams{
	fd_set* clientSet;
	vector<int>* clientFds;
	int* maxFd;
};

class ChatRoom : public Server{
	fd_set clientSet;
	vector<int> clientFds;
	
	static void* handleClient(void* arg);
public:
	ChatRoom(int port);
	
	int getRoomSize();
	virtual void start();
	virtual void stop();
};


void* ChatRoom::handleClient(void* arg){
	ChatRoomThreadParams* params = (ChatRoomThreadParams*)arg;
	fd_set* fdSet = params->clientSet;
	int* maxFd = params->maxFd;
	vector<int>* fds = params->clientFds;
	
	fd_set temp;
	struct timeval tv;
	while(1){
		FD_ZERO(&temp);
		temp = *fdSet;
				
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		int ret = select((*maxFd) + 1, &temp, NULL, NULL, &tv);
		if(ret < 0){
			perror("ERROR in select");
			break;
		} else if(ret == 0){
			//time out
			continue;
		} else {
			for(int i = 0; i < fds->size(); i++){
				int fd = fds->at(i);
				if(FD_ISSET(fd, &temp)){
					char buf[MAX_BUF_SIZE];
					memset(buf, 0, MAX_BUF_SIZE);
					
					int len = recv(fd, buf, MAX_BUF_SIZE, 0);
					
					//when user terminates unceremoniously (i.e. len == 0)
					if(len <= 0){
						perror("ERROR receiving message");
						close(fd);
						FD_CLR(fd, fdSet);
						fds->erase(fds->begin() + i);
						break;
					}
					
					//send the recently received message to the rest of the room members
					for(int j = 0; j < fds->size(); j++){
						if(fds->at(j) == fd)
							continue;
						
						int n = write(fds->at(j), buf, strlen(buf));
						if(n < 0){
							perror("ERROR sending message");
							close(fds->at(j));
							FD_CLR(fds->at(j), fdSet);
							fds->erase(fds->begin() + j);
						}
					}
					break;
				}
			}
		}
	}
}

ChatRoom::ChatRoom(int port) : Server(port){
	FD_ZERO(&clientSet);
}

int ChatRoom::getRoomSize(){
	return clientFds.size();
}

void ChatRoom::start(){
	int newSocketFd;
	struct sockaddr_in clientAddr;
	unsigned int clientLen = sizeof(clientAddr);
	int maxFd = 0;
	
	ChatRoomThreadParams params;
	params.clientSet = &clientSet;
	params.maxFd = &maxFd;
	params.clientFds = &clientFds;
	
	pthread_t thread;
	pthread_create(&thread, NULL, &handleClient, (void*)&params);
   
	while (1) {
		newSocketFd = accept(sockFd, (struct sockaddr*) &clientAddr, &clientLen);
		
		if (newSocketFd < 0) {
			perror("ERROR on accept");
			close(sockFd);
			exit(1);
		}

		FD_SET(newSocketFd, &clientSet);
		clientFds.push_back(newSocketFd);
		
		if(maxFd < newSocketFd)
			maxFd = newSocketFd;
   }
}

void ChatRoom::stop(){
	//send warning message to all clients
	string msg = "Chat room being deleted, shutting down connection...";
	for(int i = 0; i < clientFds.size(); i++){
		int n = write(clientFds[i], msg.c_str(), msg.length());
		if(n < 0)
			perror("ERROR sending message");
		
		close(clientFds[i]);
	}
	
	FD_ZERO(&clientSet);
	Server::stop();
}