/**
 * nonstop_networking
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include "vector.h"
#include "dictionary.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "format.h"
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include "common.h"
#include <errno.h>
#include <fcntl.h>

typedef struct {
    int state;  // Represents the current state in the DFA
    verb method;  // HTTP request verb, e.g., "GET"
    char filename[255];  // Filename for file-related operations
} connection_state_t;

void remove_directory_contents(const char *path);
void client_process(struct epoll_event client_event);
size_t get_header(int client_fd, char* header);
int header_parser(int client_fd, connection_state_t* client_info);
int PUT_file(int client_fd, connection_state_t* client_info);

static int epoll_fd;
static int server_socket;
static vector* file_list;
static dictionary* fd_to_connState;
static char* dir_name = NULL;


static int MAX_EVENTS = 50;



char* server_parser(int argc, char **argv) {
    if (argc != 2) {
        print_server_usage();
        exit(1);
    }

    //check if it is an integer
    char* endptr;
    strtol(argv[1], &endptr, 10); // Parse as base-10 integer
    if (*endptr != '\0') { // If endptr points to the end, it's valid
        print_server_usage();
        perror("NO PORT NUMBER IN ARGV");
        exit(1);
    } 

    return argv[1];
}

void response_generator(const char* response, int client_fd) {
    printf("Start writing response\n");
    size_t wsize = write_all_to_socket(client_fd, response, strlen(response));
    if (wsize != strlen(response)) {
        perror("generating response error");
    }
    printf("finished generating response\n");
}

void shutdown_server() { //unfinished
    //clean the directory
    remove_directory_contents(dir_name);
    if (rmdir(dir_name) != 0) {
        perror("Failed to remove directory");
    } 

    dictionary_destroy(fd_to_connState);
    vector_destroy(file_list);

    close(epoll_fd);
    close(server_socket);

    exit(1);
}

int main(int argc, char **argv) {
    // good luck!

    //setting up the SIGINT as early as possible
    struct sigaction sa;
    memset (&sa, 0, sizeof (struct sigaction));
    sa.sa_handler = shutdown_server;
    sa.sa_flags = 0;   // No special flags
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Error setting up signal handler for SIGPIPE");
        shutdown_server();
    }

    char* port = server_parser(argc, argv);


    struct addrinfo hints;
    struct addrinfo *result;
    memset (&hints, 0, sizeof (struct addrinfo)); // used to clear 

    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE; // << We want Server socket

    int s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        freeaddrinfo(result);
        result = NULL;
        exit (1);
    }

    server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    int enable = 1;

    // Set SO_REUSEADDR
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_socket);
        freeaddrinfo(result);
        result = NULL;
        exit(1);
    }

    // Set SO_REUSEPORT
    //#ifdef SO_REUSEPORT
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEPORT) failed");
        close(server_socket);
        freeaddrinfo(result);
        result = NULL;
        exit(1);
    }
    //#endif

    if (bind(server_socket, result->ai_addr, result->ai_addrlen) != 0 ) {
        perror("Bind error");
        close(server_socket);
        freeaddrinfo(result);
        result = NULL;
        exit(1);
    }

    freeaddrinfo(result);
    result = NULL;

    if (listen(server_socket, 50)) {
        perror("Listen error");
        close(server_socket);
        exit(1);
    }

    fd_to_connState = int_to_shallow_dictionary_create();
    file_list = string_vector_create();
    
    //create the temp directory
    char template[] = "XXXXXX";
    dir_name = mkdtemp(template);
    if(dir_name == NULL){
        perror("mkdtemp failed: ");
        close(server_socket);
        exit(1);
    }
    print_temp_directory(dir_name);
    
    //setup epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Epoll creation failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Register server socket with epoll (edge-triggered)
    struct epoll_event event;
    event.events = EPOLLIN;  // Do not set Edge-triggered 
    event.data.fd = server_socket;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) { //By registering this listening socket with epoll_ctl, you indicate that you want to be notified when there's activity on this socket (like new incoming connections).
        close(epoll_fd);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    
    while (1) {
        printf("Start epoll_waiting\n");
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        printf("Finish epoll_waiting\n");
        if (event_count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            perror("Epoll wait failed");
            shutdown_server();
        }

        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == server_socket) { 
                int client_fd = accept(server_socket, NULL, NULL);
                if (client_fd == -1) {
                    perror("Accept failed");
                    shutdown_server();
                }

                // //fcntl
                // int flags = fcntl(client_fd, F_GETFL, 0);  // Get current flags
                // if (flags == -1) {
                //     perror("Error getting file descriptor flags");
                //     shutdown_server();
                // }

                // // Set non-blocking mode
                // if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                //     perror("Error setting non-blocking mode");
                //     shutdown_server();
                // }

                // Register client with epoll (edge-triggered)
                event.events = EPOLLIN;
                event.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    perror("Epoll ctl for client failed");
                    shutdown_server();
                }

                //set the fd's connection state to the dictionary
                connection_state_t* conn_stat = calloc(1, sizeof(connection_state_t));
                dictionary_set(fd_to_connState, &client_fd, conn_stat);
            } else {
                printf("(fd: )client fd is %d\n", events[i].data.fd);
                client_process(events[i]);

                int client_fd = events[i].data.fd;
                free(dictionary_get(fd_to_connState, &client_fd));
                shutdown(client_fd, SHUT_RDWR);
                close(client_fd);
                dictionary_remove(fd_to_connState, &client_fd);  
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL); //!!!!!!
            }
        }

    }

    //clean everything
    shutdown_server();

    return 0;
}

void client_process(struct epoll_event client_event) {
    int client_fd = client_event.data.fd;
    if (dictionary_contains(fd_to_connState, &client_fd) == false) {
        return;
    }
    connection_state_t* client_info = dictionary_get(fd_to_connState, &client_fd);
    
    //see if the header is parsed, if no parse it
    if (client_info->state == 0) { //first time parse it
        int hp_result = header_parser(client_fd, client_info);
        if (hp_result != 1) {
            // client_info->state = -100;
            perror("PROBLEM IN PARSE HEAD");
            response_generator("ERROR\n", client_fd);
            response_generator(err_bad_request, client_fd);
            return;
        } 
    }

    if (client_info->method == GET) {
        char* retrieve_file = client_info->filename;
        struct stat st;
        size_t fileSize = 0;  //indicates how many bytes are present in the binary data in the request.

        char path[BUFFER_SIZE];
        memset(path, 0, sizeof(path));
        snprintf(path, sizeof(path), "%s/%s", dir_name, retrieve_file);
        if (stat(path, &st) == 0) {
            fileSize = st.st_size;
        } else {
            perror("FILE NOT EXIST ON SERVER");
            response_generator("ERROR\n", client_fd);
            response_generator(err_no_such_file, client_fd);
            return;
        }
        
        if (client_info->state == 0) {
            //get the binary data of the localFile and write it to the socket.
            FILE* file = fopen(path, "rb");  // Open the retrieve file in binary mode
            if (file == 0) {
                perror("FAILED TO OPEN FILE ON SERVER");
                fclose(file);
                response_generator("ERROR\n", client_fd);
                response_generator(err_no_such_file, client_fd);
                return;
            }

            response_generator("OK\n", client_fd);
            
            char sizeBuffer[sizeof(size_t)]; //VERB [filename]\n([File size])
            memcpy(sizeBuffer, &fileSize, sizeof(fileSize));
            if (write_all_to_socket(client_fd, sizeBuffer, sizeof(size_t)) == -1) {
                perror("write error for size");
                fclose(file);
                return;
            }
            

            char read_buffer[BUFFER_SIZE];
            while (!feof(file)) {   
                size_t byte_read = fread(read_buffer, 1, BUFFER_SIZE, file); //read every blocks（buffer)
                if (byte_read > 0) { //read successfully
                    size_t byte_written = write_all_to_socket(client_fd, read_buffer, byte_read);
                    if (byte_written == (size_t)-1) {
                        perror("FAILED TO WRITE TO SERVER SOCKET in PUT");
                        response_generator("ERROR\n", client_fd);
                        fclose(file);
                        return;
                    } 
                } 
            }

            fclose(file);
        }
        client_info->state = 1;
    } else if (client_info->method == PUT) {
        int pf_result = PUT_file(client_fd, client_info);
        if (pf_result == -100 || pf_result == -500) { //too little or too much
            response_generator("ERROR\n", client_fd);
            response_generator(err_bad_file_size, client_fd);
            return;
        }
        if (pf_result == -200) {
            perror("BAD REQUEST");
            response_generator("ERROR\n", client_fd);
            response_generator(err_bad_request, client_fd);
            return;
        }
        if (pf_result == -1) {
            perror("CAN't CREAT FILE ");
            response_generator("ERROR\n", client_fd);
            response_generator(err_no_such_file, client_fd);
            return;
        }
        if (pf_result == -1000) {
            perror("PUT HAS SIGPIPE PROBABLY");
            return;
        }
        client_info->state = 1;
    } else if (client_info->method == DELETE) {
        char* delete_file = client_info->filename;
        
        char path[BUFFER_SIZE];
        snprintf(path, sizeof(path), "%s/%s", dir_name, delete_file);

		if (remove(path) != 0) {
            perror("file not exist so can not be removed or remove has error");
			response_generator("ERROR\n", client_fd);
            response_generator(err_no_such_file, client_fd);
            return;
		}

        if (client_info->state == 0) {
            int removed = 0;
            for (size_t i = 0; i < vector_size(file_list); i++) {
                if (strncmp((char*)vector_get(file_list, i), delete_file, strlen(delete_file)) == 0) {
                    vector_erase(file_list, i);
                    removed = 1;
                    printf("REMOVED THE file\n");
                    break;
                }
            }
            if (removed == 0) {
                response_generator("ERROR\n", client_fd);
                response_generator(err_no_such_file, client_fd);
                return;
            }
        }
        client_info->state = 1;
    } else if (client_info->method == LIST) {
        response_generator("OK\n", client_fd);
        size_t lsize = 0;
        for (size_t i = 0; i < vector_size(file_list); i++) {
            char* filename = (char*)vector_get(file_list, i);
            lsize += strlen(filename);
            if (i != vector_size(file_list) - 1) {
                lsize++;
            }
        }
        char sizeBuffer[sizeof(size_t)];
        memcpy(sizeBuffer, &lsize, sizeof(sizeBuffer));
        write_all_to_socket(client_fd, sizeBuffer, sizeof(sizeBuffer));

        for (size_t i = 0; i < vector_size(file_list); i++) {
            char* filename = (char*)vector_get(file_list, i);
        
            size_t filename_length = strlen(filename);
            size_t buffer_size = (i != vector_size(file_list) - 1) ? filename_length + 1 : filename_length;  // Adjust size based on newline
            
            char buffer[buffer_size];
            memcpy(buffer, filename, filename_length);  // Copy without null-termination

            // Add newline if not the last item
            if (i != vector_size(file_list) - 1) {
                buffer[filename_length] = '\n';  // Append newline at the end
            }

            // Write to the socket the exact length
            write_all_to_socket(client_fd, buffer, buffer_size);

        }
        client_info->state = 1;
    } else {
        response_generator("ERROR\n", client_fd);
        response_generator(err_bad_request, client_fd);
        return;
    }

    //printf("state is %u \n", client_info->state);
    if (client_info->method == PUT || client_info->method == DELETE) {
        if (client_info->state == 1) {
            response_generator("OK\n", client_fd);
        }
    }
}

/*
-1 NO FILE EXIST
-100 too little
-200 bad request
-500 too much
1 success for all

*/


