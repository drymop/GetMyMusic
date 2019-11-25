/**
 * 
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>     /* integer of fixed size */
#include <sys/types.h>

#include "StorageService.h"


/** Protocol version */
static const uint8_t VERSION = 0x1;

/* 
 * Packet types 
 */

enum PacketType {
    TYPE_SIGNUP_REQUEST = 1,
    TYPE_LOGON_REQUEST,
    TYPE_TOKEN_RESPONSE,
    TYPE_LEAVE_REQUEST,
    TYPE_LIST_REQUEST,
    TYPE_LIST_RESPONSE,
    TYPE_FILE_REQUEST,
    TYPE_FILE_TRANSFER,
};

/**
 * Common packet header
 */
struct PacketHeader {
    /** Protocol version */
    uint8_t  version;
    /** Request type */
    uint8_t  type;
    /** Token specific to an user and a session */
    uint16_t packet_len;
    /** Token specific to an user and a session */
    uint32_t session_token;
};

static const size_t HEADER_LEN = sizeof(struct PacketHeader);


ssize_t receive_packet(int socket ,char* buffer, size_t buff_len);

/**
 * Make the logon packet containing user name and password
 * Return length of packet, or -1 if fail
 */
ssize_t make_logon_request(char* buffer, 
                          size_t buff_len, 
                          bool is_new_account,
                          const char* username, 
                          const char* password);


ssize_t make_token_response(char* buffer, size_t buff_len, uint32_t token);


/**
 * Make the packet indicating client is leaving the server
 * @return Length of packet, or -1 if error
 */
ssize_t make_leave_request(char* buffer, size_t buff_len, uint32_t token);


ssize_t make_list_request(char* buffer, size_t buff_len, uint32_t token);


ssize_t make_list_response(char* buffer, size_t buff_len, uint32_t token, 
        struct FileInfo* file_info, int n_files);


#endif // PROTOCOL_H_