/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    if (input_str == NULL) {
        return NULL;
    }

    //determine how many sentenses we have
    size_t sentence_num = 0;
    const char *tmp = input_str;
    while (*tmp != 0) {
        if (ispunct(*tmp)) {
            sentence_num++;
        }
        tmp++;
    }

    char** output = malloc((sentence_num + 1) * sizeof(char*));
    if (output == NULL) {
        return NULL;
    }
    output[sentence_num] = NULL; //NULL-terminated array 

    const char* ptr = input_str;
    size_t sentence_idx = 0;
    size_t length = 0;

    while (*input_str != 0) {
        if (!ispunct(*input_str) && !isspace(*input_str) && !isalpha(*input_str)) {
            length++;
            input_str++;
            continue;
        }

        if (isspace(*input_str)) {
            input_str++;
            continue;
        }

        if (ispunct(*input_str)) {
            char* sentence = malloc(length + 1);  //invalid after each iteration -> char sentense=[length + 1];
            if (sentence == NULL) {
                return NULL;
            }
            sentence[length] = '\0';
            int idx = 0;
            bool first_alpha = true;
            //bool had_alpha = false;
            bool first_word = true;
            while(ptr != input_str) {
                if (!ispunct(*ptr) && !isspace(*ptr) && !isalpha(*ptr)) {
                    sentence[idx] = *ptr;
                    idx++;
                    //continue;
                }
                
                if (isspace(*ptr)) {
                    if (first_alpha == false) {
                        first_word = false;
                    }
                    first_alpha = true;
                }

                if (isalpha(*ptr)) {
                    //printf("the char is %c, and the bool first_alpha is %d \n", *ptr, first_alpha);
                    
                    
                    if (first_word) {
                        sentence[idx] = tolower(*ptr);
                    } else if (!first_word && first_alpha) {
                        sentence[idx] = toupper(*ptr);
                    } else if (!first_word && !first_alpha) {
                        sentence[idx] = tolower(*ptr);
                    }
                    first_alpha = false;
                    idx++;

                }
                ptr++;
            }

            output[sentence_idx] = sentence;
            sentence_idx++;
            input_str++;
            continue;
        }

        input_str++;
        length++;
    }
    
    return output;
}

void destroy(char **result) {
    // TODO: Implement me!
    char** ptr = result;
    while (*ptr != 0) {
        free(*ptr);
        ptr++;
    }
    free(result);
    result = NULL;
    return;
}