int PUT_file(int client_fd, connection_state_t* client_info) {
    char* filename = client_info->filename;
    char wholePath[BUFFER_SIZE];
    snprintf(wholePath, sizeof(wholePath), "%s/%s", dir_name, filename);

    int fd = open(wholePath, O_WRONLY | O_CREAT | O_TRUNC, 00777);
    if (fd == -1) {
        perror("Failed to open file");
        remove(wholePath);
        return -1;
    }

    // Get the file size 
    size_t file_size = 0;
    char size_buf[BUFFER_SIZE];
    size_t bytes_read = read_all_from_socket(client_fd, size_buf, sizeof(size_t));
    //read_all_from_socket(client_fd, size_buf, sizeof(size_t));
    if (bytes_read != sizeof(size_t)) {
        perror("Error reading file size");
        close(fd);
        remove(wholePath);
        return -200;
    }
    memcpy(&file_size, size_buf, sizeof(size_t));
    
    // Write the binary data to the file on the server
    char buffer[BUFFER_SIZE];
    size_t total_read = 0;
    while (total_read < file_size) {
        size_t byte_read = read_all_from_socket(client_fd, buffer, BUFFER_SIZE);
        if (byte_read == 0 && total_read < file_size) {
            close(fd);
            remove(wholePath);
            return -100; // Not enough data
        }
        if (byte_read == (size_t)-1) {
            close(fd);
            remove(wholePath);
            return -200;
        }
        if (byte_read > 0) { // Successful read
            size_t byte_written = write_all_to_socket(fd, buffer, byte_read);
            if (byte_written == -1) {
                perror("PROBABLY SIGPIPE");
                close(fd);
                remove(wholePath);
                return -1000;
            }
            if (byte_written != byte_read) {
                perror("Failed to write data to file");
                close(fd);
                remove(wholePath);
                return -200;
            }
        }
        total_read += byte_read;
    }

    if (total_read > file_size || read_all_from_socket(client_fd, buffer, BUFFER_SIZE) > 0) {
        close(fd);
        remove(wholePath);
        return -500; // Excessive data
    }

    //int r = fclose(file_stream); // Close the file stream
    close(fd);

    // Add to the file list
    vector_push_back(file_list, filename);

    return 1; // Success
}

