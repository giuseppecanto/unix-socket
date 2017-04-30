/*
 * @author: giuseppecanto
 * @date: 2017-Apr
 *
 * iterative tcp client
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include "errlib.h"
#include "sockwrap.h"

#define BUF_MAXLEN 255
#define MAX_STR 1023
#define MSG_OK 	 "+OK\r\n"
#define MSG_ERR  "-ERR\r\n"
#define MSG_QUIT "QUIT\r\n"

#ifdef DEBUG
#define debug(x) x
#else
#define debug(x)
#endif

char *prog_name;

int main (int argc, char *argv[]) {

	// Define variables
	int sockfd, err;
	char buf[BUF_MAXLEN], *dest_h, *dest_p, fname[BUF_MAXLEN-10];
	struct sockaddr_in destaddr, *solvedAddr;
	struct addrinfo *ai;

	// Initialize variables
	sockfd = err = 0;
	ai = NULL;

	// Check passed parameters and validate them
	if (!(argc == 3))
	  err_quit ("usage: %s <server_hostname> <server_port>", argv[0]);

	prog_name = argv[0];
	dest_h 	  = argv[1];
	dest_p	  = argv[2];

	// Get the address of the destination TCP server
	Getaddrinfo(dest_h, dest_p, NULL, &ai);
	solvedAddr = (struct sockaddr_in *) ai->ai_addr;

	// Create the socket (end point communication)
	sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	trace(err_msg("[%s] Socket created", prog_name));

	// Prepare informations for the connection
	memset(&destaddr, 0, sizeof(destaddr));
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = solvedAddr->sin_port;
	destaddr.sin_addr.s_addr = solvedAddr->sin_addr.s_addr;
	
	// Connect to the TCP SERVER
	Connect(sockfd, (struct sockaddr *)&destaddr, sizeof(destaddr));
	trace(err_msg("[%s] connected to %s:%u", prog_name,
		inet_ntoa(destaddr.sin_addr), ntohs(destaddr.sin_port)));

	while(1) {

		// Get the filename to ask and prepare the request
		printf("(%s) Type the file you want (exit with CTRL+D): \n", prog_name);
		char *res = fgets(fname, BUF_MAXLEN-10, stdin);

		// Check the fname whether it is not a invalid input
		if (res == NULL)
			break;

		sprintf(buf, "GET %s\r\n", fname);

		// Send the request
		Write(sockfd, buf, strlen(buf));
		trace(err_msg("(%s) Data has been sent", prog_name));

		fd_set rset;
		struct timeval tv;
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		if (select(sockfd+1, &rset, NULL, NULL, &tv) > 0) {

			int nread = 0;
			char c;
			time_t timestamp;

			do {
			  Read(sockfd, &c, sizeof(char));
			  buf[nread++]=c;
			} while (c!='\n' && nread<BUF_MAXLEN-1);

			trace(err_msg("[%s] Received string: %s", prog_name, buf));

			if ((nread >= strlen(MSG_OK)) && strncmp(buf, MSG_OK, strlen(MSG_OK)) == 0) {
				char fnamestr[MAX_STR+1];
				sprintf(fnamestr, "down_%s", fname);

				//Get the number of bytes to receive
				Read(sockfd, buf, 4);
				uint32_t file_bytes = ntohl((*(uint32_t *) buf));
				trace(err_msg("[%s] Received file size '%u'", prog_name, file_bytes));

				//Get the timestamp of the last modification
				Read(sockfd, buf, 4);
				timestamp = ntohl((time_t) buf);
				trace(err_msg("[%s] Received the timestamp of the last modification:%d.",
					prog_name, timestamp));

				FILE *fp;
				if ((fp=fopen(fnamestr, "wb")) != NULL) {
					char c;

					for (int i=0; i<file_bytes; i++) {
						Read(sockfd, &c, sizeof(char));
						fwrite(&c, sizeof(char), 1, fp);
					}

					fclose(fp);
					trace(err_msg("[%s] Received and wrote file: %s", prog_name, fnamestr));
				} else 
				    trace(err_msg("[%s] Cannot open file: %s", prog_name, fnamestr) );
			} else
			    trace (err_quit("[%s] Protocol error: received response: %s", prog_name, buf));
		} else 
		    printf("Timeout waiting for an answer from server\n");
	}

	// Closing the connection
	sprintf(buf, "%s", MSG_QUIT);
	Write(sockfd, buf, strlen(buf));
	Close (sockfd);

	return 0;
}
