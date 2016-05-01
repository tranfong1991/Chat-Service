#include <iostream>
#include "ChatServer.h"

using namespace std;

int main(int argc, char *argv[]){
	if(argc == 1){
		cout<<"Please enter the port number."<<endl;
		return 0;
	}

	const char* port = argv[1];
	
	ChatServer chatServer(atoi(port));
	chatServer.start();
}