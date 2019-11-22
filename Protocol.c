#include "Protocol.h"

#include <arpa/inet.h>  /* htons, ntohs */
#include <stdio.h>      /* file IO */
#include <string.h>     /* memcpy */
#include <sys/types.h>





/**
 * Helper function to write packet header 
 */
void make_header(uint8_t* buffer, uint8_t type, uint16_t packet_len, uint32_t token) {
    struct PacketHeader* header = (struct PacketHeader*) buffer;
    header->version = VERSION;
    header->type = type;
    // muti-byte values must be in network endian (big-endian)
    header->packet_len = htons(packet_len);
    header->token = htonl(token);
}


ssize_t make_header_only_packet(uint8_t* buffer, size_t buff_len, uint8_t type, uint16_t token) {
    /* make sure buffer has enough length */
    if (buff_len < HEADER_LEN) {
        return -1;
    }
    make_header(buffer, type, HEADER_LEN, token);
    return HEADER_LEN;
}


/**
 * Make the logon packet containing user name and password
 * Return length of packet, or -1 if fail
 */
ssize_t make_logon_packet(uint8_t* buffer, 
                     size_t buff_len, 
                     bool is_new_account,
                     const char* username, 
                     const char* password) {

    /* make sure buffer has enough length */
    
    size_t user_len = strlen(username) + 1; // include null terminator
    size_t pass_len = strlen(password) + 1; // include null terminator
    size_t packet_len = HEADER_LEN + user_len + pass_len;
    // if buffer too small, return with error
    if (buff_len < packet_len) {
        return -1;
    }

    /* write header */

    uint8_t type = is_new_account? TYPE_SIGNUP : TYPE_LOGON;
    make_header(buffer, type, packet_len, 0);

    /* write data */

    // write user name (including null terminator)
    buffer += HEADER_LEN;
    memcpy(buffer, username, user_len);

    // write password (including null terminator)
    buffer += user_len;
    memcpy(buffer, password, pass_len);
    return packet_len;
}


/**
 * Make the packet indicating client is leaving the server
 * Return length of packet, or -1 if fail
 */
ssize_t make_leave_packet(uint8_t* buffer, size_t buff_len, uint16_t token) {
    return make_header_only_packet(buffer, buff_len, TYPE_LEAVE, token);
}


/**
 * Make the packet indicating client is leaving the server
 * Return length of packet, or -1 if fail
 */
ssize_t make_list_packet(uint8_t* buffer, size_t buff_len, uint16_t token) {
    return make_header_only_packet(buffer, buff_len, TYPE_LIST, token);
}


ssize_t make_download_packet(
        uint8_t* buffer, size_t buff_len, uint16_t token, const char* file_name) {
    size_t file_name_len = strlen(file_name) + 1;  // include null terminator
    size_t packet_len = HEADER_LEN + file_name_len;

    // make sure buffer is big enough for packet
    if (buff_len < packet_len) {
        return -1;
    }

    // write header and data
    make_header(buffer, TYPE_DOWNLOAD, packet_len, token);
    memcpy(buffer + HEADER_LEN, file_name, file_name_len);
    return packet_len;
}













