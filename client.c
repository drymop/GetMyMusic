/**
 * Run a client that send a query to a Who server, and print out
 * the list of users/login times received from the server.
 */

#include <errno.h>
#include <signal.h>
#include <time.h>

/**
 * Print out the error, then exit the program
 * detail can be NULL, in which case no additional detail is printed
 *
 * @param message Error message to be printed out
 * @param detail  Details about the error, can be NULL
 */
void die_with_error(const char* message, const char* detail);


/**
 * Parse the argument supplied to the main program
 * and extract arguments into output parameters.
 * If arguments are invalid (missing required arguments,
 * invalid flags, etc.) exit the program
 *
 * @param argc        Number of command line arguments
 * @param argv        Array of command line arguments
 * @param server      [out] Address of the variable to store server IP
 * @param port        [out] Address of the variable to store the port string
 */
void parse_arguments(int argc, char* argv[], char** server, char** port);


/**
 * Create a socket talking to the server at specified port
 * If an error happens, log error message and exit the program.
 * Return the socket file descriptor
 * Server address info is returned through output parameter server_addr
 * This method dynamically allocate memory for server_addr, 
 * so when finish using  server_addr, freeaddrinfo(server_addr) needs 
 * to be called to free up the allocated memory.
 *
 * @param server      Server IP or server hostname
 * @param server_port Server port number (as string)
 * @param server_addr [out] Address of the variable to store the server's address info
 */
int create_socket(const char* server, const char* server_port, struct addrinfo** server_addr);


/**
 * Send a request packet to the Who server, querying for a host name
 * If an error happens, simply ignore and return.
 *
 * @param server_socket Socket file descriptor of the server
 * @param server_addr   Server address info
 * @param packet_buffer Empty buffer that will be used to store the packet
 * @param buffer_len    Max length of the packet buffer
 * @param query_id      Unique ID for this query
 * @param hostname      Hostname to be queried
 */
void send_who_request(int server_socket, 
											const struct addrinfo* server_addr, 
											char* packet_buffer, 
											int buffer_len, 
											unsigned short query_id, 
											const char* hostname);

/**
 * Deserialize the response packet, validate the packet, 
 * then print out the packet's content.
 * Return 0 if sucessfully handles packet, or -1 if error happends
 * (for example: corrupted packet, wrong query ID, etc.)
 * 
 * @param packet_buffer Buffer that currently contains the serialized
 *                      response packet received from server
 * @param packet_len    Number of bytes of the serialized response packet
 * @param query_id      The expected query ID 
 */
int handle_who_response(char* packet_buffer, int packet_len, unsigned short query_id);


/**
 * Verify if a response packet received from server is well-formed.
 * 
 * @param packet   The packet struct
 * @param query_id The expected query ID
 */
int verify_packet(const struct WhoPacket packet, unsigned short query_id);


/**
 * Handle the alarm signal when an alarm goes off.
 * Simply set the interrupted flag.
 */
void handle_alarm_signal(int ignored);


/** indicate whether alarm has gone off */
int interrupted;


/**
 * Connect to the Who server specified in the command line argument.
 * Query the hostname (specified in command line argument) to the server.
 * Wait until receiving a list of users from the server, the print out the users.
 * If timeout, resend the query until max number of retries is exceeded.
 */
