// Immanuel Amirtharaj
// EmpiotNotifier.h

#ifndef EMPIOT_NOTIFIER_H
#define EMPIOT_NOTIFIER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>

using namespace std;

class EmpiotNotifier {
	public:
	EmpiotNotifier();
	EmpiotNotifier(string, int);
	void startTesting();
	void stopTesting();

	private:
	string hostName;
	int portNumber;
	void sharedSend(char *, int);
};

#endif
