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

#define MAXBUFF   8
#define SERV_PORT 2840

int random_string(char* dest);
void create_packet(char* dest, uint16_t seqnum);
uint16_t parse_packet(char* packet);
uint16_t parse_ack(char* ack);

typedef struct
{
	uint16_t seqnum;
	char data[6];
} packet;

struct sockaddr_in serveraddr, clientaddr;

int main(int argc, char **argv)
{
	int fd, s;
	int clientaddr_len = sizeof(clientaddr);
	packet packets[16];
	
	char buffsend[MAXBUFF], buffrecv[MAXBUFF], randstring[6];

	/* Form the server address. */
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = SERV_PORT;
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    /* Open the socket. */
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("SENDER socket creation error");
        exit(errno);
    }

    /* Bind the socket to serveraddr. */
    if(bind(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("SENDER socket bind error");
        exit(errno);
    }

    /* Listen for incoming connections. */
    listen(fd, 2);
    
    /* Wait for and accept connection from receiver. */
    if ((s = accept(fd, (struct sockaddr*)&clientaddr, &clientaddr_len)) < 0)
	{
		perror("SENDER accept error");
		exit(errno);
	}
	
	int cwnd = 1, tmpcwnd = 1, i = 0, j = 0;
	int numstored = 0, last_seqnum = -5, need_retransmit = 0;
	int num_dup_acks = 0;
	uint16_t seqnum = 0; 
	
	/* Infinite loop. */
	while(1)
	{
		printf("\nSENDING...\n");
		
		/* Send packets until cwnd is full. */
		for(i = 0; i < cwnd; i++)
		{
		
			if(!need_retransmit)
			{
				printf("Sender is sending new data...\n");
			
				/* Create the packet. */
				create_packet(buffsend, seqnum);
		
				/* Store the packet information in an array in case it is lost. */
				packets[i].seqnum = seqnum;
				strcpy(packets[i].data, buffsend + 2);
				
				printf("sent data: %s\n", buffsend + 2);
				printf("sent seqnum: 0x%x\n", seqnum);
				
				/* Byte increment the sequence number. */
				seqnum += 5;
				
				/* Increment the number of stored packets. */
				numstored++;
				
			} else {
				printf("Sender is retransmitting...\n");
			
				seqnum = packets[j].seqnum;
			
				create_packet(buffsend, packets[j - 4].seqnum);
				strcpy(buffsend + 2, packets[j - 4].data);
			}	
			
			printf("numstored: %d\n", numstored);
	
			/* Send the packet to the receiver. */
			if (write(s, buffsend, 7) < 0)
			{
				perror("SENDER write error");
				exit(errno);
			}
			
			if(seqnum == 25)
			{
				/* Intoduce error. */
				seqnum++;
			}
		}
		
		printf("\nRECEIVING...\n");
		
		/* Wait for ACKS from the receiver. */
		for(j = 0; j < tmpcwnd; j++)
		{
		
			printf("stored data %d: %s\n", j, packets[j].data);
			printf("stored seqnum: 0x%x\n", packets[j].seqnum);
		
			/* Read the ACK from the receiver. */
			bzero(buffrecv, sizeof(buffrecv));
			if (read(s, buffrecv, sizeof(buffrecv)) < 0)
			{
				perror("SENDER read error");
				exit(errno);
			}
		
			/* Get the sequence number. */
			uint16_t recvseqnum = parse_ack(buffrecv);
			
			if(recvseqnum != last_seqnum + 5)
			{
			
				/* Increment the number of duplicate acks received. */
				num_dup_acks++;
				printf("number of duplicate acks: %d\n", num_dup_acks);
				
				if(num_dup_acks > 3)
				{
					/* Update the retransmit flag to resend stored data. */
					need_retransmit = 1;
					break;
				}
			} else {
				
				/* Update the last received sequence number. */
				last_seqnum = recvseqnum;
				
				/* Set the retransmit flag to indicate the ACK was received successfully. */
				need_retransmit = 0;
			
				/* Increment the congestion window. */
				cwnd++;
				
				/* Reset the number of duplicate acks. */
				num_dup_acks = 0;
				
				/* Decrease the number of stored data. */
				numstored--;
			}
		}
		tmpcwnd = cwnd;
		printf("CWND: %d\n", cwnd);
	}
    
    /* Close the socket. */
    close(fd);
    return fd;
}


/*** HELPER FUNCTIONS ***/

/* 
 * Creates a random string of length 5.
 */
int random_string(char* dest)
{
	char char_set[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i = 0, index = 0;
	
	for(i = 2; i < 7; i++)
	{
		index = rand() % 61;
		dest[i] = char_set[index];
	}
	
	return 0;
}

/*
 * Creates a packet using with a sequence number and a random string.
 */
void create_packet(char* dest, uint16_t seqnum)
{
	bzero(dest, sizeof(dest));
	dest[0] = (uint8_t) ((seqnum >> 8) & 0xff);
	dest[1] = (uint8_t) (seqnum & 0xff);
	
	random_string(dest);
}

/*
 * Get the sequence number from the ACK.
 */
uint16_t parse_ack(char* ack)
{
	uint8_t msb, lsb;
	
	msb = ack[0];
	lsb = ack[1];
	
	return (msb << 8) | lsb;
}
