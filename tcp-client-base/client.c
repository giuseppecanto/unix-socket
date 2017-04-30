/*
* @author: giuseppcanto
* @date: 2017-Apr
*
* base tcp client
*/

#include <stdio.h>	
#include <sys/types.h> 
#include <sys/socket.h> 
#include <string.h> 	
#include <netinet/in.h> 
#include <stdlib.h> 	
#include <errno.h>
#include "errlib.h"
#include "sockwrap.h"

// GLOBAL VARIABLES
char* prog_name;

// MACROS
#ifdef DEBUG
#define debug(x) x
#else
#define debug(x)
#endif

int main(int argc, char* argv[]) {

	// Declaring variables
	int sockfd;
	char *dest_h, *dest_p;
	struct sockaddr_in daddr, *solvedaddr;
	struct addrinfo *list;

	// Check the input parameters
	if (!(argc == 3))
		err_quit("Usage: %s <dest_host> <dest_port>", argv[0]);

	// Set the parameters
	prog_name = argv[0];
	dest_h 	  = argv[1];
	dest_p    = argv[2];

	// Create the socket (end point communication)
	sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	debug(err_msg("(%s) I created a socket.", prog_name));

	// Set the struct sockaddr_in daddr for the destination
	Getaddrinfo(dest_h, dest_p, NULL, &list);
	solvedaddr = (struct sockaddr_in *) list->ai_addr;
	memset(&daddr, 0, sizeof(daddr));
	daddr.sin_family = AF_INET;
	daddr.sin_port = solvedaddr->sin_port;
	daddr.sin_addr.s_addr = solvedaddr->sin_addr.s_addr;

	// Connect to the destination using the previous daddr
	Connect(sockfd, (struct sockaddr *) &daddr, sizeof(daddr));
	err_msg("(%s) I am now connected to: %s:%u.", prog_name, inet_ntoa(daddr.sin_addr), ntohs(daddr.sin_port));
	freeaddrinfo(list);

	//Close connection and exit
	close(sockfd);
	return 0;
}
