/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>




ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    size_t total_written = 0;
    ssize_t byte_written = 0;
    while (total_written < count) {
        byte_written = write(socket, buffer + total_written, count - total_written);
        if (byte_written == -1 && errno == EINTR) {
            //error detected
            continue;   
        } else if (byte_written == 0) {
            //socket disconnected
            break;
        } else if (byte_written > 0){
            total_written += byte_written;
        } else {
            return -1;
        }
    }
    return (ssize_t)total_written;
}