/**
 * Contains functions to handle client requests
 */

#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_


#include <stdint.h>

#define USERNAME_LEN 128
#define USERNAME_LEN_WITH_NULL 129
#define MAX_CONNECTIONS 1


/**
 * Contains the session info of a currently connected client
 */
struct ClientInfo {
	int client_socket;
	char username[USERNAME_LEN_WITH_NULL];
	uint32_t session_token;
};


/**
 * Initialize
 */
void initialize_client_handler();


/**
 * Handle a client request, and update the client info if needed
 */
void handle_client(struct ClientInfo* client_info);

#endif // CLIENT_HANDLER_H_