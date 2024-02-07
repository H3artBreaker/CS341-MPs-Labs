/**
 * utilities_unleashed
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include "format.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>

bool check_input_validity(int argc, char * argv[]) {
    if (argc < 3) { //we can have no ENV
        return false;
    }
    for (size_t i = 1; i < (size_t)argc; i++) {
        if (strcmp(argv[i], (const char*)"--") == 0) {
            return (i + 1 < (size_t) argc); //check if there is any cmd after --
        } 
        if (strchr(argv[i], '=') == NULL) {
            return false;
        }
    }
    return false;
}

void split_string(const char* string) { // 0 1 2 3 = 5 6 7
    char* equal_pos = strchr(string, '=');
    size_t key_len = equal_pos - string;
    size_t value_len = strlen(equal_pos) - 1;

    //extract key
    char* key = malloc(sizeof(char) * key_len + 1);
    memcpy(key, string, key_len);
    key[key_len] = '\0';

    //extract value
    char* value;
    if (strchr(equal_pos + 1, '%') == NULL) {
        value = strdup(equal_pos + 1);
        //setenv
        int env_set = setenv(key, value, 1);
        if (env_set != 0) {
            print_environment_change_failed();
            exit(1);
        }
    } else {
        value_len--;
        value = strdup(equal_pos + 2);
        // printf("value is: %s and the value lenth is %ld \n", value, value_len);
        // printf("The same ? %d \n", strcmp(value, "TEMP"));
        char* prev_value = getenv(value);
        //printf("prev_value is %s ", prev_value);
        if (prev_value == NULL) {
            print_environment_change_failed();
            exit(1);
        }
        int env_set = setenv(key, prev_value, 1);
        if (env_set != 0) {
            print_environment_change_failed();
            exit(1);
        }

    }


    free(key);
    key = NULL;
    free(value);
    value = NULL;

}


int main(int argc, char *argv[]) {
    if (!check_input_validity(argc, argv)) {
        print_env_usage();
        exit(1);
    }
    pid_t pid = fork();

    if (pid < 0) {
        print_fork_failed();
    } else if (pid == 0) {
        int i = 1;
        for (; i < argc - 2; i++) {
            if (strcmp(argv[i], (const char*)"--") == 0) break;
            split_string(argv[i]);
        }
        
        execvp(argv[i + 1], argv + i + 1); // does the -- counted in the arguments? - No starting from the cmd after it
        print_exec_failed();
    } else {
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}

