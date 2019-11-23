/**
 * The username - password hash pair are stored in a file, where each entry 
 * of the file contains an username and the corresponding password hash.
 * The username is padded with 0 until MAX_USERNAME_LEN + 1 (so it is always
 * null terminated), then the following 4 bytes are password hash
 */

#include "AuthenticationService.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


// MAX_USERNAME_LEN (63) + null terminator + 4-byte hash
#define MAX_LINE_LEN 68


static const char* DATABASE_FILE = "data/password.dat";


/**
 * Hash the password into a 32-bit integer
 * TODO Implement a better hash!
 */
uint32_t bad_hash(const char* password) {
	size_t len = strlen(password);
	int i;
	uint32_t hash = 0;
	for (i = 0; i < len; i++) {
		hash += password[i];
	}
	return hash;
}


bool check_user(const char* username, const char* password) {
	size_t username_len = strlen(username);
	size_t password_len = strlen(password);
	if (username_len <= 0 || username_len > MAX_USERNAME_LEN
			|| password_len <= 0 || password_len > MAX_PASSWORD_LEN) {
		return false; 
	}
	uint32_t hash = bad_hash(password);

	// check for username and password in database
	char cur_line[MAX_LINE_LEN];

	FILE* db_file = fopen(DATABASE_FILE, "rb");
	if (db_file == NULL) {
		return false;
	}
	while (fread(cur_line, 1, MAX_LINE_LEN, db_file) == MAX_LINE_LEN) {
		if (strcmp(cur_line, username) == 0) {
			uint32_t* correct_hash = (uint32_t*)(cur_line + MAX_USERNAME_LEN + 1);
			fclose(db_file);
			return (*correct_hash) == hash;
		}
	}
	fclose(db_file);
	return false;
}


bool create_user(const char* username, const char* password) {
	size_t username_len = strlen(username);
	size_t password_len = strlen(password);
	if (username_len <= 0 || username_len > MAX_USERNAME_LEN
			|| password_len <= 0 || password_len > MAX_PASSWORD_LEN) {
		return false; 
	}
	uint32_t hash = bad_hash(password);

	// make sure username doesn't already exist
	// if so add the username and hash to database
	FILE* db_file = fopen(DATABASE_FILE, "a+b");
	char cur_line[MAX_LINE_LEN];

	while (fread(cur_line, 1, MAX_LINE_LEN, db_file) == MAX_LINE_LEN) {
		if (strcmp(cur_line, username) == 0) {
			// username already exist
			fclose(db_file);
			return false;
		}
	}
	// zero out line
	memset(cur_line, 0, MAX_LINE_LEN);
	// start with username (null terminated)
	memcpy(cur_line, username, username_len + 1);
	// append hash
	memcpy(cur_line + MAX_USERNAME_LEN + 1, &hash, 4);
	fwrite(cur_line, 1, MAX_LINE_LEN, db_file);
	fclose(db_file);
	return true;
}