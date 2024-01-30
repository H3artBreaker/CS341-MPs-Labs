/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    const char* input[16];
    input[0] = NULL;
    input[1] = "";
    input[2] = " ";
    input[3] = " h";
    input[4] = " hello. ";
    input[5] = " hello. World";
    input[6] = " hello. World!";
    input[7] = "陈";
    input[8] = " 9hello      world.     ";
    input[9] = "  n o     p u n c t   a n d    l o t o f s p a c e";
    input[10] = " DDsa. 9S 6st.";
    input[11] = "3 rings.";

    
    if (!test_helper(camelCaser, destroy, input[0], NULL)) {
        return 0;
    }
    
    if (!test_helper(camelCaser, destroy, input[1], (char*[]){NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[2], (char*[]){NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[3], (char*[]){NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[4], (char*[]){"hello", NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[5], (char*[]){"hello", NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[6], (char*[]){"hello", "world", NULL})) {
        return 0;
    }
    
    if (!test_helper(camelCaser, destroy, input[7], (char*[]){NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[8], (char*[]){"9helloWorld", NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[9], (char*[]){NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[10], (char*[]){"ddsa", "9s6St", NULL})) {
        return 0;
    }

    if (!test_helper(camelCaser, destroy, input[11], (char*[]){"3Rings", NULL})) {
        return 0;
    }


    return 1;
}

int test_helper(char **(*camelCaser)(const char *), void (*destroy)(char **), const char * input, char** target) {
    char **output = camelCaser(input);

    if (output == NULL) {
        if (target == NULL) {
            return 1;
        }
        return 0;
    }

    char** opt_ptr = output;
    char** tgt_ptr = target;
    while (*opt_ptr != NULL && *tgt_ptr != NULL) {
        if (strcmp(*opt_ptr, *tgt_ptr) != 0) {
            destroy(output);  
            return 0;
        }
        opt_ptr++;
        tgt_ptr++;
    }

    // Check if both arrays reached their end
    if (*opt_ptr != NULL || *tgt_ptr != NULL) {
        destroy(output);  
        return 0;
    }

    destroy(output);  
    return 1;
}

