#ifndef STORAGE_SERVICE_H_
#define STORAGE_SERVICE_H_


#include "FileChecksum.h"


#define MAX_FILE_NAME_LEN 256 // this includes null-terminator


/**
 * Define a linked list node to contain all file infos
 */
struct FileInfo {
	char name[MAX_FILE_NAME_LEN];
	uint32_t checksum;
	struct FileInfo* next_info;
};


/**
 * Find the info of all regular files in the given directory
 * @param  dir_path  path to directory
 * @return Pointer to a FileInfo struct, which represents the
 *         head of a linked list of FileInfo.
 *         The memory for the linked list is dynamically allocated,
 *         so user must call free_file_info on the returned pointer
 *         after finishes using the linked list.
 */
struct FileInfo* list_files(const char* dir_path);


/**
 * Free the dynamically allocated list of file info
 */
void free_file_info_list(struct FileInfo* file_info_list);


#endif // STORAGE_SERVICE_H_
