// Immanuel Amirtharaj
// EmpiotNotifier.cpp

#ifndef EMPIOT_NOTIFIER_CPP
#define EMPIOT_NOTIFIER_CPP

#include "EmpiotNotifier.h"

EmpiotNotifier::EmpiotNotifier(string hName, int pNumber) {
	hostName = hName;
	portNumber = pNumber;
}

EmpiotNotifier::EmpiotNotifier() {
	hostName = "127.0.0.1";
	portNumber = 5000;
}


void EmpiotNotifier::sharedSend(char * message, int length) {
	int sock;
	struct sockaddr_in addr;
	struct hostent *server;
	int port;

	port = portNumber;

	sock =socket(AF_INET, SOCK_DGRAM, 0);

	server = gethostbyname(hostName);

	bzero((char *)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
   	bcopy((char *)server->h_addr, (char *)&addr.sin_addr, server->h_length);
	addr.sin_port = htons(port); 
	addr.sin_addr.s_addr = INADDR_ANY;

	sendto(sock, message, length, 0, (struct sockaddr *)&addr, sizeof(addr));
}

void EmpiotNotifier::startTesting() {
	char buffer [] = "start";
	sharedSend(buffer, sizeof(buffer));
}

void EmpiotNotifier::stopTesting() {
	char buffer[] = "stop";
	sharedSend(buffer, sizeof(buffer));

}

#endif
