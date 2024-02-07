/**
 * utilities_unleashed
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <time.h>
#include "format.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


//wall_time/elapsed time is the total time it takes for a process or operation to complete. 
//The difference between it and CPU is CPU time is the time actually spent by CPU executing method code (only the time during which the processor is actively working on a certain task or process).
//If WT > CT, it means our programs was waiting for systems resources to become available

int main(int argc, char *argv[]) {
    struct timespec start, end;
    int begin = clock_gettime(CLOCK_MONOTONIC, &start);       //how does CLOCK_MONOTONIC work in clock_gettime
    if (begin == -1) {exit(1);}
    
    if (argc < 2) {
        print_time_usage();
    }

    pid_t pid = fork();

    if (pid < 0) {
        print_fork_failed();
        exit(1); 
    } else if (pid > 0) {
        int status; 
        waitpid(pid, &status, 0);  // check the signiture of waitpid
        if (!WIFEXITED(status) || status != 0) {
            exit(1);
        }
    } else {
        execvp(argv[1], argv + 1);
        print_exec_failed();
    }


    clock_gettime(CLOCK_MONOTONIC, &end);

    double wall_time = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
    display_results(argv, wall_time);

    return 0;
}
