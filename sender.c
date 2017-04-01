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
#include <time.h>

#include "helper.h"

struct sockaddr_in serveraddr, clientaddr;

int main(int argc, char **argv){

	int fd, s;
	int clientaddr_len = sizeof(clientaddr);
	char buffsend[MAXBUFF], buffrecv[3];

	/* Form the server address. */
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = SERV_PORT;
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    /* Open the socket. */
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("SENDER socket creation error");
        exit(errno);
    }

    /* Bind the socket to serveraddr. */
    if(bind(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
        perror("SENDER socket bind error");
        exit(errno);
    }

    /* Listen for incoming connections. */
    listen(fd, 2);
    
    /* Wait for and accept connection from receiver. */
    if((s = accept(fd, (struct sockaddr*)&clientaddr, &clientaddr_len)) < 0){
		perror("SENDER accept error");
		exit(errno);
	}
	
	double cwnd = MSS, ssthresh = 16.0 * MSS;
	int dupACKcount = 0;
	int previous_seqnum = -5, seqnum = 0, received_seqnum = 0, max_seqnum = 0;
	int index = 0, i;
	
	cwnd_packet cwnd_packets[256];
	
	while(1){
		
		/* Slow start. */
		while(1){
		
			/* Check for any timeouts. */
			for(i = received_seqnum + MSS; i < seqnum; i += MSS){
				long long diff = (long long)time(0) - cwnd_packets[i/MSS].send_time;
				//printf("Checking timeout\t seqnum: %d\t data: %s\t diff: %lld\t received_seqnum: %d\n", cwnd_packets[i/MSS].seqnum, cwnd_packets[i/MSS].data, diff, received_seqnum);
				if(diff >= 3L){
					printf("TIMEOUT\t seqnum: 0x%x - %d\t data: %s\t diff: %lld\t received_seqnum: %d\n", cwnd_packets[i/MSS].seqnum, cwnd_packets[i/MSS].seqnum, cwnd_packets[i/MSS].data, diff, received_seqnum);
					cwnd = MSS;
					index = 0;
					max_seqnum = seqnum;
					ssthresh = cwnd / 2;
					seqnum = i;
				}
			}
		
			/* There is room in the congestion window so send another packet. */
			if(index < cwnd){
			
				if(max_seqnum > seqnum) {
					copy_packet(buffsend, &cwnd_packets[seqnum/MSS]);
				} else {
					/* Create the packet and store it in the congestion window. */
					make_packet(buffsend, seqnum);
					
					/* Save the packet to the congestion window array along with it's creation time. */
					save_packet(buffsend, &cwnd_packets[seqnum / MSS]);
				}
				
				
				printf("Packet sent: 0x%x %d %s %lld\n", cwnd_packets[seqnum/MSS].seqnum, cwnd_packets[seqnum/MSS].seqnum, cwnd_packets[seqnum/MSS].data, (long long)cwnd_packets[seqnum/MSS].send_time);
			
				/* Write to the receiver. */
				if(write(s, buffsend, MAXBUFF) < 0){
					perror("SENDER write error");
					exit(errno);
				}
		
				/* Byte increment the sequence number. */
				seqnum += MSS;
				
				/* Byte increment the index (total number of packets in cwnd). */
				index += MSS;
				
			/* There is not room in the congestion window so read ACKs. */
			} else {
			
				/* Read from the receiver. */
				bzero(buffrecv, 3);
				if(read(s, buffrecv, sizeof(buffrecv)) < 0){
					perror("SENDER read error");
					exit(errno);
				}
	
				/* Get the contents of the ACK. */
				received_seqnum = parse_ack(buffrecv);
			
				printf("ACK: 0x%x\t %d\t", received_seqnum, received_seqnum);
				
				printf("Correct\n");
				previous_seqnum = received_seqnum;
				dupACKcount = 0;
				index = index - MSS;
				
				/* Slow start or congestion avoidance. */
				if(cwnd < ssthresh){
					cwnd += MSS;
				} else {
					cwnd += (MSS * MSS) / cwnd;
				}
				
				printf("cwnd: %0.2lf\n", cwnd / MSS);
			}
		}
		
		/* Congestion control. */
		
		/* Fast retransmit. */
	}
}
