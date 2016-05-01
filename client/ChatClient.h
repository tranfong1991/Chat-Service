#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <iostream>

#define MAX_BUF_SIZE 1024

using namespace std;

class ChatClient{
	char* hostName;
	int port;
	
	static void* handleNewMsg(void* arg);
	
	void handle(char* command);
	void handleCreate(int sockFd, char* command);
	void handleJoin(int sockFd, char* command);
	void handleDelete(int sockFd, char* command);
	void handleTerminate(int sockFd, char* command);
	int connectToHost(int portNo);
	void sendRequest(int sockFd, char* command);
	void waitForResponse(int sockFd, char buffer[MAX_BUF_SIZE]);
	void sendRequestAndWaitForResponse(int sockFd, char* command);
public:
	ChatClient(char* hostName, int port);
	
	void start();
};

int findPort(char* buffer){
	int len = strlen(buffer);
	char temp[6];
	int tempIndex = 0;
	
	for(int i = 0; i < len; i++){
		if(buffer[i] == '='){
			for(int j = i + 1; j < len; j++){
				if(buffer[j] == ',')
					break;
				temp[tempIndex++] = buffer[j];
			}
			break;
		}
	}
	
	return atoi(temp);
}

void* ChatClient::handleNewMsg(void* arg){
	int sock = *((int*)arg);
	fd_set fdSet;
	
	while(1){		
		FD_ZERO(&fdSet);
		FD_SET(sock, &fdSet);
				
		int ret = select(sock + 1, &fdSet, NULL, NULL, NULL);
		if(ret < 0){
			perror("ERROR in select");
			break;
		} else {
			char buf[MAX_BUF_SIZE];
			memset(buf, 0, MAX_BUF_SIZE);
			
			int len = recv(sock, buf, MAX_BUF_SIZE, 0);
			if(len <= 0){
				perror("ERROR receiving message");
				break;
			}
			
			cout << buf << endl;
		}
	}
	close(sock);
}

ChatClient::ChatClient(char* hostName, int port){
	this->hostName = hostName;
	this->port = port;
}

void ChatClient::handle(char* c){
	int sockFd = connectToHost(port);
	if(sockFd < 0){
		perror("ERROR connecting to server");
		return;
	}
	
	string command = c;
	if(command.compare(0, 6, "CREATE") == 0){
		handleCreate(sockFd, c);
	} else if(command.compare(0, 6, "DELETE") == 0){
		handleDelete(sockFd, c);
	} else if(command.compare(0, 4, "JOIN") == 0){
		handleJoin(sockFd, c);
	} else if(command.compare(0, 4, "EXIT") == 0){
		handleTerminate(sockFd, c);
	}
}

void ChatClient::handleCreate(int sockFd, char* command){
	sendRequestAndWaitForResponse(sockFd, command);
}

void ChatClient::handleJoin(int sockFd, char* command){
	sendRequest(sockFd, command);
	
	char buffer[MAX_BUF_SIZE];
	memset(buffer, 0, MAX_BUF_SIZE);
	waitForResponse(sockFd, buffer);
	
	int chatRoomPort = findPort(buffer);
	int newSockFd = connectToHost(chatRoomPort);
	if(newSockFd < 0){
		perror("ERROR connecting to server");
		return;
	}

	cout << "You are connected to chat room in port " 
		<< chatRoomPort 
		<< ". You can start typing messages." 
		<< endl;

	pthread_t thread;
	pthread_create(&thread, NULL, &handleNewMsg, (void*)&newSockFd);

	while(1){
		char input[MAX_BUF_SIZE];
		memset(input, 0, MAX_BUF_SIZE);
		fgets(input, MAX_BUF_SIZE, stdin);

		int n = write(newSockFd, input, strlen(input) - 1);
		if(n < 0){
			perror("ERROR writing to socket");
			close(newSockFd);
			exit(1);
		}
	}
}

void ChatClient::handleTerminate(int sockFd, char* command){
	sendRequest(sockFd, command);
	
	cout << "Thank you for using this app. Exiting" << endl;
	close(sockFd);
	exit(1);
}

void ChatClient::handleDelete(int sockFd, char* command){
	sendRequestAndWaitForResponse(sockFd, command);
}

int ChatClient::connectToHost(int portNo){
	int sockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockFd < 0) {
		return -1;
	}
	
	struct hostent* server = gethostbyname(hostName);
	if (server == NULL) {
		return -1;
	}
	
	struct sockaddr_in serverAddr;
	memset((char *)&serverAddr, 0, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNo);

	if(connect(sockFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
		return -1;

	return sockFd;
}

void ChatClient::sendRequest(int sockFd, char* command){
	//strlen - 1 to exclude newline character at the end
	int n = write(sockFd, command, strlen(command) - 1);
	if(n < 0){
		perror("ERROR writing");
		close(sockFd);
		return;
	}
}

void ChatClient::waitForResponse(int sockFd, char buffer[MAX_BUF_SIZE]){
	fd_set fdSet;
	FD_ZERO(&fdSet);
	FD_SET(sockFd, &fdSet);
	
	int ret = select(sockFd + 1, &fdSet, NULL, NULL, NULL);
	if(ret < 0){
		perror("ERROR reading from socket");
		close(sockFd);
		return;
	} else {		
		int n = read(sockFd, buffer, MAX_BUF_SIZE);
		if(n < 0){
			perror("ERROR reading");
			close(sockFd);
			return;
		}
		
		cout << "Response from server: " << buffer << endl << endl;
	}
}

void ChatClient::sendRequestAndWaitForResponse(int sockFd, char* command){
	sendRequest(sockFd, command);
	
	char buffer[MAX_BUF_SIZE];
	waitForResponse(sockFd, buffer);
}

void ChatClient::start(){
	cout << "Welcome to Andy's Chat Application" << endl;
	cout << "==================================" << endl << endl;

	while(1){
		cout << "Menu" << endl;
		cout << "====" << endl;
		cout << "1. To create a new chat room: CREATE <room name>" << endl;
		cout << "2. To join an existing chat room: JOIN <room name>" << endl;
		cout << "3. To delete an existing chat room: DELETE <room name>" << endl;
		cout << "4. To exit the app: EXIT" << endl << endl;
		
		cout << "Command: ";
		char input[MAX_BUF_SIZE];
		memset(input, 0, MAX_BUF_SIZE);
		fgets(input, MAX_BUF_SIZE, stdin);
		
		string command = input;
		while(command.compare(0, 6, "CREATE") != 0 
			&& command.compare(0, 6, "DELETE") != 0 
			&& command.compare(0, 4, "JOIN") != 0
			&& command.compare(0, 4, "EXIT") != 0){
			cout << "Invalid command. Please try again!" << endl;
			cout << "Command: ";
			memset(input, 0, MAX_BUF_SIZE);
			fgets(input, MAX_BUF_SIZE, stdin);
			
			command = input;
		}
		
		handle(input);
	}
}