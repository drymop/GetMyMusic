/**
 * 
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>     /* integer of fixed size */
#include <sys/types.h>


/** Protocol version */
static const uint8_t VERSION = 0x1;

/* 
 * Packet types 
 */

enum PacketType {
    TYPE_SIGNUP   = 1,
    TYPE_LOGON,
    TYPE_LEAVE,
    TYPE_LIST,
    TYPE_UPLOAD,
    TYPE_DOWNLOAD,
    TYPE_TOKEN
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
ssize_t make_logon_packet(char* buffer, 
                     	  size_t buff_len, 
                          bool is_new_account,
                     	  const char* username, 
                     	  const char* password);


ssize_t make_token_response(char* buffer, size_t buff_len, uint32_t token);



#endif // PROTOCOL_H_