int main (int argc, char *argv[]) {

  /*
   * Parse arguments supplied to main program
   */

  char* server = SERVER_HOST; // init with default value
  char* port = SERVER_PORT;   // init with default value
  char* hostname;
	unsigned int timeout; // in seconds
	int max_retries;      // number of retries before giving up
	parse_arguments(argc, argv, &server, &port, &timeout, &max_retries, &hostname);

	/*
   * Setup alarm handler so that the program can use alarm later
   */
  
  struct sigaction handler; // signal handler info
  handler.sa_handler = handle_alarm_signal;
  // handler will block all signals
  if (sigfillset(&handler.sa_mask) < 0) {
  	die_with_error("Fail to set alarm handler", "sigfillset() failed");
  }
  // set handler to the correct signal (SIGALRM)
  handler.sa_flags = 0;
  if (sigaction(SIGALRM, &handler, 0) < 0) {
  	die_with_error("Fail to set alarm handler", "sigaction() failed");
  }

  /*
   * Initialize socket and IO buffer
   */

  struct addrinfo *server_addr;  // hold info of the server address
  int server_socket = create_socket(server, port, &server_addr);

  const int buffer_len = BUFFSIZE;
  char buffer[buffer_len]; // IO buffer

  /*
   * Repeatedly: (until the max number of retries is reached)
   * - Send query to server
   * - Try to receive response from server until succeeded, or timeout
   */

  // start with a random query ID
  srand(time(0));  // set random seed
  unsigned short query_id = rand();

  struct sockaddr_storage from_addr;  // will store address of sender
	socklen_t from_addr_len = sizeof(from_addr);

	int success = 0;  // flag to indicate whether a valid response has been received 

	// repeatedly sending query to server until receiving a valid response 
  // (or exceeding number of retries)
  int n_retries;
	for (n_retries = 0; n_retries <= max_retries; n_retries++) {

		// send query to server
  	send_who_request(server_socket, server_addr, buffer, buffer_len, query_id, hostname);
  	interrupted = 0;  // timeout alarm has not gone off yet
  	alarm(timeout);   // set timeout alarm to go off after specified timeout amount

    // try to receive response until succeeded, or time out
  	while (1) {
  		// try to receive response
  		int packet_len = recvfrom(
	      	server_socket, buffer, buffer_len, 0, 
	      	(struct sockaddr*) &from_addr, &from_addr_len);

      // If received some response and handled response successfully
      // then exit the loop
  		if (packet_len >= 0 
  			  && handle_who_response(buffer, packet_len, query_id) == 0) {
				alarm(0);     // cancel alarm
				success = 1;  // specify that a valid response is received
        break;        // stop waiting for more repsonses
  		}

      // If get to here: invalid packet, or alarm goes off during recvfrom()
  		
      // Check to see if alarm went off (timeout), if so stop listening for response
      if (interrupted) {
        query_id += 1;  // use a different query ID when retrying
        printf("Timed out, %d retries remaining ...\n", max_retries - n_retries);
  			break;          // stop waiting for more response
  		}
  	}

  	// already received a valid response, no need to resend query
  	if (success) {
  		break;
  	}
	}

	if (!success) {
		die_with_error("No response from server.", "Max number of retries exceeded");
	}

  /*
   * Release resource and exit
   */

  // free dynamically allocated memory from create_socket()
  freeaddrinfo(server_addr);
  
  // exit
  return 0;
}


/*
 * Function implementations
 */


void die_with_error(const char* message, const char* detail) {
  printf("Error: %s\n", message);
  if (detail != NULL) { 
    printf("Detail: %s\n", detail);
  }
  exit(1);
}


void parse_arguments(
		int argc, char* argv[], char** server, char** port, 
		unsigned int* timeout, int* max_retries, char** hostname
) {
  static const char* USAGE_MESSAGE = 
  		"Usage:\n ./Project3Client [-h <server>] [-p <port>] -t <timeout> -i <max-retries> -d <hostname>";
  
  // flag specifying whether required arguments are encountered
  int has_timeout = 0;
  int has_retries = 0;
  int has_hostname = 0;
  
  // there must be an odd number of arguments
  if (argc % 2 == 0) {
  	die_with_error(USAGE_MESSAGE, NULL);
  }

  // argument list must have the form:
  // program_name -flag1 value1 -flag2 value ...
  int i;
  for (i = 1; i < argc; i+= 2) {
    // argv[i] must be a flag
    // argv[i+1] must be the flag's value

    // check for flag
    if (argv[i][0] != '-') {
      die_with_error(USAGE_MESSAGE, NULL);
    }

    // assign value to variable associated with the flag
    char* value = argv[i+1];
    switch (argv[i][1]) {
    	case 'h':  // server IP or hostname
    		*server = value;
    		break;
    	case 'p':  // server port string
    		*port = value;
    		break;
    	case 't':  // timeout (in seconds)
    		*timeout = atoi(value);
    		has_timeout = 1;
    		break;
    	case 'i':  // max number of retries
    		*max_retries = atoi(value);
    		has_retries = 1;
    		break;
    	case 'd':  // queried hostname
    		*hostname = value;
    		has_hostname = 1;
    		break;
    	default:   // unknown flag
      	die_with_error(USAGE_MESSAGE, "Unknown flag");
    }
  }

  // check if having all required arguments
  if (!(has_timeout && has_retries && has_hostname)) {
    die_with_error(USAGE_MESSAGE, "Missing required arguments");
  }
}


