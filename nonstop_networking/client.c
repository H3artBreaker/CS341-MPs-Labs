/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>


#include "common.h"
#define BUFFER_SIZE 2048

char **parse_args(int argc, char **argv);
verb check_args(char **args);
int read_from_server(int sock_fd, verb method, FILE* file_stream);
int main(int argc, char **argv) {
    // Good luck!
    char** parse_result = parse_args(argc, argv);
    verb method = check_args(parse_result);

    char* remote__server_host = parse_result[0];
    char* remote_server_port = parse_result[1];
    char* remoteFile = parse_result[3];
    char* localFile = parse_result[4];


    struct addrinfo hints;
    struct addrinfo *result;
    memset (&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET; // IPv4   (AF_INET6 for IPv6 addresses)
    hints.ai_socktype = SOCK_STREAM; // TCP like a POSIX pipes

    int s = getaddrinfo (remote__server_host, remote_server_port, &hints, &result);
    if (s != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        exit (1);
    }
    

    int sock_fd  = socket(result ->ai_family, result->ai_socktype, result->ai_protocol /* will be zero */);
    int connect_result = connect(sock_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    if (connect_result == -1) {    
        perror("connect has error");
        close(sock_fd);
        exit(1);
    }

    //puts("Connected!");

    char header[1024]; //VERB [filename]\n([File size])
    memset(header, 0, sizeof(header)); //clear the buffer
    FILE* stream = NULL;
    if (method == GET) {
        //sprintf(header, "%s %s\n", "GET", remoteFile);
        int header_len = snprintf(header, sizeof(header), "%s %s\n", "GET", remoteFile); //snprintf can prevent buffer overflow
        //write the request to the server through socket;
        if (header_len > 0) {
            write(sock_fd, header, header_len);
            shutdown(sock_fd, SHUT_WR);
        } else {
            perror("write header error in GET");
            exit(1);
        }
        stream = fopen(localFile , "w+");
        if (stream == NULL) {
            perror("FAILED TO FOPEN in GET");
            exit(1);
        }
    } 
    
    if (method == PUT) {
        struct stat st;
        size_t fileSize = 0;  //indicates how many bytes are present in the binary data in the request.
        if (stat(localFile, &st) == 0) {
            fileSize = st.st_size;
        } else {
            perror("STAT ERROR IN PUT");
            free(parse_result);
            print_invalid_response();
            exit(1);
        }
        // int header_len = snprintf(header, sizeof(header), "%s %s\n%zu", "PUT", remoteFile, fileSize);
        int header_len = snprintf(header, sizeof(header), "%s %s\n", "PUT", remoteFile);
        memcpy(header + header_len, &fileSize, sizeof(fileSize));
        header_len += sizeof(fileSize);
        if (header_len > 0) {
            write(sock_fd, header, header_len);
        } else {
            perror("write header error in GET");
            free(parse_result);
            close(sock_fd);
            exit(1);
        }
        //get the binary data of the localFile and write it to the socket.
        FILE* file = fopen(localFile, "rb");  // Open the local file in binary mode
        if (file == 0) {
            perror("FAILED TO OPEN FILE in PUT");
            fclose(file);
            close(sock_fd);
            free(parse_result);
            exit(1);
        }

        char read_buffer[BUFFER_SIZE];
        while (!feof(file)) {   
            size_t byte_read = fread(read_buffer, 1, BUFFER_SIZE, file); //read every blocks（buffer)
            if (byte_read > 0) { //read successfully
                size_t byte_written = write_all_to_socket(sock_fd, read_buffer, byte_read);
                if (byte_written == (size_t)-1) {
                    perror("FAILED TO WRITE TO SERVER SOCKET in PUT");
                    print_connection_closed();
                    fclose(file);
                    close(sock_fd);
                    free(parse_result);
                    exit(1);
                } 
            }
        }
        shutdown(sock_fd, SHUT_WR);
        fclose(file);
    }


    if (method == DELETE) {
        int header_len = snprintf(header, sizeof(header), "%s %s\n", "DELETE", remoteFile);
        //write the request to the server through socket;
        if (header_len > 0) {
            write(sock_fd, header, header_len);
            shutdown(sock_fd, SHUT_WR);
        } else {
            perror("write header error in DELETE");
            close(sock_fd);
            free(parse_result);
            exit(1);
        }
    }
    
    if (method == LIST) {
        int header_len = snprintf(header, sizeof(header), "%s\n", "LIST");
        //write the request to the server through socket;
        if (header_len > 0) {
            write(sock_fd, header, header_len);
            shutdown(sock_fd, SHUT_WR);
        } else {
            perror("write header error in LIST");
            exit(1);
        }
        stream = stdout;
    }

    int response_result = read_from_server(sock_fd, method, stream);
    if (method == GET) {
        fclose(stream);
    }
    close(sock_fd);
    if (response_result == -1) {
        free(parse_result);
        exit(1);
    } else if (response_result == -100) {
        print_too_little_data();
        free(parse_result);
        exit(1);
    } else if (response_result == -500) {
        print_received_too_much_data();
        free(parse_result);
        exit(1);
    }

    free(parse_result);
    // printf("HELLO");
    return 1;
}

/*
-1 has error
-100 too little
-500 too much
1 success for all

*/

int read_from_server(int sock_fd, verb method, FILE* file_stream) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int result = 0;
    if (read_all_from_socket(sock_fd, buffer, 3) != 3) {
        return -1;
    } 
    //check response if OK or ERROR
    if (strncmp(buffer, "OK\n", 3) == 0) {
        memset(buffer, 0, sizeof(buffer));
        size_t file_size;
        if (method == GET || method == LIST) {
            if (read_all_from_socket(sock_fd, buffer, sizeof(size_t)) != sizeof(size_t)) {
                return -1;  
            }
            memcpy(&file_size, buffer, sizeof(size_t));  // Copy the bytes into a size_t variable
            memset(buffer, 0, sizeof(buffer));

            size_t total_read = 0;
            while (total_read < file_size) {
                size_t byte_read = read_all_from_socket(sock_fd, buffer, BUFFER_SIZE); //read every blocks（buffer)
                if (byte_read == 0 && total_read < file_size) {
                    return -100;
                }
                if (byte_read == (size_t)-1) {
                    return -1;
                }
                if (byte_read > 0) { //read successfully
                    size_t byte_written = fwrite(buffer, 1, byte_read, file_stream);
                    if (byte_written != byte_read) {
                        return -1;
                    }
                }
                total_read += byte_read;
            }
            if (total_read > file_size || read_all_from_socket(sock_fd, buffer, BUFFER_SIZE) > 0) {
                return -500;
            }
        } else {
            print_success();
        }
        result = 1;
    } else if (strncmp(buffer, "ERR", 3) == 0) {
        read_all_from_socket(sock_fd, buffer + 3, 3); //clean the OR\n
        if (strncmp(buffer, "ERROR\n", 6) != 0) {
            print_invalid_response();
            return -1;
        }
        memset(buffer, 0, sizeof(buffer));

        char part[32];
        char* true_errmsg = NULL;
        size_t len = 0;
        size_t total_capacity = 0;

        while (1) {
            size_t byte_read = read_all_from_socket(sock_fd, part, sizeof(part));
            if (byte_read <= 0) break;  // Handle read error or disconnect

            char* new_line = memchr(part, '\n', byte_read);
            size_t length_to_copy = (new_line != NULL) ? (new_line - part + 1) : byte_read;

            if (len + length_to_copy > total_capacity) {
                // Ensure there is enough space
                total_capacity = len + length_to_copy;
                char* new_buff = realloc(true_errmsg, total_capacity);
                if (!new_buff) {
                    free(true_errmsg);
                    perror("Failed to allocate memory");
                    return -1;
                    //exit(EXIT_FAILURE);
                }
                true_errmsg = new_buff;
            }

            memcpy(true_errmsg + len, part, length_to_copy);
            len += length_to_copy;

            if (new_line != NULL) {
                break;  // Stop reading if newline found
            }
        }

        if (true_errmsg) {
            print_error_message(true_errmsg);
            free(true_errmsg);
        } 
        result = 1;
    } else {
        print_invalid_response();
        return -1;
    }
    
    return result;
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
