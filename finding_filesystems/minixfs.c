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
#include <string.h>

data_block* get_block(file_system *fs, inode* ind, uint64_t idx);

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
    //return NULL if inode already exists or * cannot be created.
    if (get_inode(fs, path)) {
        return NULL;
    }
    if (valid_filename(path) != 1) {
        return NULL;
    }

    const char* filename = NULL;
    inode* parent = parent_directory(fs, path, &filename);
    inode_number ndNum = first_unused_inode(fs);
    if (ndNum == -1) {
        perror("NO ENOUGH INODE");
        return NULL;
    }
    inode* new_node = fs->inode_root + ndNum;
    init_inode(parent, new_node);
    //add a new dirent to the parent and then increase the parent's size.
    //minixfs_dirent new_dirent = {(char*)filename, ndNum};
    int parBlockNum = parent->size / sizeof(data_block);
    //int remainder = parent->size % sizeof(data_block);
    
    if (parBlockNum >= NUM_DIRECT_BLOCKS && parent->indirect == UNASSIGNED_NODE) { //if all direct blocks are used
        if (add_single_indirect_block(fs, parent) == -1) {
            perror("Failed addng indirect block in parent");
            return NULL;
        }
    }
    
    clock_gettime(CLOCK_REALTIME, &(parent->atim));
    return new_node;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        //first generate the block_info_string parameter(num_used_blocks)
        char* dm = GET_DATA_MAP(fs->meta);
        int used = 0;
        for (size_t i = 0; i < fs->meta->dblock_count; i++){ 
            if (dm[i] == 1) {
                used++;
            }
        }
        char* content = block_info_string(used);
        if ((unsigned long)*off > strlen(content)) {
            return 0;
        }
        size_t cpyBytes;
        if (count > strlen(content) - *off) {
            cpyBytes = strlen(content) - *off;
        } else {
            cpyBytes = count;
        }
        memcpy(buf, content + *off, cpyBytes);
        *off += cpyBytes;
        return cpyBytes;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    //size_t max = NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS;
    uint64_t required_blocks = ceil((double)(count + *off) / (double)sizeof(data_block)); 
    int result = minixfs_min_blockcount(fs, path, required_blocks); //check if we have enough space in current inode for the writing
    if (result == -1) {
        perror("min_blockcount ERROR");
        errno = ENOSPC;
        return -1;
    }

    inode* fileInode = get_inode(fs, path);

    if (is_file(fileInode) == 0) {
        perror("NOT A FILE");
        return -1;
    }

    int blockIndex = *off / sizeof(data_block); 
    size_t cpyBytes = 0, bytes_written = 0;
    if (fileInode->size < *off + count) {
        fileInode->size = *off + count;
    }

    while (bytes_written < count) {
        int blockOffset = *off % sizeof(data_block);
        //if we reach the end of the file
        if (sizeof(data_block) - blockOffset < sizeof(data_block)) { //when we start with part of the db 
            cpyBytes = (count > sizeof(data_block) - blockOffset) ? sizeof(data_block) - blockOffset : count;
        } else if (count - bytes_written >= sizeof(data_block)) { //need to write a whole block
            cpyBytes = sizeof(data_block);
        } else {
            cpyBytes = count - bytes_written;
        }

        data_block* db = get_block(fs, fileInode, blockIndex);
        if (db == NULL) break;
        memcpy((db->data) + blockOffset, buf + bytes_written, cpyBytes); //cpy from buffer

        bytes_written += cpyBytes;
        blockIndex++;
        *off += cpyBytes;
    }
    
    
    clock_gettime(CLOCK_REALTIME, &(fileInode->atim));
    clock_gettime(CLOCK_REALTIME, &(fileInode->mtim));
    return count;
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

    int blockIndex = *off / sizeof(data_block); 
    size_t cpyBytes = 0, bytes_read = 0;

    while (bytes_read < count) {
        int blockOffset = *off % sizeof(data_block);
        //if we reach the end of the file
        if (sizeof(data_block) - blockOffset < sizeof(data_block)) { //when we start with part of the db 
            cpyBytes = (count > sizeof(data_block) - blockOffset) ? sizeof(data_block) - blockOffset : count;
        } else if (count - bytes_read >= sizeof(data_block)) { //need to read a whole block
            cpyBytes = sizeof(data_block);
        } else {
            cpyBytes = count - bytes_read;
        }

        data_block* db = get_block(fs, fileInode, blockIndex);
        if (db == NULL) break;
        memcpy(buf + bytes_read, (db->data) + blockOffset, cpyBytes); //cpy at the correct block place
        printf("buf in loop: %s\n", (char*)buf);

        bytes_read += cpyBytes;
        blockIndex++;
        *off += cpyBytes;
    }

    clock_gettime(CLOCK_REALTIME, &(fileInode->atim));

    return bytes_read;
}

data_block* get_block(file_system *fs, inode* ind, uint64_t idx) { //need to check the algorithm here
    if (idx < NUM_DIRECT_BLOCKS) {
        return fs->data_root + ind->direct[idx];
    } 
    if (idx - NUM_DIRECT_BLOCKS >= NUM_INDIRECT_BLOCKS) { //do I need to add 1 here?
        return NULL;
    } 
    data_block* indiBlock = fs->data_root + ind->indirect;
    data_block_number* crtIdx = (data_block_number*)indiBlock + (idx - NUM_DIRECT_BLOCKS);
    return fs->data_root + *crtIdx;
}