int create_socket(const char* server, const char* server_port, struct addrinfo** server_addr) {
  /*
   * define criteria for address info
   */
  struct addrinfo addr_criteria;                    // struct storing the address info
  memset(&addr_criteria, 0, sizeof(addr_criteria)); // zero out the struct
  addr_criteria.ai_family = AF_UNSPEC;              // any address family
  addr_criteria.ai_socktype = SOCK_DGRAM;           // only accept datagram socket
  addr_criteria.ai_protocol = IPPROTO_UDP;          // only use UDP protocol      

  /* 
   * get a list of address infos that fit the criteria
   */
  int err_code = getaddrinfo(server, server_port, &addr_criteria, server_addr);
  if (err_code != 0) {
    die_with_error("getaddrinfo() failed", gai_strerror(err_code));
  }

  /*
   * create UDP socket
   */
  int server_socket = socket((*server_addr)->ai_family, 
                             (*server_addr)->ai_socktype, 
                             (*server_addr)->ai_protocol);
  if (server_socket < 0) {
    die_with_error("socket() failed", NULL);
  }
  return server_socket;
}


void send_who_request(int server_socket, 
											const struct addrinfo* server_addr, 
											char* packet_buffer, 
											int buffer_len, 
											unsigned short query_id, 
											const char* hostname) {
	// construct packet header and data
	struct WhoPacket packet;
	packet.version = VERSION;
	packet.queryId = query_id;
	packet.isResponse = 0;      // packet type is request, not response
	packet.isHostNameFound = 0; // field not used for request packet
	packet.nEntries = 1;        // data field has 1 entry (which is hostname)
	packet.data = hostname;

	// serialize packet into a byte array (stored in packet buffer)
	int packet_len = serialize_packet(packet_buffer, buffer_len, packet);
	
	// send packet byte array to server
	sendto(
			server_socket, packet_buffer, packet_len, 0,
			server_addr->ai_addr, server_addr->ai_addrlen);
}


int handle_who_response(char* packet_buffer, int packet_len, unsigned short query_id) {
	struct WhoPacket packet; // info about response packet
  // convert byte array into packet struct
  if (deserialize_packet(packet_buffer, packet_len, &packet) < 0) {
  	// stop if fail to deserialize packet
  	return -1;
  }
  if (verify_packet(packet, query_id)) {
    // packet is malformed
    return -1;
  }

  // If get to here, response packet is valid.
  // Print out the response from server.

  if (!packet.isHostNameFound) {
	  // the queried hostname is not found in database
  	printf("Invalid hostname\n");
  } else if (packet.nEntries == 0) {
  	// hostname found, but no active users
  	printf("No active users on this hostname\n");
  } else {
  	// hostname found with active users
  	// print out the table of username and login time

    // pointer to current entry (username, login time) in packet's data
  	const char* entry = packet.data; 
 
  	// buffer to store username with null terminator
  	char username_buffer[RESPONSE_USER_LEN + 1];
  	username_buffer[RESPONSE_USER_LEN] = 0; // null terminator

    // print table titles
  	printf("%-16s%s\n", "Username", "Time");
  	// print table entries
    int i;
  	for (i = 0; i < packet.nEntries; i++) {
  		// copy the bytes of username in current entry to a null-terminated buffer
  		memcpy(username_buffer, entry, RESPONSE_USER_LEN);
  		// concatenate the 4 bytes (in big-endian order) to form login time
  		const char* login_time_bytes = entry + RESPONSE_USER_LEN;
  		unsigned int login_time = (login_time_bytes[0] & 0xFF) << 24 |
  															(login_time_bytes[1] & 0xFF) << 16 |
  															(login_time_bytes[2] & 0xFF) << 8  |
  															(login_time_bytes[3] & 0xFF);

  		printf("%-16s%u\n", username_buffer, login_time);
  		entry += RESPONSE_ENTRY_LEN; // next entry
  	}
  }

  return 0; // successfully handle response
} 


int verify_packet(const struct WhoPacket packet, unsigned short query_id) {
  if (packet.version != VERSION) {
    // check version
    return -1;
  } else if (!packet.isResponse) {
    // check type (must be response rather than request)
    return -1;
  } else if (packet.queryId != query_id) {
    // check if packet ID match the expected query ID
  	return -1;
  }
  return 0;  // packet is valid
}


void handle_alarm_signal(int ignored) {
	interrupted = 1;
}



