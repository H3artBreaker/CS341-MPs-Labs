/**
 * mapreduce
 * CS 341 - Spring 2024
 */
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char **argv) {
    if (argc != 6) {
        printf("INCORRECT ARGUMENTS");
        exit(1);
    }
    //char* input_file = argv[1];
    char* output_file = argv[2];
    //char* mapper = argv[3];
    //char* reducer = argv[4];
    char* mapper_count_str = argv[5];
    int mapper_count = atoi(mapper_count_str);
    // Create an input pipe for EACH mapper.
    int* mpr_fds[mapper_count]; 
    for (int i = 0; i < mapper_count; i++) {
        mpr_fds[i] = calloc(2, sizeof(int));
        if (pipe(mpr_fds[i]) < 0) {
            perror("Piping error for reducer");
            exit(1);
        }
    }
    
    // Create one input pipe for the reducer.
    int rdcr_pp[2];
    if (pipe(rdcr_pp) < 0) {
        perror("Piping error for reducer");
        exit(1);
    }

    // Open the output file.
    int fd_opf = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); //may not need O_TRUNC
    if (fd_opf == -1) {
        perror("Error opening the output file");
        exit(1);
    }

    // Start all the mapper processes.
    

    // Start a splitter process for each mapper.
    pid_t children_pids[mapper_count];

    for (int i = 0; i < mapper_count; i++) {
        children_pids[i] = fork();
        
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) { //child process 

    } else {

    }

    // Start the reducer process.

    // Wait for the reducer to finish.

    // Print nonzero subprocess exit codes.

    // Count the number of lines in the output file.





    for (int i = 0; i < mapper_count; i++) {
      free(mpr_fds[i]);
    }

    return 0;
}
