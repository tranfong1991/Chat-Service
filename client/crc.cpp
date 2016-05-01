#include "ChatClient.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	if(argc < 3){
		cout<<"Please enter the host name and port number."<<endl;
		return 0;
	}

	char* hostName = argv[1];
	int portNo = atoi(argv[2]);

	ChatClient client(hostName, portNo);
	client.start();
	
	return 0;
}