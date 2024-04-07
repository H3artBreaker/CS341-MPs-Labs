/**
 * mapreduce
 * CS 341 - Spring 2024
 */
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 6) {
        printf("INCORRECT ARGUMENTS");
        exit(1);
    }
    char* input_file = argv[1];
    char* output_file = argv[2];
    char* mapper = argv[3];
    char* reducer = argv[4];
    char* mapper_count_str = argv[5];
    int mapper_count = atoi(mapper_count_str);
    // Create an input pipe for EACH mapper.
    int* mpr_fds[mapper_count]; 
    //int mpr_fds[mapper_count][2]; 
    for (int i = 0; i < mapper_count; i++) {
        mpr_fds[i] = calloc(2, sizeof(int));
        if (pipe(mpr_fds[i]) < 0) {
            perror("Piping error for reducer");
            exit(1);
        }
    }
    
    // Create one input pipe for the reducer.
    int rdcr_fd[2];
    if (pipe(rdcr_fd) < 0) {
        perror("Piping error for reducer");
        exit(1);
    }

    // Open the output file.
    int fd_opf = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); //may not need O_TRUNC
    if (fd_opf == -1) {
        perror("Error opening the output file");
        exit(1);
    }

    // Start a splitter process for each mapper.
    pid_t children_pid_s[mapper_count];

    for (int i = 0; i < mapper_count; i++) {
        children_pid_s[i] = fork();
        if (children_pid_s[i] < 0) {
            perror("Fork failed for splitter process");
            exit(1);
        } else if (children_pid_s[i] == 0) {
            close(rdcr_fd[0]); //splitter does not use reducer pipe read end
            close(rdcr_fd[1]); //splitter does not use reducer pipe write end
            close(mpr_fds[i][0]);  //splitter does not use mapper pipe read end

            dup2(mpr_fds[i][1], STDOUT_FILENO); //redirect the STDOUT of the splitter to the mapper pipe write end by copying its fd number
            close(mpr_fds[i][1]); // after the line 60, it means the STDOUT is gonna act as the write end. Therefore, we are having two write ends now, so we close the original one.

            char idx[12];
            sprintf(idx, "%d", i); //converting i to string

            execlp("./splitter", "./splitter", input_file, mapper_count_str, idx, NULL);
            perror("failed exec splitter");
            exit(1);
        }
    }

    // Start all the mapper processes. 
    pid_t children_pid_m[mapper_count];
    for (int i = 0; i < mapper_count; i++) {
        close(mpr_fds[i][1]);  //the whole program is not going to use mapper pipeline write end anymore
        children_pid_m[i] = fork();

        if (children_pid_m[i] < 0) {
            perror("Fork failed for mapper process");
            exit(1);
        } else if(children_pid_m[i] == 0) {
            close(rdcr_fd[0]); //mapper does not use reducer pipe read end
            
            dup2(mpr_fds[i][0], STDIN_FILENO); //redirect the mapper pipeline read end to the STDIN of mapper process.
            close(mpr_fds[i][0]); //after the above line, it means stdin is gonna act as read end of the mapper pipeline, so we need to close the original one since we now have two read ends.
            dup2(rdcr_fd[1], 1); //redirect the STDOUT of the mapper process to the write end of the reducer pipeline
            close(rdcr_fd[1]);

            execlp(mapper, mapper, NULL); //not execlp
            perror("failed exec mapper");
            exit(1);
        }
        close(mpr_fds[i][0]);  //the whole program is not going to use mapper pipeline write end anymore
    }

    

    // Start the reducer process.
    close(rdcr_fd[1]); // the whole program is not gonna use the write end of the pipeline for the reducer anymore
    pid_t rdcr_pid = fork();
    if (rdcr_pid < 0) {
        perror("Fork failed for reducer process");
        exit(1);
    } else if (rdcr_pid == 0) {
        dup2(rdcr_fd[0], STDIN_FILENO);
        close(rdcr_fd[0]);
        dup2(fd_opf, 1);
        close(fd_opf);
        execlp(reducer, reducer, NULL);
        perror("failed exec reducer");
        exit(1);
    } 
    close(fd_opf);
    close(rdcr_fd[0]);

                                                    //should I put above two functions before reducer? because reducer complete all at the same time or reducer is actually take one by one 

    // Wait for the reducer to finish.

    // Wait for the mappers to finish.
    for (int i = 0; i < mapper_count; i++) {     
        int status;
        waitpid(children_pid_s[i], &status, 0);
    }
    for (int i = 0; i < mapper_count; i++) {
        int status;
        waitpid(children_pid_m[i], &status, 0);
    }   

    int status;
    waitpid(rdcr_pid, &status, 0);
    // Print nonzero subprocess exit codes.
    if (status) {
        print_nonzero_exit_status(reducer, status);
    }
    
    // Count the number of lines in the output file.
    print_num_lines(output_file);


    for (int i = 0; i < mapper_count; i++) {
      free(mpr_fds[i]);
    }

    return 0;
}
