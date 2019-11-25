#include "StorageService.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#define DATABASE_DIR "data"


/**
 * Check if the path refers to a regular file (as opposed to a directory/sym link/etc.)
 */
bool is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}


struct FileInfo* list_user_files(const char* username, int* n_files) {
	/*
	 * Construct path to user's directory
	 */
	size_t db_dir_len = strlen(DATABASE_DIR);
	size_t username_len = strlen(username);
	// dir path is "<DATABASE_DIR>/<username>"
	// 2 extra char for slash and null terminator
	char* dir_path = malloc(username_len + db_dir_len + 2); 
	memcpy(dir_path, DATABASE_DIR, db_dir_len);
	dir_path[db_dir_len] = '/';
	// end path with username and null terminator
	memcpy(dir_path + db_dir_len + 1, username, username_len + 1);

	/*
	 * Get a list of all user files
	 */
	struct FileInfo* file_list = list_files(dir_path, n_files);

	free(dir_path);
	return file_list;
}



struct FileInfo* list_files(const char* dir_path, int* n_files) {
	*n_files = 0;
	
	// open the directory 
	DIR* dir;
	struct dirent* entry;
	dir = opendir(dir_path);
	if (dir == NULL) {
		return NULL;
	}

	// prepare buffer to concat dir_path with file name in dir
	// in the form "<dir_path>/<file_name>"
	size_t dir_path_len = strlen(dir_path);
	char* file_path = malloc(dir_path_len + 1 + MAX_FILE_NAME_LEN);
	memcpy(file_path, dir_path, dir_path_len);
	file_path[dir_path_len] = '/';
	char* file_path_name_part = file_path + dir_path_len + 1;

	// go through all regular files in directory, store the name and checksum
	// in a linked list
	struct FileInfo* info_list = NULL;
	while ((entry = readdir(dir)) != NULL) {
		// create the path to this file (stored in file_path buffer)
		memcpy(file_path_name_part, entry->d_name, MAX_FILE_NAME_LEN);
		
		// if not regular file, skip this entry
		if (!is_regular_file(file_path)) {
			continue;
		}

		// create a new linked list node to store file info
		struct FileInfo* node = malloc(sizeof(struct FileInfo));
		// store name
		memcpy(node->name, entry->d_name, MAX_FILE_NAME_LEN);
		// store checksum
		FILE* fd = fopen(file_path, "r");
		node->checksum = crc32_file_checksum(fd);
		fclose(fd);

		// add node to linked list
		// here, we add the node to the top of list, because it's easier
		node->next_info = info_list;
		info_list = node;
		(*n_files)++;
	}

	free(file_path);
	return info_list;
}


void free_file_info(struct FileInfo* file_info_list) {
	struct FileInfo* head = file_info_list;
	while (file_info_list != NULL) {
		head = file_info_list;
		file_info_list = file_info_list->next_info;
		free(head);
	}
}