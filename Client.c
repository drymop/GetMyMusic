/**
 * Run a client that send a query to a Who server, and print out
 * the list of users/login times received from the server.
 */

#include <errno.h>
#include <signal.h>
#include <time.h>

#include "NetworkHeader.h"


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
    parse_arguments(argc, argv, &server, &port);


    /*
     * Initialize socket and IO buffer
     */

    // struct addrinfo *server_addr;  // hold info of the server address
    // int server_socket = create_socket(server, port, &server_addr);

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


void parse_arguments(int argc, char* argv[], char** server, char** port) {
    static const char* USAGE_MESSAGE = 
            "Usage:\n ./client [-h <server>] [-p <port>]";
    
    // there must be an odd number of arguments (program name and flag-value pairs)
    if (argc > 3) {
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
            default:   // unknown flag
                die_with_error(USAGE_MESSAGE, "Unknown flag");
        }
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



