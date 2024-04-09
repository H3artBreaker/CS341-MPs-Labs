// /**
//  * finding_filesystems
//  * CS 341 - Spring 2024
//  */
// #include "minixfs.h"
// #include "minixfs_utils.h"
// #include <assert.h>
// #include <stdio.h>

// int main(int argc, char *argv[]) {
//     int new_permissions = 0x124; // New permissions: 0444 (read-only for all)
//     file_system* mock_fs = NULL;

//     // Perform the operation
//     int result = minixfs_chmod(mock_fs, "/home/panyuec2/sp24_cs341_panyuec2/finding_filesystems/goodies/hello.txt", new_permissions);
    
//     // Check that the function succeeded
//     assert(result == 0);

//     inode* mock_inode = get_inode(mock_fs, "/home/panyuec2/sp24_cs341_panyuec2/finding_filesystems/goodies/hello.txt");

//     // Verify permissions were updated correctly
//     assert((mock_inode->mode & 0x1FF) == new_permissions);

//     // Verify the change time was updated
//     // (For simplicity, just check it's non-zero; in practice, compare against previous value)
//     assert(mock_inode->ctim.tv_sec != 0 || mock_inode->ctim.tv_nsec != 0);

//     printf("Test passed: Permissions were successfully updated.\n");
// }

#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Write tests here!
    file_system *fs = open_fs("test.fs");
    if (!fs) {
        return -1;
    }
    off_t off = 0;
    // char expected[14];
    // memset(expected, 0, 14);
    // ssize_t bytes_read = minixfs_read(fs, "/goodies/hello.txt", expected, 13, &off);
    // if (bytes_read == -1) {
    //     printf("fail 1\n");
    //     close_fs(&fs);
    //     return -1;
    // }

    // off = 0;
    // ssize_t bytes_written = minixfs_write(fs, "/newhello.txt", expected, 13, &off);
    // if (bytes_written == -1) {
    //     printf("fail 2\n");
    //     close_fs(&fs);
    //     return -1;
    // }
    off = 12;
    char buf[14];
    memset(buf, 0, 14);
    printf("buf at start: %s\n", buf);
    ssize_t bytes_read_out = minixfs_read(fs, "/goodies/hello.txt", buf, 12, &off);
    if (bytes_read_out == -1) {
        printf("fail 3\n");
        close_fs(&fs);
        return -1;
    }
    //open /goodies/hello.txt with open() or fopen() and read the contents way you normally would
    printf("buf: %s\n", buf);
    // if (!strcmp(buf, expected)) {
    //     printf("same\n");
    // } else {
    //     printf("different\n");
    //     printf("expected: %s\n", expected);
    //     printf("buf: %s\n", buf);
    // }
    close_fs(&fs);
    return 0;
}
