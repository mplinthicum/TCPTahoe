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

/*
 * This routine is called with a probability of bit error, p, and
 * returns 1 if there is congestion, and 0 otherwise.
 */
int AddCongestion(double p){
	int c, seed;
	double r;
	long M = RAND_MAX;
	
	seed = (int) time(NULL);
	srandom(seed);
	c = 0;
	r = (double)random()/M;
	
	if (r < p) c = 1;
	
	return c;
}

/* 
 * Creates a random string of length 5 and sets it to 
 * bytes 1 - 6 (0 indexed) in the packet character array.
 * The first 2 bytes hold the sequence number.
 */
void random_string(char* dest){
	char char_set[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i = 0, index = 0;
	
	for(i = 2; i < 7; i++){
		index = rand() % 61;
		dest[i] = char_set[index];
	}
}

/*
 * Copies the packet contents from a saved packet to a buffer.
 * Also updates the send_time.
 */
void copy_packet(char* dest, cwnd_packet* packet){
	bzero(dest, sizeof(dest));
	dest[0] = (uint8_t) ((packet->seqnum >> 8) & 0xff);
	dest[1] = (uint8_t) (packet->seqnum & 0xff);
	strcpy(dest + 2, packet->data);
	packet->send_time = (long long)time(0);
}

/*
 * Sets the first 2 bytes of the packet character array to 
 * a two byte (uint_16t) sequence number.  The next five bytes
 * are then set to a random sequence of characters.
 */
void make_packet(char* dest, uint16_t seqnum){
	bzero(dest, sizeof(dest));
	dest[0] = (uint8_t) ((seqnum >> 8) & 0xff);
	dest[1] = (uint8_t) (seqnum & 0xff);
	random_string(dest);
}

/*
 * Sets the first 2 bytes of the ACK to a two byte
 * (uint_16t) sequence number.  Very similar to the
 * make_packet function.
 */
void make_ack(char*dest, uint16_t seqnum){
	bzero(dest, sizeof(dest));
	dest[0] = (uint8_t) ((seqnum >> 8) & 0xff);
	dest[1] = (uint8_t) (seqnum & 0xff);
}

/*
 * Gets the sequence number from the first 2 bytes of
 * the packet and returns it to type uint16_t.  Sets the
 * 5 bytes of random text to dest.
 */
uint16_t parse_packet(char* packet, char* dest){
	uint8_t msb, lsb;
	
	bzero(dest, sizeof(dest));
	strcpy(dest, packet + 2);
	
	msb = packet[0];
	lsb = packet[1];
	
	return (msb << 8) | lsb;
}

/*
 * Copies the contents of a packet.
 */
void save_packet(char* packet, cwnd_packet* dest){
	dest->seqnum = parse_packet(packet, dest->data);
	dest->send_time = (long long)time(0);
}

/*
 * Gets the sequence number from the ACK.  Very similar
 * to the parse_packet function.
 */
uint16_t parse_ack(char* packet){
	uint8_t msb, lsb;
	
	msb = packet[0];
	lsb = packet[1];
	
	return (msb << 8) | lsb;
}
