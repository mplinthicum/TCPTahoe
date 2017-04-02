#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
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

	int fd, s, data_fd;
	int clientaddr_len = sizeof(clientaddr);
	char buffsend[MAXBUFF], buffrecv[3], buffdata[5];
	
	/* Open the file that holds the data to be sent. */
	if((data_fd = open("data.txt", O_RDONLY)) < 0){
		perror("SENDER data file open error");
		exit(errno);
	}

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
	
	/* Initialize some variables. */
	double cwnd = MSS, ssthresh = 16.0 * MSS;
	int dupACKcount = 0, previous_seqnum = -5, seqnum = 0, received_seqnum = 0, max_seqnum = 0;
	int index = 0, i;
	
	cwnd_packet cwnd_packets[1024];
	
	/* Set up the output file. */
	int output_file = open("cwnd_output.txt", O_RDWR | O_CREAT, 0666);
		
	/* Slow start. */
	while(1){
	
		/* Check for any timeouts. */
		/*for(i = received_seqnum + MSS; i < seqnum; i += MSS){
			long long diff = (long long)time(0) - cwnd_packets[i/MSS].send_time;
			if(diff >= 3L){
				printf("TIMEOUT\t seqnum: 0x%x\n", i);
				ssthresh = cwnd / 2;
				cwnd = MSS;
				index = 0;
				max_seqnum = seqnum;
				seqnum = i;
				break;
			}
		}*/
	
		/* There is room in the congestion window so send another packet. */
		if(index < cwnd){
		
			/* Create the packet and store it in the congestion window. */
			make_packet(buffsend, seqnum, data_fd);
			
			/* Save the packet to the congestion window array along with it's creation time. */
			save_packet(buffsend, &cwnd_packets[seqnum/MSS]);
			
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
		} else if(recv(s, buffrecv, 3, MSG_PEEK | MSG_DONTWAIT) > 0){
		
			/* Read from the receiver. */
			bzero(buffrecv, 3);
			if(read(s, buffrecv, sizeof(buffrecv)) < 0){
				perror("SENDER read error");
				exit(errno);
			}

			/* Get the contents of the ACK. */
			received_seqnum = parse_ack(buffrecv);
		
			printf("ACK: 0x%x\t %d\t", received_seqnum, received_seqnum);
			
			
			if(received_seqnum != (previous_seqnum + 5)){
				dupACKcount++;
				printf("Duplicate\n");
				
				if(dupACKcount > 3){
					printf("ENTER FAST RETRANSMIT\n");
					close(fd);
					close(s);
					return 0;
				}
				
			} else {
				printf("Correct\n");
			}
			
			previous_seqnum = received_seqnum;
			index = index - MSS;
			
			/* Slow start or congestion avoidance. */
			if(cwnd < ssthresh){
				cwnd += MSS;
			} else {
				cwnd += (MSS * MSS) / cwnd;
			}
			
			printf("cwnd: %0.2lf\n", cwnd / MSS);
			
			/* Write the cwnd value and system time to the file. */
			write_to_file(output_file, cwnd / MSS, (long long)time(0));
		}
	}
}
