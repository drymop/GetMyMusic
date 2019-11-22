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

/** Create a new account */
static const uint8_t TYPE_SIGNUP   = 1;

/** Login to account */
static const uint8_t TYPE_LOGON    = 2;

/** Close the connection to server */
static const uint8_t TYPE_LEAVE    = 3;

/** Request list of files stored in server */
static const uint8_t TYPE_LIST     = 4;

/** Upload file to server */
static const uint8_t TYPE_UPLOAD   = 5;

/** Request to download file from server */
static const uint8_t TYPE_DOWNLOAD = 6;


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
    uint32_t token;
};

static const size_t HEADER_LEN = sizeof(struct PacketHeader);


/**
 * Make the logon packet containing user name and password
 * Return length of packet, or -1 if fail
 */
ssize_t make_logon_packet(uint8_t* buffer, 
                     	  size_t buff_len, 
                          bool is_new_account,
                     	  const char* username, 
                     	  const char* password);


#endif // PROTOCOL_H_