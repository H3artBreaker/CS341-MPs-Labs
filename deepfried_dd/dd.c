/**
 * deepfried_dd
 * CS 341 - Spring 2024
 */
#include "format.h"
#include <unistd.h>  
#include <stdio.h>  
#include <signal.h>
#include <time.h>
#include <stdlib.h>

static size_t full_blocks_in = 0;
static size_t partial_blocks_in = 0;
static size_t full_blocks_out = 0;
static size_t partial_blocks_out = 0;
static size_t total_bytes_copied = 0;
static double time_elapsed = 0;

static volatile sig_atomic_t print_status = 0; 

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        print_status = 1;
    }
}

int main(int argc, char **argv) {
    int opt;
    char* i_filename;
    char* o_filename;
    size_t block_size = 512;
    long cpy_blocks = -1; //defaults to the entire file
    long skip_input_blocks = 0;
    long skip_output_blocks = 0;
    //size_t cpy_size;

    while((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
        switch(opt) {
            case 'i':
                i_filename = optarg;
                break;
            case 'o':
                o_filename = optarg;
                break;
            case 'b':
                sscanf(optarg, "%zu", &block_size);
                break;
            case 'c':
                sscanf(optarg, "%ld", &cpy_blocks);
                break;
            case 'p':
                sscanf(optarg, "%ld", &skip_input_blocks);
                break;
            case 'k':
                sscanf(optarg, "%ld", &skip_output_blocks);
                break;
            case '?':
                exit(1);

        }
    }

    FILE* input_file = (i_filename) ? fopen(i_filename, "r") : stdin;
    FILE* output_file = (o_filename) ? fopen(o_filename, "w+") : stdout;
    
    if (input_file == NULL) {
        print_invalid_input(i_filename);
        exit(1);
    }

    if (output_file == NULL) {
        print_invalid_output(o_filename);
        exit(1);
    }
    //move to the starting position
    if (skip_input_blocks > 0 && input_file != stdin) { 
        if (fseek(input_file, skip_input_blocks * block_size, SEEK_SET) != 0) {
            perror("FSEEK PROBLEM IN INPUT FILE");
            exit(1);
        }
    }

    if (skip_output_blocks > 0 && output_file != stdout) {
        if (fseek(output_file, skip_output_blocks * block_size, SEEK_SET) != 0) {
            perror("FSEEK PROBLEM IN OUTPUT FILE");
            exit(1);
        }
    }
    

    //set the signal handler
    signal(SIGUSR1, signal_handler);
    struct timespec start_time;
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    char buffer[block_size];
    size_t written_in = 0;
    //printf("hello\n");
    while (!feof(input_file) && (cpy_blocks == -1 || written_in < cpy_blocks * block_size)) {
        //printf("hello\n");
        size_t byte_read = fread(buffer, 1, block_size, input_file); //read every blocks
        if (byte_read > 0) { //read successfully
            size_t byte_written = fwrite(buffer, 1, byte_read, output_file);
            if (byte_read == block_size) {
                full_blocks_in++;
                if (byte_written == byte_read) {
                    full_blocks_out++;
                } else {
                    partial_blocks_out++;
                }
            } else {
                partial_blocks_in++;
                if (byte_written == byte_read) {
                    partial_blocks_out++;
                } else {
                    full_blocks_out++;
                }
            }
            written_in += byte_written;
            total_bytes_copied += byte_written;
        }


        if (print_status == 1) {
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            time_elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
            print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied, time_elapsed);
            print_status = 0;
        }
    }



    fclose(input_file);
    fclose(output_file);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    time_elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied, time_elapsed);
    return 0;
}