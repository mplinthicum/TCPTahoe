#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "helper.h"

struct sockaddr_in serveraddr;

int main(int argc, char **argv)
{
	int fd;
	char buffrecv[MAXBUFF], buffsend[3], packet_text[5];
	
	/* Form the server address. */
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = SERV_PORT;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	/* Open the socket. */
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("RECEIVER socket creation error");
		exit(errno);
	}
	
	/* Connect to the server. */
	if(connect(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
		perror("RECEIVER socket connection error");
		exit(errno);
	}
	
	int seqnum = 0;
	
	while(1){
	
		/* Read from the server. */
		bzero(buffrecv, sizeof(buffrecv));
		if((read(fd, buffrecv, MAXBUFF)) < 0){
			perror("RECEIVER socket read error");
			exit(errno);
		}
		
		/* Get the sequence number and text from the received packet. */
		seqnum = parse_packet(buffrecv, packet_text);
		make_ack(buffsend, seqnum);
			
		/* Wait one second. */
		sleep(1);

		if(write(fd, buffsend, sizeof(buffsend)) < 0){
			perror("RECEIVER socket write error");
			exit(errno);
		}
	}
}
