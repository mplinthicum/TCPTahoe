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

#define SERV_PORT 2840
#define MAXBUFF   8

uint16_t parse_packet(char* packet);
void create_ack(char* dest, uint16_t seqnum);

struct sockaddr_in serveraddr;

int main(int argc, char **argv)
{
	int fd;
	char buffsend[MAXBUFF], buffrecv[MAXBUFF];
	
	/* Form the server address. */
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = SERV_PORT;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	/* Open the socket. */
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("RECEIVER socket creation error");
		exit(errno);
	}
	
	/* Connect to the server. */
	if (connect(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("RECEIVER socket connection error");
		exit(errno);
	}
	
	int last_received_seqnum = -5;
	
	/* Infinite loop. */
	while(1)
	{
		uint16_t seqnum = 0;
	
		/* Read data from the socket. */
		bzero(buffrecv, sizeof(buffrecv));
		if ((read(fd, buffrecv, 7)) < 0)
		{
			perror("RECEIVER socket read error");
			exit(errno);
		}
	
		/* Get the sequence number from the packet. */
		seqnum = parse_packet(buffrecv);
		
		/* Check to see if the sequence number is what it should be. */
		if(seqnum != last_received_seqnum + 5 && seqnum != 0)
		{
			/* Ignore the packet and resend the last ACK. */
			create_ack(buffsend, last_received_seqnum);
		} else {
			/* Create the ACK using the sequence number. */
			create_ack(buffsend, seqnum);
			last_received_seqnum = seqnum;
		}
		
		/* Wait one second. */
		sleep(1);
		
		/* Send the ACK. */
		if (write(fd, buffsend, sizeof(buffsend)) < 0)
		{
			perror("RECEIVER socket write error");
			exit(errno);
		}
	}
	
	/* Close the socket. */
	close(fd);
}

/* 
 * Parses the packet received from the sender and extracts the 
 * sequence number.
 */
uint16_t parse_packet(char* packet)
{
	uint8_t msb, lsb;
	
	msb = packet[0];
	lsb = packet[1]; 
	
	return (msb << 8) | lsb;
}

/*
 * Creates the ACK using the sequence number from the received
 * packet.
 */
void create_ack(char* dest, uint16_t seqnum)
{
	bzero(dest, sizeof(dest));
	dest[0] = (uint8_t) ((seqnum >> 8) & 0xff);
	dest[1] = (uint8_t) (seqnum & 0xff);
}