/*
1 - successs
< 0 - malformed (header problem or no method)

*/


int header_parser(int client_fd, connection_state_t* client_info) {
    char header[HEADER_BUFFER_SIZE]; //HEADER_BUFFER_SIZE == 1024
    memset(header, 0, sizeof(header));
    size_t header_len = get_header(client_fd, header);
    if (header_len <= 0) {
        return (int)header_len;
    }
    printf("the header is %s \n", header);
    //check verb
    if (strncmp(header, "GET ", 4) == 0) { //add the whitespace in to compare
        client_info->method = GET;
        strcpy(client_info->filename, header + 4);
        client_info->filename[strlen(client_info->filename) - 1] = '\0'; //replace the \n by \0
    } else if (strncmp(header, "PUT ", 4) == 0) { //add the whitespace in to compare
        client_info->method = PUT;
        strcpy(client_info->filename, header + 4);
        client_info->filename[strlen(client_info->filename) - 1] = '\0'; //replace the \n by \0
        printf("HEADER is %s \n", client_info->filename);
    } else if (strncmp(header, "DELETE ", 7) == 0) { //add the whitespace in to compare
        client_info->method = DELETE;
        strcpy(client_info->filename, header + 7);
        client_info->filename[strlen(client_info->filename) - 1] = '\0'; //replace the \n by \0
    } else if (strncmp(header, "LIST\n", 5) == 0) {
        client_info->method = LIST;
    } else {
        perror("malformed!!");
        return -1;
    }
    return 1;
}


