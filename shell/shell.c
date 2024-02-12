/**
 * shell
 * CS 341 - Spring 2024
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <stdio.h>  
#include <unistd.h>  
#include <stdbool.h>
#include <string.h>
#include <signal.h>

static vector* history_vector;
static vector* process_vector;
bool built_in_commands(char* cmd);
bool logical_commands(char* cmd);
bool external_commands(char* cmd);
vector* strsplit(char* cmd, char delim);
vector* read_file(char* file);
//static vector* cmds = NULL;

extern char *optarg;

typedef struct process {
    char *command;
    pid_t pid;
} process;


void load_history(char* file) {
    FILE* fp = fopen(file, "r");

    if (fp == NULL) {
        print_history_file_error();
        return;
    }
    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fp) != -1) {
        if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';
        vector_push_back(history_vector, line);
    }

    free(line);
    fclose(fp);
}

vector* read_file(char* file) {
    FILE* fp = fopen(file, "r");

    if (fp == NULL) {
        print_script_file_error();
    }
    vector* commands = string_vector_create();
    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fp) != -1) {
        if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';
        vector_push_back(commands, line);
    }

    free(line);
    line = NULL;
    fclose(fp);
    return commands;
}

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    if (!(argc == 1 || argc == 3 || argc == 5)) {
        print_usage();
        exit(1);
    }
    signal(SIGINT, SIG_IGN);
    process_vector = shallow_vector_create();

    history_vector = string_vector_create();
    //bool loaded_his = false;

    int opt;
    while ((opt = getopt(argc, argv, "h:f:")) != -1) {
        char * file = get_full_path(optarg);
        if (opt == 'h') {
            //char * file = get_full_path(optarg);
            load_history(file);
            //loaded_his = true;
            //break;
        } else if (opt == 'f') {
            vector* cmds = read_file(file);
            for (size_t i = 0; i < vector_size(cmds); i++) {
                char* cmd = vector_get(cmds, i);
                char buffer[80];
                if (getcwd(buffer, sizeof(buffer))) {
                    print_prompt(getcwd(buffer, sizeof(buffer)), getpid());
                    print_command(cmd);
                }
                //check if it has logical operator
                if (strstr(cmd, "&&") || strstr(cmd, "||") != NULL || strstr(cmd, "; ") != NULL) {
                    logical_commands(cmd);
                } else if (built_in_commands(cmd) == false) {
                    external_commands(cmd);
                }
                free(cmd);
            } 
            vector_destroy(cmds);
            //break;
        } else if (opt == '?') {
            free(file);
            print_usage();
            exit(1);
        }
        free(file);
    }

    vector_destroy(history_vector);
    vector_destroy(process_vector);
    return 0;
}


bool built_in_commands(char* cmd) {
    //char** cmd_args = strsplit(cmd, ' ');
    bool result = false;
    vector* cmd_args = strsplit(cmd, ' '); //split each arguments into vector elements
    if (strcmp(vector_get(cmd_args, 0), "cd") == 0) {
        result = true;
        if (vector_size(cmd_args) == 1) {
            print_no_directory(cmd);
        } else {
            if (chdir(vector_get(cmd_args, 1)) != 0) { 
                print_no_directory(vector_get(cmd_args, 1));
            } 
        }
        vector_push_back(history_vector, cmd);
    } else if (strcmp(vector_get(cmd_args, 0), "!history") == 0) {
        result = true;
        for (size_t i = 0; i < vector_size(history_vector); i++) {
            print_history_line(i, vector_get(history_vector, i));
        }
    } else if (*cmd == '#') {
        result = true;
        if (strlen(cmd) == 1) { //check if there is anything after #
            print_invalid_command(cmd);
            vector_destroy(cmd_args);
            return result;
        }
        //find the number
        size_t num = atoi(cmd + 1);

        if (num >= vector_size(history_vector)){
            print_invalid_index();
            vector_destroy(cmd_args);
            return result;
        }

        char* target_cmd = vector_get(history_vector, num);  
        print_command(target_cmd); //will be added to history when it is executed

        if (strstr(target_cmd, "&&") || strstr(target_cmd, "||") || strstr(target_cmd, "; ")) {
            logical_commands(target_cmd);
        } else if (built_in_commands(target_cmd) == false) {
            external_commands(target_cmd);
        }


    } else if (*cmd == '!') {
        result = true;
        char* prefix_exec_cmd = NULL;
        char* prefix = cmd + 1;
        if (strlen(cmd) == 1) { //check if there is anything after !
            prefix_exec_cmd = vector_get(history_vector, (vector_size(history_vector) - 1));
        } else {
            //vector* parts = NULL;
            for (int i = vector_size(history_vector) - 1; i >= 0; i--) {
                if (strncmp(vector_get(history_vector, i), prefix, strlen(prefix)) == 0) {
                    //find it 
                    prefix_exec_cmd = vector_get(history_vector, i);
                    break;
                }
            }
        }
        //run from the end
        
        if (prefix_exec_cmd == NULL) {
            print_no_history_match();
            vector_destroy(cmd_args);
            return result;
        }

        print_command(prefix_exec_cmd);

        if (strstr(prefix_exec_cmd, "&&") || strstr(prefix_exec_cmd, "||") || strstr(prefix_exec_cmd, "; ")) {
            logical_commands(prefix_exec_cmd);
        } else if (built_in_commands(prefix_exec_cmd) == false) {
            external_commands(prefix_exec_cmd);
        }

    } 

    vector_destroy(cmd_args);


    return result;
}

vector* strsplit(char* cmd, char delim) {
    sstring* sstring = cstr_to_sstring(cmd); 
    vector* cmd_args_vec = sstring_split(sstring, delim); 

    // char** cmd_args = malloc((vector_size(cmd_args_vec) + 1) * sizeof(char*));
    // for (size_t i = 0; i < vector_size(cmd_args_vec); i++) {
    //     cmd_args[i] = strdup(vector_get(cmd_args_vec, i));
    // }
    // cmd_args[vector_size(cmd_args_vec)] = 0;

    sstring_destroy(sstring);
    // vector_destroy(cmd_args_vec);

    //return cmd_args;
    return cmd_args_vec;
}

bool logical_commands(char* cmd) {
    return false;
}

bool external_commands(char* cmd) {

    return false;
}
