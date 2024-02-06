/**
 * vector
 * CS 341 - Spring 2024
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>
// #include <stdlib.h> // For malloc, free

struct sstring {
    // Anything you want
    char* string;
};

sstring *cstr_to_sstring(const char *input) {
    sstring* sstr_ptr = (sstring*) malloc(sizeof(sstring));
    sstr_ptr->string = malloc(strlen(input) + 1);
    strcpy(sstr_ptr->string, input);
    return sstr_ptr;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    char* str = malloc(sizeof(input->string) + 1);
    strcpy(str, input->string);
    return str;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    int length = strlen(this->string) + strlen(addition->string);         
    this->string = realloc(this->string, length + 1);
    strcpy(this->string + strlen(this->string), addition->string);         //it will not copy the \0 at the end of the string?
    return length;
}


vector *sstring_split(sstring *this, char delimiter) {
    vector* vec = vector_create(string_copy_constructor, string_destructor, string_default_constructor);

    size_t start = 0;
    size_t len = strlen(this->string);
    for (size_t i = 0; i <= len; ++i) {
        if (this->string[i] == delimiter || i == len) {
            size_t part_len = i - start;
            char* part = malloc(part_len + 1);
            if (part) {
                memcpy(part, this->string + start, part_len);
                part[part_len] = '\0'; // Null-terminate the substring
                vector_push_back(vec, part);
                free(part); 
            }
            start = i + 1;
        }
    }

    return vec; 
}


int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here

    char* start = strstr(this->string + offset, target);
    if (start == NULL) {
        return -1;
    }
    char* newstr = malloc(strlen(this->string) - strlen(target) + strlen(substitution) + 1);
    size_t prefixLength = start - this->string;

    memcpy(newstr, this->string, prefixLength);
    memcpy(newstr + prefixLength, substitution, strlen(substitution));
    memcpy(newstr + prefixLength + strlen(substitution), start + strlen(target), strlen(this->string) - prefixLength - strlen(target) + 1);

    free(this->string);
    this->string = newstr;
    
    return 0;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char* slice = malloc(end - start + 1);
    strncpy(slice, this->string + start, end - start);
    slice[end - start] = '\0';
    return NULL;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    free(this->string);
    this->string = NULL;
    free(this);
    this = NULL;
}
