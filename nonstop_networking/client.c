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

    puts("Connected!");

    char header[1024]; //VERB [filename]\n([File size])
    memset(header, 0, sizeof(header)); //clear the buffer
    
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
        FILE* lf_stream = fopen(localFile , "w+");
        if (lf_stream == NULL) {
            perror("FAILED TO FOPEN in GET");
            exit(1);
        }
        read_from_server(sock_fd, method, lf_stream);


        
    } 



    
    if (method == PUT) {
        struct stat st;
        size_t fileSize = 0;  //indicates how many bytes are present in the binary data in the request.
        if (stat(remoteFile, &st) == 0) {
            fileSize = st.st_size;
        } else {
            perror("STAT ERROR IN PUT");
            free(parse_result);
            print_invalid_response();
            exit(1);
        }
        int header_len = snprintf(header, sizeof(header), "%s %s\n%zu", "PUT", remoteFile, fileSize);
        if (header_len > 0) {
            write(sock_fd, header, header_len);
            shutdown(sock_fd, SHUT_WR);
        } else {
            perror("write header error in GET");
            exit(1);
        }
        //get the binary data of the remoteFile and write it to the socket.
        FILE* file = fopen(remoteFile, "rb");  // Open the file in binary mode
        if (file == 0) {
            perror("FAILED TO OPEN FILE in PUT");
            fclose(file);
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
    }

    free(parse_result);

    return 1;
}

int read_from_server(int sock_fd, verb method, FILE* file_stream) {
    char buffer[BUFFER_SIZE];
    size_t total_recd = 0;
    int first_check_done = 0; // To ensure we only check the first three bytes once.
    while (1) {
        ssize_t bytes_recd = read(sock_fd, buffer, sizeof(buffer));
        if (bytes_recd == -1) {
            if (errno != EINTR) {
                return -1;
            }
            continue;
        } else if (bytes_recd == 0) {
            break;
        } else if (bytes_recd > 0) {
            //check the response status (OK or ERROR);
            int ok = 0;
            if (first_check_done == 0) {
                if (bytes_recd >= 3) { // Make sure we have at least three bytes to check
                    ok = (strncmp(buffer, "OK ", 3)) ? 1 : 0;
                }
                first_check_done = 1; // Mark that we have done the first check
            }
            
            if (method == GET || method == LIST) {
                if (ok == 1 && file_stream != NULL) {
                    total_recd = 1;
                }
            } else {
                break;
            }

        } else {
            return -1;
        }

        total_recd += bytes_recd;
        
    }
    return total_recd;
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
