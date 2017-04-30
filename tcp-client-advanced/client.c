/*
* @author: giuseppecanto
* @date: 2017-Apr
*
* tcp client with multiplexing
*/

/#include <stdio.h>
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

#define MAXBUFL 255
#define MAX_STR 1023
#define MSG_OK "+OK"
#define MSG_ERR "-ERR"

#ifdef DEBUG
#define debug(x) x
#else
#define debug(x)
#endif

char *prog_name;

int main (int argc, char *argv[]) {
	
	// Declaring variables
	int sockfd, err=0;
	char *dest_h, *dest_p;
	struct sockaddr_in daddr, *solvedaddr;
	int op1, op2, res, nconv;
	char buf[MAXBUFL];
	struct addrinfo *list;
	int err_getaddrinfo;

	// Check the input parameters
	if (argc != 3 || argc > 3) {
		printf("Usage: %s <dest_host> <dest_port>\n", argv[0]);
		printf("NOTE: use a port number within range [1024, 49151], both inclusive.\n");
		return 1;
	}

	// Set the parameters
	prog_name = argv[0];
	dest_h 	  = argv[1];
	dest_p    = argv[2];

	// Create the socket (end point communication)
	sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	debug(err_msg("[%s] I created a socket.", prog_name));

	// Set the struct sockaddr_in daddr for the destination
	Getaddrinfo(dest_h, dest_p, NULL, &list);
	solvedaddr = (struct sockaddr_in *) list->ai_addr;
	memset(&daddr, 0, sizeof(daddr));
	daddr.sin_family = AF_INET;
	daddr.sin_port = solvedaddr->sin_port;
	daddr.sin_addr.s_addr = solvedaddr->sin_addr.s_addr;

	// Connect to the destination using the previous daddr
	Connect(sockfd, (struct sockaddr *) &daddr, sizeof(daddr));
	debug(err_msg("[%s] I am now connected to: %s:%u.", prog_name, inet_ntoa(daddr.sin_addr), ntohs(daddr.sin_port)));
	printf("Client ready, waiting for commands (GET file, Q, A):\n");

	char c, command[MAXBUFL];
	int flag_reading_file = 0;
	int flag_end = 0;
	char fnamestr[MAX_STR+1];
	int filecnt = 0;  // this is needed to generate a new filename for writing
	uint32_t file_bytes;  // need to keep the info between different iterations in the next while
	FILE *fp;  // need to keep the info between different iterations in the next while
	int file_ptr;  // need to keep the info between different iterations in the next while
	time_t timestamp;

	while (1) {
	
		// Check whether I am not reading any file and someone asked to terminate with Q command	
		if ((flag_reading_file == 0) && (flag_end == 1))
			break;
		
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		FD_SET(fileno(stdin), &rset);  
		Select(sockfd+1, &rset, NULL, NULL, NULL); // This will block until something happens

		// Check if the socket is readable
		if (FD_ISSET(sockfd, &rset)) {

			/* If response from server is not trusted or network is unreliable,
			   do not perform calls to Read() without being sure
			   that they will not be blocked, i.e., go through select() again.
			   Here we trust that we will find \n soon */
			if (flag_reading_file == 0) {
				
				int nread = 0;
				
				do {
				
					Read(sockfd, &c, sizeof(char));
					buf[nread++] = c;

				} while ((c != '\n') && (nread < MAXBUFL-1));
				
				buf[nread] = '\0';
				debug(err_msg("[%s] --- I received the string: %s", prog_name, buf));
                        
				if ((nread >= strlen(MSG_OK)) && (strncmp(buf, MSG_OK, strlen(MSG_OK)) == 0)) {

					// Use a number, otherwise the requested filenames should be remembered
					sprintf(fnamestr, "down_%d", filecnt++);
					
					// Reading the number of bytes to be received
					Read(sockfd, buf, 4);
					file_bytes = ntohl((*(uint32_t *) buf));
					debug(err_msg("(%s) --- received file size '%u'",prog_name, file_bytes));

					// Reading the timestamp of the last modification of the file
					Read(sockfd, buf, 4);
					timestamp = ntohl((time_t) buf);
					debug(err_msg("[%s] --- I received the timestamp of the last modification. %d", prog_name, timestamp));

                       			// Open the file
					if ((fp = fopen(fnamestr, "wb")) != NULL) {
						debug(err_msg("(%s) --- opened file '%s' for writing", prog_name, fnamestr));
						file_ptr = 0;
						flag_reading_file = 1;
					} else 
						debug(err_quit("(%s) --- cannot open file '%s'",prog_name, fnamestr));

				} else if (nread >= strlen(MSG_ERR) && strncmp(buf,MSG_ERR,strlen(MSG_ERR))==0) 
					debug ( err_msg("(%s) - received '%s' from server: maybe a wrong request?", prog_name, buf) );
				  else
					debug ( err_quit("(%s) - protocol error: received response '%s'", prog_name, buf) );
		
			} else if (flag_reading_file==1) {
				
				Read (sockfd, &c, sizeof(char));
				fwrite(&c, sizeof(char), 1, fp);
				file_ptr++;

				if (file_ptr == file_bytes) {
					fclose(fp);
					debug( err_msg("(%s) --- received and wrote file '%s'",prog_name, fnamestr) );
					flag_reading_file = 0;
				}
			} else {
				debug ( err_quit("(%s) - flag_reading_file error '%d'", prog_name, flag_reading_file) );
			}
		}


		// Check if the stdin is readable
		if (FD_ISSET(fileno(stdin), &rset)) {
		
			// Check whether I do not received any input, and if yes terminate
			if ( (fgets(command, MAXBUFL, stdin)) == NULL) 
				command[0]='Q';
			
			// Check the input received
			switch(command[0]) {
				case 'G':
				case 'g':
					debug( err_msg("(%s) --- GET command sent to server",prog_name, fnamestr) );
					Write(sockfd, command, strlen(command));
					break;
				case 'Q':
				case 'q':
					printf("Q command received\n");
					sprintf(command, "QUIT\r\n");
					Write(sockfd, command, strlen(command));
					Shutdown(sockfd, SHUT_WR);
					flag_end = 1;
					err_msg("(%s) - waiting for file tranfer to terminate", prog_name);
					break;
				case 'A':
				case 'a':
					printf("A command received\n");
					Close (sockfd);   // This will give "connection reset by peer at the server side
					err_quit("(%s) - exiting immediately", prog_name);
					break;
				default:
					printf("Unknown command: %c\n", command[0]);
			}
		}
	}

	return 0;
}
