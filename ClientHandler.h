/**
 * Contains functions to handle client requests
 */

#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_


#include <stdint.h>

static const int USERNAME_LEN = 128;
static const int MAX_CONNECTIONS = 16;

struct ClientInfo {
	int client_socket;
	char username[USERNAME_LEN + 1];
	uint32_t session_token;
};




#endif // CLIENT_HANDLER_H_