size_t get_header(int client_fd, char* header) { //VERB + (filename)
    char part[1];
    memset(part, 0, sizeof(part));

    size_t total_capacity = 0;

    while (1) {
        size_t byte_read = read_all_from_socket(client_fd, part, 1);
        if (byte_read <= 0) break;  // Handle read error or disconnect

        memcpy(header + total_capacity, part, 1); //strcpy does not work here???
        total_capacity += byte_read;

        if (total_capacity >= HEADER_BUFFER_SIZE) {
            return -500; //too much buffer overflow;
        }

        if (part[0] == '\n') {
            break;
        }

    }

    return total_capacity;
}

void remove_directory_contents(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *dp;

    while ((dp = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path to the file or symbolic link
        char newpath[strlen(path) + strlen(dp->d_name) + 2];
        snprintf(newpath, sizeof(newpath), "%s/%s", path, dp->d_name);

        struct stat s;
        if (lstat(newpath, &s) == 0) {
            if (S_ISREG(s.st_mode) || S_ISLNK(s.st_mode)) {
                // If it's a regular file or symbolic link, delete it
                if (unlink(newpath) != 0) {
                    perror("Failed to delete file or symbolic link");
                }
            }
        } else {
            perror("Error in lstat");
        }
    }

    closedir(dir);
}


