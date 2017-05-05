/*
 * @author: giuseppecanto
 * @date: 2017-Apr
 * 
 * tcp client working with xdr format
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include "errlib.h"
#include "sockwrap.h"
#include <rpc/xdr.h>

#ifdef DEBUG
#define debug(x) x
#else
#define debug(x)
#endif
#define BUF_MAXL 255

char *prog_name;
int main (int argc, char *argv[]) {

	// Define variables
	int sockfd, op1, op2, res;
	char *ip, *port, buf[BUF_MAXL];
	struct sockaddr_in destaddr, *solvedaddr;
	struct addrinfo *ai;
	XDR xdrs;
	XDR xdrs_r;

	// Check passed parameters
	if (!(argc == 3))
		err_quit ("usage: %s <ip> <port>", argv[0]);

	// Initialize variables
	prog_name	= argv[0];
	ip		= argv[1];
	port		= argv[2];

	// Get the address from typed ip/port
	Getaddrinfo(ip, port, NULL, &ai);
	solvedaddr = (struct sockaddr_in *) ai->ai_addr;
	
	// Create the endpoint communication
	sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	debug(err_msg("(%s) socket created", prog_name));

	// Connect to the destination host/service
	memset(&destaddr, 0, sizeof(destaddr));
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = solvedaddr->sin_port;
	destaddr.sin_addr.s_addr = solvedaddr->sin_addr.s_addr;
	Connect(sockfd, (struct sockaddr *) &destaddr, sizeof(destaddr));
	debug(err_msg("(%s) connected to %s:%u", prog_name,
		inet_ntoa(destaddr.sin_addr), ntohs(destaddr.sin_port)));
	freeaddrinfo(ai);

	// Get data to send from input
	printf("Please insert the first operand to send to the server: ");
	scanf("%d", &op1);
	printf("Please insert the second operand to send to the server: ");
	scanf("%d", &op2);

	// Send data encoding as XDR
	xdrmem_create(&xdrs, buf, BUF_MAXL, XDR_ENCODE);
	xdr_int(&xdrs, &op1);
	xdr_int(&xdrs, &op2);	
	Write(sockfd, buf, xdr_getpos(&xdrs));
	xdr_destroy(&xdrs);
	sprintf(buf, "%d %d\n", op1, op2);
	debug(err_msg("(%s) data has been sent", prog_name));

	// Read data from stream decoding from XDR
	FILE *stream_socket_r = fdopen(sockfd, "r");
	if (stream_socket_r == NULL)
	  err_quit ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs_r, stream_socket_r, XDR_DECODE); 
	if (!(xdr_int(&xdrs_r, &res)))
	  debug(err_msg("(%s) - cannot read res with XDR ...", prog_name) );
	else
	  err_msg("(%s) sum: %d", prog_name, res);
	
	// Close
	Close (sockfd);
	return 0;
}
