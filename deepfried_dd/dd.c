/**
 * deepfried_dd
 * CS 341 - Spring 2024
 */
#include "format.h"
#include <unistd.h>  
#include <stdio.h>  
#include <stdlib.h>

static size_t full_blocks_in = 0;
static size_t partial_blocks_in = 0;
static size_t full_blocks_out = 0;
static size_t partial_blocks_out = 0;
static size_t total_bytes_copied = 0;
static double time_elapsed = 0;

int main(int argc, char **argv) {
    int opt;
    char* i_filename;
    char* o_filename;
    size_t block_size = 512;
    long cpied_blocks = -1; //defaults to the entire file
    long skip_input_blocks = 0;
    long skip_output_blocks = 0;

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
                sscanf(optarg, "%ld", &cpied_blocks);
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
    




    fclose(input_file);
    fclose(output_file);


    print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied, time_elapsed);
    return 0;
}