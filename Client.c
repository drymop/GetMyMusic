/**
 * GetMyMusic client's main program
 */

#include "NetworkHeader.h"
#include "Protocol.h"

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
 * Create a TCP socket connecting to the server at specified port
 * If an error happens, log error message and exit the program.
 * Return the socket file descriptor
 *
 * @param server      Server IP or server hostname
 * @param server_port Server port number (as a string)
 */
int create_socket(const char* server, const char* server_port);


/**
 * For now, the client simply prompt for login info, then send it to server
 * TODO: LOTS to implement
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
    int server_socket = create_socket(server, port);
    char packet[BUFFSIZE];

    /*
     * Logon/sign-up
     */
    // Prompt for username and password
    char username[128];
    printf("Enter username:\n");
    scanf("%s", username);
    char* password = getpass("Enter password:\n");
    printf("%s\n", username);
    printf("%s\n", password);
    // Create logon request
    ssize_t packet_len = make_logon_packet(packet, BUFFSIZE, 1, username, password);
    ssize_t sent_bytes = send(server_socket, packet, packet_len, 0);

    // Receive a session token
    packet_len = receive_packet(server_socket, packet, BUFFSIZE);
    if (packet_len <= 0) {
        printf("Error when receiving repsonse\n");
    }

    struct PacketHeader* header = (struct PacketHeader*) packet;
    uint32_t session_token = ntohl(header->session_token);
    printf("Token is %u\n", session_token);

    // Release resource and exit
    close(server_socket);
    return 0;
}


/*
 * Function implementations
 */


void die_with_error(const char* message, const char* detail) {
    printf("Error: %s\n", message);
    if (detail != NULL) { 
        printf("       %s\n", detail);
    }
    exit(1);
}


void parse_arguments(int argc, char* argv[], char** server, char** port) {
    static const char* USAGE_MESSAGE = 
            "Usage:\n ./client [-h <server>] [-p <port>]";
    
    // there must be an odd number of arguments (program name and flag-value pairs)
    if (argc % 2 == 0 || argc > 5) {
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


int create_socket(const char* server, const char* server_port) {
    /*
     * define criteria for address info
     */
    struct addrinfo addr_criteria;                    // struct storing the address info
    memset(&addr_criteria, 0, sizeof(addr_criteria)); // zero out the struct
    addr_criteria.ai_family = AF_UNSPEC;              // any address family
    addr_criteria.ai_socktype = SOCK_STREAM;          // only accept stream socket
    addr_criteria.ai_protocol = IPPROTO_TCP;          // only use TCP protocol      

    /* 
     * get a list of address infos that fit the criteria
     */
    struct addrinfo* server_addr; // the start of a linked list of possible addresses
    int err_code = getaddrinfo(server, server_port, &addr_criteria, &server_addr);
    if (err_code != 0) {
        die_with_error("Cannot get server's address info - getaddrinfo() failed", gai_strerror(err_code));
    }

    /*
     * try all addresses in the list of address infos
     * until successful connection is established
     */
    int server_socket = -1;
    struct addrinfo* addr;
    for (addr = server_addr; addr != NULL; addr = addr->ai_next) {
        // create socket from address info
        server_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (server_socket < 0) {
            continue; // fail to create socket, try other addresses
        }
        // initiate connection to server
        if (connect(server_socket, addr->ai_addr, addr->ai_addrlen) == 0) {
          break; // successfully connect to a socket
        }
        // fail to connect, close all resources and try other addresses
        close(server_socket);
        server_socket = -1;
    }

    /*
     * Clean up and return
     */
    // free the dynamically allocated list of address infos
    freeaddrinfo(server_addr);
    // die if cannot successfully connect with server
    if (server_socket < 0) {
        die_with_error("Failed to connect to server", "connect() failed");
    }

    return server_socket;
}



