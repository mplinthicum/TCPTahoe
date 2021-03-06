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
	
	int seqnum = 0, previous_seqnum = -5;
	int to_ack[32];
	int i = 0, j = 0;
	int dupACKcount = 0;
	
	while(1){

		/* Fill a list with acks for every packet in the queue. */
		bzero(buffrecv, sizeof(buffrecv));	
		while(recv(fd, buffrecv, 7, MSG_PEEK | MSG_DONTWAIT) > 0){
		
			/* Read from the server. */
			bzero(buffrecv, sizeof(buffrecv));
			if(recv(fd, buffrecv, MAXBUFF, 0) < 0){
				perror("RECEIVER socket read error");
				exit(errno);
			}
			
			
			/* Get the sequence number and text from the received packet. */
			if(AddCongestion(0.1) != 1){
				seqnum = parse_packet(buffrecv, packet_text);
			}
			
			if(seqnum == (previous_seqnum + 5)){
				to_ack[i] = seqnum;
				previous_seqnum = seqnum;
			} else {
				to_ack[i] = previous_seqnum;
			}
			
			i++;
		}
			
		/* Wait one second. */
		sleep(1);
		
		/* Send acks for every packet in the queue. */
		for(j = 0; j < i; j++){
			make_ack(buffsend, to_ack[j]);

			if(write(fd, buffsend, sizeof(buffsend)) < 0){
				perror("RECEIVER socket write error");
				exit(errno);
			}
		}
		
		/* Reset the queue counter. */
		i = 0;
	}
}
