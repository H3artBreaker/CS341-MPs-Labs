/**
 * charming_chatroom
 * CS 341 - Spring 2024
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
    // Your code here
    int32_t n = htonl(size); //int32_t is to ensure the data is in 32 bits over the network
    return write_all_to_socket(socket, (char *)&n, MESSAGE_SIZE_DIGITS); //change to char* for easier transfering data byte by byte later
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    size_t total_read = 0;
    ssize_t byte_read = 0;
    while (total_read < count) {
        byte_read = read(socket, buffer + total_read, count - total_read);
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
