#define SERV_PORT 2841
#define MAXBUFF   8
#define MSS		  5


/*
 * A struct to hold the creation time, sequence number, and 
 * data of a packet.  For easy storage and refernce.
 */
typedef struct
{
	time_t send_time;
	int seqnum;
	char data[6];
} cwnd_packet;



/*
 * This routine is called with a probability of bit error, p, and
 * returns 1 if there is congestion, and 0 otherwise.
 */
int AddCongestion(double p);

/* 
 * Creates a random string of length 5 and sets it to 
 * bytes 1 - 6 (0 indexed) in the packet character array.
 * The first 2 bytes hold the sequence number.
 */
void random_string(char* dest);

/*
 * Sets the first 2 bytes of the packet character array to 
 * a two byte (uint_16t) sequence number.  The next five bytes
 * are then set to a random sequence of characters.
 */
void make_packet(char* dest, uint16_t seqnum);

/*
 * Sets the first 2 bytes of the ACK to a two byte
 * (uint_16t) sequence number.  Very similar to the
 * make_packet function.
 */
void make_ack(char*dest, uint16_t seqnum);

/*
 * Gets the sequence number from the first 2 bytes of
 * the packet and returns it to type uint16_t.  Sets the
 * 5 bytes of random text to dest.
 */
uint16_t parse_packet(char* packet, char* dest);

/*
 * Copies the contents of a packet.
 */
void save_packet(char* packet, cwnd_packet* dest);

/*
 * Gets the sequence number from the ACK.  Very similar
 * to the parse_packet function.
 */
uint16_t parse_ack(char* packet);