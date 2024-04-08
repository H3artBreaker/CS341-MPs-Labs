/**
 * finding_filesystems
 * CS 341 - Spring 2024
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) { //need to write test cases to check it
    inode* ind = get_inode(fs, path);
    if (ind == NULL) {
        perror("INODE DOES NOT EXIST");
        errno = ENOENT;
        return -1;
    }
    //change the permission
    ind->mode &= ~((1 << RWX_BITS_NUMBER) - 1); //reset the lower bits 0 - 8 to 1 first and then 0
    ind->mode |= (new_permissions & ((1 << RWX_BITS_NUMBER) - 1)); 
    clock_gettime(CLOCK_REALTIME, &(ind->ctim));
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    inode* ind = get_inode(fs, path);
    if (ind == NULL) {
        perror("INODE DOES NOT EXIST");
        errno = ENOENT;
        return -1;
    }

    if (owner != ((uid_t)-1)) {
        ind->uid = owner;
    }

    if (group != ((uid_t)-1)) {
        ind->gid = group;
    }

    clock_gettime(CLOCK_REALTIME, &(ind->ctim));

    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    return NULL;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    return -1;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);

    inode* fileInode = get_inode(fs, path);
    if (fileInode == NULL) {
        perror("INODE DOES NOT EXIST");
        errno = ENOENT;
        return -1;
    }

    if (buf == 0) { 
        perror("BUFFER IS NULL ERROR");
        return -1;
    }

    if (is_file(fileInode) == 0) {
        perror("NOT A FILE");
        return -1;
    }

    if ((uint64_t)*off > fileInode->size) { 
        return 0;
    }

    if (count > (size_t)(fileInode->size - (uint64_t)*off)) {
        count = (size_t)(fileInode->size - (uint64_t)*off);
    }

    // int offsetBlock = (*off / sizeof(data_block)) + 1; //
    // int readBlocks = ceil((double)count / (double)sizeof(data_block));
    // int read_indiBlock;
    // if (offsetBlock + readBlocks > NUM_DIRECT_BLOCKS) {
    //     read_indiBlock = 1;
    // }

    






    return bytes_read;
}
