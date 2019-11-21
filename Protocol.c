#include "Protocol.h"

#include <arpa/inet.h>  /* htons, ntohs */
#include <stdio.h>      /* file IO */
#include <string.h>     /* memcpy */


typedef unsigned char uchar;
typedef unsigned short ushort;

/** Protocol version */
static const uchar VERSION = 0x1;

/* 
 * Packet types 
 */

/** Create a new account */
static const uchar TYPE_SIGNUP   = 1;

/** Login to account */
static const uchar TYPE_LOGON    = 2;

/** Close the connection to server */
static const uchar TYPE_LEAVE    = 3;

/** Request list of files stored in server */
static const uchar TYPE_LIST     = 4;

/** Upload file to server */
static const uchar TYPE_UPLOAD   = 5;

/** Request to download file from server */
static const uchar TYPE_DOWNLOAD = 6;


/**
 * Common packet header
 */
struct PacketHeader {
    /** Protocol version */
    uchar  version;
    /** Request type */
    uchar  type;
    /** Token specific to an user and a session */
    ushort token;
};


/**
 * Helper function to write packet header 
 */
void make_header(uchar* buffer, uchar type, ushort token) {
    struct PacketHeader* header = (struct PacketHeader*) buffer;
    header->version = VERSION;
    header->type = type;
    // muti-byte values must be in network order (big-endian)
    header->token = htons(token);
}


int make_header_only_packet(uchar* buffer, size_t buff_len, uchar type, ushort token) {
    /* make sure buffer has enough length */
    if (buff_len < sizeof(struct PacketHeader)) {
        return -1;
    }
    make_header(buffer, type, token);
    return sizeof(struct PacketHeader);
}


/**
 * Make the logon packet containing user name and password
 * Return length of packet, or -1 if fail
 */
int make_logon_packet(uchar* buffer, 
                     size_t buff_len, 
                     bool is_new_account,
                     const char* username, 
                     const char* password) {

    /* make sure buffer has enough length */
    
    size_t user_len = strlen(username) + 1; // include null terminator
    size_t pass_len = strlen(password) + 1; // include null terminator
    size_t packet_len = sizeof(struct PacketHeader) + user_len + pass_len;
    // if buffer too small, return with error
    if (buff_len < packet_len) {
        return -1;
    }

    /* write header */

    uchar type = is_new_account? TYPE_SIGNUP : TYPE_LOGON;
    make_header(buffer, type, 0);

    /* write data */

    // write user name (including null terminator)
    buffer += sizeof(struct PacketHeader);
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
int make_leave_packet(uchar* buffer, size_t buff_len, ushort token) {
    return make_header_only_packet(buffer, buff_len, TYPE_LEAVE, token);
}


/**
 * Make the packet indicating client is leaving the server
 * Return length of packet, or -1 if fail
 */
int make_list_packet(uchar* buffer, size_t buff_len, ushort token) {
    return make_header_only_packet(buffer, buff_len, TYPE_LIST, token);
}


int make_download_packet(
        uchar* buffer, size_t buff_len, ushort token, const char* file_name) {
    size_t file_name_len = strlen(file_name) + 1;  // include null terminator
    size_t packet_len = sizeof(struct PacketHeader) + file_name_len;

    // make sure buffer is big enough for packet
    if (buff_len < packet_len) {
        return -1;
    }

    // write header and data
    make_header(buffer, TYPE_DOWNLOAD, token);
    memcpy(buffer + sizeof(struct PacketHeader), file_name, file_name_len);
    return packet_len;
}













