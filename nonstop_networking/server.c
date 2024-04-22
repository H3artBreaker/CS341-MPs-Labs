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

void remove_directory_contents(const char *path);

static int epoll_fd;
static int server_socket;
static vector* file_list;
static dictionary* fd_to_connState;
static char* dir_name;

static int MAX_EVENTS = 50;

typedef struct {
    int state;  // Represents the current state in the DFA
    verb method;  // HTTP request verb, e.g., "GET"
    char *request_buffer;  // Buffer for storing request data
    int buffer_offset;  // Offset for reading/writing in the buffer
    char *filename;  // Filename for file-related operations
} connection_state_t;

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

void shutdown_server() {
    //clean the directory
    remove_directory_contents(dir_name);
    if (rmdir(dir_name) != 0) {
        perror("Failed to remove directory");
    } 



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

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

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

    if (listen(server_socket, 100)) {
        perror("Listen error");
        close(server_socket);
        exit(1);
    }

    fd_to_connState = shallow_to_shallow_dictionary_create();
    file_list = shallow_vector_create();

    //create the temp directory
    char* template = "XXXXXX";
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
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;  // Edge-triggered read events
    event.data.fd = server_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        perror("Epoll ctl failed");
        close(epoll_fd);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1) {
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

                // Register client with epoll (edge-triggered)
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    perror("Epoll ctl for client failed");
                    shutdown_server();
                }

                //set the fd's connection state to the dictionary
                connection_state_t* conn_stat = calloc(1, sizeof(connection_state_t));
                dictionary_set(fd_to_connState, &client_fd, conn_stat);
            } else {
                
            }
        }

    }




    //clean everything
    shutdown_server();


    return 0;
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
