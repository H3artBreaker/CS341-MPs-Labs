/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    size_t total_read = 0;
    ssize_t byte_read = 0;
    while (total_read < count) {
        byte_read = read(socket, buffer + total_read, count - total_read);
        //printf("byte_read func is %s\n", buffer);
        if (byte_read == -1 && errno == EINTR) {
            //error detected
            continue;   
        } else if (byte_read == 0) {
            //socket disconnected
            break;
        } else if (byte_read > 0){
            total_read += byte_read;
        } else {
            return -1;
        }
    }
    return (ssize_t)total_read;
}





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