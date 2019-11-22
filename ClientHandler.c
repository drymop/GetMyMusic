#include "ClientHandler.h"
#include "Protocol.h"
#include "NetworkHeader.h"


static unsigned char* packet_buffer[BUFFSIZE];


/**
 * Helper functions
 */
int receive_packet(int client_socket);
int receive_until(int client_socket, int n_received, int target_len);
void close_client(struct ClientInfo* client_info);
int handle_signup(struct ClientInfo* client_info);





void handle_client(struct ClientInfo* client_info) {
    int packet_len = receive_packet(client_info->client_socket);
    if (packet_len <= 0) {
        // always close the session if any error happens
        close_client(client_info);
        return;
    }
    struct PacketHeader* header = (struct PacketHeader*)packet_buffer;
    
    // check if the header token is correct
    uint32_t session_token = ntohl(header->session_token);
    if(session_token != client_info->session_token) {
        close_client(client_info);
        return;
    }

    int err;
    switch (header->type) {
        case TYPE_SIGNUP:
            err = handle_signup(client_info);
            break;


        default:
            // unknown request type
            err = -1;
            break;
    }
}


int handle_signup(int packet_len, struct ClientInfo* client_info) {
    unsigned char* username = packet_buffer + HEADER_LEN;
    size_t username_len = strlen(username);
    unsigned char* password = username + username_len + 1;
    size_t password_len = strlen(password);

    // TODO verify existing account
    // TODO create user directory

    printf("New user: %s, password: %s", username, password);
    uint32_t token = 12345;

    memcpy(client_info->username, username, username_len + 1);
    client_info->session_token = token;
    return 0;
}



int receive_packet(int client_socket) {
    int n_received = 0;
    // since TCP doesn't have message boundary, we have to make sure that
    // the packet is received fully. In order to do this, we first read 
    // the full packet header, which contains the packet length, then
    // use that length as stopping condition for the rest of the loop.

    // receive the header, and find the packet length
    n_received = receive_until(client_socket, 0, HEADER_LEN);
    if (n_received <= 0) {
        // failed to receive data
        return -1;
    }
    struct PacketHeader* header = (struct PacketHeader*)packet_buffer;
    int packet_len = ntohs(header->packet_len);

    // receive the rest of the packet
    n_received = receive_until(client_socket, n_received, packet_len);
    // wrong packet length
    if (n_received != packet_len) {
        return -1;
    }
    return packet_len;
}

int receive_until(int client_socket, int n_received, int target_len) {
    while(n_received < target_len) {
        int n_new_bytes = recv(client_socket, packet_buffer + n_received, 
                               BUFFSIZE - n_received, 0);
        if (n_new_bytes <= 0) {
            // fail to recv
            return -1;
        }
        n_received += n_new_bytes;
    }
    return n_received;
}

void close_client(struct ClientInfo* client_info) {
    // release resource for socket
    close(client_info->client_socket);
    // clear client info
    memset(client_info, 0, sizeof(struct ClientInfo));
}



