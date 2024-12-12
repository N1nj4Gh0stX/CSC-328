/*************************************************************/
/* author: liz                                               */
/* filename: parsing.cpp                                      */
/* purpose: this source file implements the parsemenu function */
/*          that processes command-line arguments for a client.*/
/*          It parses the `-h` (hostname) and `-p` (port)      */
/*          options and stores them in a structure for further */
/*          use.                                               */
/*************************************************************/
#include <iostream>
#include <unistd.h>
#include "clientparse.h"


using namespace std;

struct options parsemenu(int argc, char* argv[]){
	struct options o;
	o.hostname = "";
	o.port = "";
	int opt;
	while((opt = getopt(argc, argv, "h:p:")) != -1){
		switch (opt){
			case 'h':
				o.hostname = optarg;
				// cout << "hostname: " << o.hostname << endl;
				break;
			
			case 'p':
				o.port = optarg;
				// cout << "port: " << o.port << endl;
				break;
			
		}
	}
	return o;
}
