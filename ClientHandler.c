#include "ClientHandler.h"
#include "Protocol.h"
#include "NetworkHeader.h"


/**
 * Global buffer for reading/writing packet
 */
static char packet_buffer[BUFFSIZE+1];


/*
 * Helper function declarations
 */

/**
 * Close the connection to a client, and clean the client's info
 * @param client_info Address of the struct storing the client's info
 */
void remove_client(struct ClientInfo* client_info);


/**
 * Handle a SIGNUP request
 * @param request_len Length of request packet
 * @param client_info Address of the client info struct
 */
ssize_t handle_signup(int request_len, struct ClientInfo* client_info);



void handle_client(struct ClientInfo* client_info) {
    ssize_t request_len = receive_packet(client_info->client_socket, packet_buffer, BUFFSIZE);
    if (request_len <= 0) {
        // always close the session if any error happens
        remove_client(client_info);
        return;
    }
    struct PacketHeader* header = (struct PacketHeader*)packet_buffer;
    
    // check if the header token is correct
    uint32_t session_token = ntohl(header->session_token);
    if(session_token != client_info->session_token) {
        remove_client(client_info);
        return;
    }

    // construct response packet
    ssize_t response_len;
    switch (header->type) {
        case TYPE_SIGNUP:
            response_len = handle_signup(request_len, client_info);
            break;
        default:
            // unknown request type
            response_len = -1;
            break;
    }
    // error handling client request
    if (response_len < 0) {
        remove_client(client_info);
        return;
    }

    // send back response packet
    ssize_t sent_bytes = send(client_info->client_socket, packet_buffer, response_len, 0);

    // TODO remove this after implement LEAVE feature for client
    remove_client(client_info);
}


/*
 * Helper function implementations
 */


ssize_t handle_signup(int request_len, struct ClientInfo* client_info) {
    char* request_end = packet_buffer + request_len;

    /*
     * Extract username and password from packet
     */
    char* username = packet_buffer + HEADER_LEN;
    size_t username_len = strlen(username) + 1;  // include null terminator
    char* password = username + username_len;
    if (password >= request_end) {
        // username is not null terminated properly
        return -1;
    }

    size_t password_len = strlen(password) + 1;  // include null terminator
    if (password + password_len != request_end) {
        // password is not null terminated properly
        return -1;
    }
    // save username to client_info
    memcpy(client_info->username, username, username_len);

    // TODO verify existing account
    // TODO create user directory

    printf("New user: %s, password: %s\n", username, password);

    /*
     * Response with session token
     */
    uint32_t token = 12345;

    client_info->session_token = token;

    // response contains user's session token
    return make_token_response(packet_buffer, BUFFSIZE, token);
}


void remove_client(struct ClientInfo* client_info) {
    // release resource for socket
    close(client_info->client_socket);
    // clear client info
    memset(client_info, 0, sizeof(struct ClientInfo));
}


