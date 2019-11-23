#include "ClientHandler.h"

#include <stdbool.h>

#include "AuthenticationService.h"
#include "NetworkHeader.h"
#include "Protocol.h"


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
ssize_t handle_logon(int request_len, struct ClientInfo* client_info, bool is_new_user);


ssize_t handle_leave(struct ClientInfo* client_info);


/**
 * Generate a 32 bit random token. Warning: Not secure random.
 * Used because security is not considered in this project.
 */
uint32_t generate_random_token();


/*
 * Public function implementations
 */


void handle_client(struct ClientInfo* client_info) {
    ssize_t request_len = receive_packet(client_info->client_socket, packet_buffer, BUFFSIZE);
    if (request_len <= 0) {
        // always close the session if any error happens
        printf("Error when receiving packet\n");
        remove_client(client_info);
        return;
    }
    struct PacketHeader* header = (struct PacketHeader*)packet_buffer;
    
    // check if the header token is correct
    uint32_t session_token = header->session_token;
    if(session_token != client_info->session_token) {
        printf("Wrong session token!\n");
        remove_client(client_info);
        return;
    }

    // construct response packet
    ssize_t response_len;
    switch (header->type) {
        case TYPE_SIGNUP:
            response_len = handle_logon(request_len, client_info, true);
            break;
        case TYPE_LOGON:
            response_len = handle_logon(request_len, client_info, false);
            break;
        case TYPE_LEAVE:
            response_len = handle_leave(client_info);
            break;
        default:
            // unknown request type
            response_len = -1;
            break;
    }
    // error handling client request
    if (response_len <= 0) {
        remove_client(client_info);
        return;
    }

    // send back response packet
    ssize_t sent_bytes = send(client_info->client_socket, packet_buffer, response_len, 0);
}


/*
 * Helper function implementations
 */


ssize_t handle_logon(int request_len, struct ClientInfo* client_info, bool is_new_user) {
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

    /*
     * Validate user
     */
    if (is_new_user) {
        printf("User signup: %s, password: %s\n", username, password);
        if (!create_user(username, password)) {
            printf("User already exist!\n");
            return -1;
        }    
    } else {
        printf("User login: %s, password: %s\n", username, password);
        if (!check_user(username, password)) {
            printf("Wrong password!\n");
            return -1;
        }
    }
    
    // TODO create user directory

    // save username to client_info
    memcpy(client_info->username, username, username_len);

    /*
     * Response with session token
     */
    uint32_t token = generate_random_token();
    client_info->session_token = token;

    // response contains user's session token
    return make_token_response(packet_buffer, BUFFSIZE, token);
}


ssize_t handle_leave(struct ClientInfo* client_info) {
    printf("Client %s left\n", client_info->username);
    return -1;
}


void remove_client(struct ClientInfo* client_info) {
    // release resource for socket
    close(client_info->client_socket);
    // clear client info
    memset(client_info, 0, sizeof(struct ClientInfo));
}


uint32_t generate_random_token() {
    uint32_t x = rand() & 0xff;
    x |= (rand() & 0xff) << 8;
    x |= (rand() & 0xff) << 16;
    x |= (rand() & 0xff) << 24;
    return x;
}