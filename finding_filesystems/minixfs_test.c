/**
 * finding_filesystems
 * CS 341 - Spring 2024
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int new_permissions = 0x124; // New permissions: 0444 (read-only for all)
    file_system* mock_fs = NULL;

    // Perform the operation
    int result = minixfs_chmod(mock_fs, "/home/panyuec2/sp24_cs341_panyuec2/finding_filesystems/goodies/hello.txt", new_permissions);
    
    // Check that the function succeeded
    assert(result == 0);

    inode* mock_inode = get_inode(mock_fs, "/home/panyuec2/sp24_cs341_panyuec2/finding_filesystems/goodies/hello.txt");

    // Verify permissions were updated correctly
    assert((mock_inode->mode & 0x1FF) == new_permissions);

    // Verify the change time was updated
    // (For simplicity, just check it's non-zero; in practice, compare against previous value)
    assert(mock_inode->ctim.tv_sec != 0 || mock_inode->ctim.tv_nsec != 0);

    printf("Test passed: Permissions were successfully updated.\n");
}
