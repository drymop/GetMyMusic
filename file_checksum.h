#ifndef FILE_CHECKSUM_H_
#define FILE_CHECKSUM_H_


#include <stdio.h>   /* file IO */
#include <stdint.h>  /* integer types of exact size */


uint_fast32_t crc32_file_checksum(FILE *fd);


#endif // FILE_CHECKSUM_H_
