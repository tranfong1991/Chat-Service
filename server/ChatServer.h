#pragma once

#include "Server.h"
#include "ChatRoom.h"
#include <iostream>
#include <map>

using namespace std;

class ChatServer;

struct ChatServerThreadParams{
	string roomName;
	int sockFd;
};

class ChatServer : public Server{
	static map<string, int> namePortMap;
	static map<int, ChatRoom*> portChatRoomMap;
	
	static void* startNewChatRoom(void* arg);
	static void* handleCreate(void* arg);
	static void* handleDelete(void* arg);
	static void* handleJoin(void* arg);
	
	void handle(int newSocketFd);
public:
	ChatServer(int port);

	virtual void start();
};

map<string, int> ChatServer::namePortMap;
map<int, ChatRoom*> ChatServer::portChatRoomMap;

void* ChatServer::startNewChatRoom(void* arg){
	ChatRoom* chatRoom = (ChatRoom*)arg;
	chatRoom->start();
}

void* ChatServer::handleCreate(void* arg){
	ChatServerThreadParams* params = (ChatServerThreadParams*)arg;
	string roomName = params->roomName;
	int sock = params->sockFd;
	int port = namePortMap[roomName];
	
	if(port == 0){
		//use port 0 so that the system will automatically assign unused port
		ChatRoom* newRoom = new ChatRoom(0);				
		int newPort = newRoom->getPort();
		namePortMap[roomName] = newPort;
		portChatRoomMap[newPort] = newRoom;
		
		string msg = roomName + " successfully created";		
		int n = write(sock, msg.c_str(), msg.length());
		if(n < 0){
			perror("ERROR writing to socket");
			close(sock);
			delete params;
			exit(1);
		}

		pthread_t thread;
		pthread_create(&thread, NULL, &startNewChatRoom, (void*)newRoom);
	} else {
		string msg = roomName + " already exists";
		int n = write(sock, msg.c_str(), msg.length());
		if(n < 0){
			perror("ERROR writing to socket");
			close(sock);
			delete params;
			exit(1);
		}
	}
	
	close(sock);
	delete params;
}

void* ChatServer::handleDelete(void* arg){	
	ChatServerThreadParams* params = (ChatServerThreadParams*)arg;
	string roomName = params->roomName;
	int sock = params->sockFd;
	int port = namePortMap[roomName];

	if(port != 0){
		ChatRoom* chatRoom = portChatRoomMap[port];
		chatRoom->stop();
		
		namePortMap[roomName] = 0;
		portChatRoomMap[port] = NULL;
		
		string msg = roomName + " successfully deleted";
		int n = write(sock, msg.c_str(), msg.length());
		if(n < 0)
			perror("ERROR writing to socket");
		
		delete chatRoom;
	} else {
		string msg = "No room found!";
		int n = write(sock, msg.c_str(), msg.length());
		if(n < 0)
			perror("ERROR writing to socket");
	}
	
	close(sock);
	delete params;
}

void* ChatServer::handleJoin(void* arg){
	ChatServerThreadParams* params = (ChatServerThreadParams*)arg;
	string roomName = params->roomName;
	int sock = params->sockFd;
	int port = namePortMap[roomName];
		
	if(port != 0){
		ChatRoom* chatRoom = portChatRoomMap[port];		
		string msg = "Port=" + to_string(port) + ", members=" + to_string(chatRoom->getRoomSize());

		int n = write(sock, msg.c_str(), msg.length());
		if(n < 0)
			perror("ERROR writing to socket");
	} else {
		string msg = "No room found!";
		int n = write(sock, msg.c_str(), msg.length());
		if(n < 0)
			perror("ERROR writing to socket");
	}
	close(sock);
	delete params;
}

ChatServer::ChatServer(int port) : Server(port){

}

void ChatServer::handle(int sock){
	fd_set fdSet;
	while(1){
		FD_ZERO(&fdSet);
		FD_SET(sock, &fdSet);
		
		int ret = select(sock + 1, &fdSet, NULL, NULL, NULL);
		if(ret < 0){
			perror("ERROR in select");
			break;
		} else {
			char buffer[MAX_BUF_SIZE];
			memset(buffer, 0, MAX_BUF_SIZE);
			
			int n = read(sock, buffer, MAX_BUF_SIZE);
			// have to check n==0 in case user terminates unceremoniously
			if(n <= 0){
				perror("ERROR reading from socket");
				close(sock);
				break;
			}
						
			//assign to a string for easier string comparison
			string command = buffer;
			if(command.compare(0, 4, "EXIT") == 0){
				close(sock);
				break;
			}
			
			ChatServerThreadParams* params = new ChatServerThreadParams();
			params->sockFd = sock;
			
			pthread_t thread;
			if(command.compare(0, 6, "CREATE") == 0){
				params->roomName = command.substr(7);
				pthread_create(&thread, NULL, &handleCreate, (void*)params);
			}
			else if(command.compare(0, 6, "DELETE") == 0){
				params->roomName = command.substr(7);
				pthread_create(&thread, NULL, &handleDelete, (void*)params);
			}
			else if(command.compare(0, 4, "JOIN") == 0){
				params->roomName = command.substr(5);
				pthread_create(&thread, NULL, &handleJoin, (void*)params);
			}
			break;
		}
	}
}

void ChatServer::start(){
	struct sockaddr_in clientAddr;
	unsigned int clientLen = sizeof(clientAddr);
   
	while (1) {
		int newSocketFd = accept(sockFd, (struct sockaddr*) &clientAddr, &clientLen);

		if (newSocketFd < 0) {
			perror("ERROR on accept");
			close(sockFd);
			exit(1);
		}

		handle(newSocketFd);
   }
}
