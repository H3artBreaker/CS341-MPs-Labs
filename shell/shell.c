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
#include <sys/types.h>
#include <sys/wait.h>

static vector* history_vector;
static vector* process_vector;
int built_in_commands(char* cmd);
bool logical_commands(char* cmd);
bool external_commands(char* cmd);
vector* strsplit(char* cmd, char delim);
vector* read_file(char* file);
void and_operation(char *cmd);
void or_operation(char* cmd);
void seperator_operation(char* cmd);
void add_process(char* cmd, pid_t pid);
static char* history_file = NULL;

extern char *optarg;

typedef struct process {
    char *command;
    pid_t pid;
} process;


void load_history(char* file) {
    FILE* fp = fopen(file, "a+"); //changed to "a+" because if file does not exist, it should create a new one

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
        return NULL;
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
    //add_process(argv[0], getpid()); 
    history_vector = string_vector_create();
    bool opend_history = false;

    int opt = 0;
    while ((opt = getopt(argc, argv, "h:f:")) != -1) {
        char * file_path = get_full_path(optarg);
        if (opt == 'h') {
            //char * file = get_full_path(optarg);
            load_history(file_path);  
            // puts("in h");
            //history_file = file_path;
            history_file = get_full_path(optarg);
            opend_history = true;
        } else if (opt == 'f') {
            vector* cmds = read_file(file_path);
            if (cmds != NULL) { //file exists
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
                    } else if (built_in_commands(cmd) == -100) {
                        int exit = external_commands(cmd);
                        if (exit == false) {
                            break;
                        }
                    }
                } 
                vector_destroy(cmds);
            }
            free(file_path);
            vector_destroy(history_vector);
            vector_destroy(process_vector);
            return 0;
        } else if (opt == '?') {
            free(file_path);
            print_usage();
            exit(1);
        }
        free(file_path);
    }
    
    

    char* buffer = NULL;
    size_t capacity = 0;

    char path_buff[PATH_MAX + 1];
    while (1) {
        if (getcwd(path_buff, sizeof(path_buff))) {
            print_prompt(path_buff, getpid());
        }
        FILE* user_input = stdin;
        if (getline(&buffer, &capacity, user_input) != -1) {
            if (buffer[strlen(buffer) - 1] == '\n') {
                buffer[strlen(buffer) - 1] = 0;
            }
            if (strcmp(buffer, "exit") == 0) {
                break;
            }

            if (strstr(buffer, "&&") || strstr(buffer, "||") != NULL || strstr(buffer, "; ") != NULL) {
                logical_commands(buffer);
            } else if (built_in_commands(buffer) == -100) {
                external_commands(buffer);
            }
        } else {
            break;
        }

    }
    if (opend_history) {
        //char* file = get_full_path(history_file);
        FILE* fd = fopen(history_file, "w");
        for (size_t i = 0; i < vector_size(history_vector); i++) {
            fputs(vector_get(history_vector, i), fd);
            fputs("\n", fd);
        }
        fclose(fd);
        free(history_file);
    }

    free(buffer);
    vector_destroy(history_vector);
    vector_destroy(process_vector);
    return 0;
}

/*

100 - correct built_in_commands && valid execution
0 - correct built_in_commands && invalid execution 
-100 - incorrect built_in_commands

*/


int built_in_commands(char* cmd) {
    //char** cmd_args = strsplit(cmd, ' ');
    int result = -100;
    vector* cmd_args = strsplit(cmd, ' '); //split each arguments into vector elements
    if (strcmp(vector_get(cmd_args, 0), "cd") == 0) {
        result = 100;
        if (vector_size(cmd_args) != 2) {
            if (vector_size(cmd_args) == 1) {
                result = 0;
                print_no_directory(cmd);
            } else {
                print_invalid_command(cmd);
            }
        } else {
            if (chdir(vector_get(cmd_args, 1)) != 0) { 
                result = 0;
                print_no_directory(vector_get(cmd_args, 1));
            } 
        }
        vector_push_back(history_vector, cmd);
    } else if (strcmp(vector_get(cmd_args, 0), "!history") == 0 && vector_size(cmd_args) == 1) {
        result = 100;
        for (size_t i = 0; i < vector_size(history_vector); i++) {
            print_history_line(i, vector_get(history_vector, i));
        }
    } else if (*cmd == '#') {
        result = 100;
        if (strlen(cmd) == 1 || vector_size(cmd_args) > 1) { //check if there is anything after # or have more than one argument
            result = 0;
            print_invalid_command(cmd);
            vector_destroy(cmd_args);
            return result;
        }
        //find the number
        size_t num = atoi(cmd + 1);

        if (num >= vector_size(history_vector)){
            result = 0;
            print_invalid_index();
            vector_destroy(cmd_args);
            return result;
        }

        char* target_cmd = vector_get(history_vector, num);  
        print_command(target_cmd); //will be added to history when it is executed

        if (strstr(target_cmd, "&&") || strstr(target_cmd, "||") || strstr(target_cmd, "; ")) {
            logical_commands(target_cmd);
        } else if (built_in_commands(target_cmd) == -100) {
            external_commands(target_cmd);
        }


    } else if (*cmd == '!') {
        result = 100;
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
            result = 0;
            print_no_history_match();
            vector_destroy(cmd_args);
            return result;
        }

        print_command(prefix_exec_cmd);

        if (strstr(prefix_exec_cmd, "&&") || strstr(prefix_exec_cmd, "||") || strstr(prefix_exec_cmd, "; ")) {
            logical_commands(prefix_exec_cmd);
        } else if (built_in_commands(prefix_exec_cmd) == -100) {
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

    return cmd_args_vec;
}

bool logical_commands(char* cmd) {
    if (strstr(cmd, "&&")) {
        and_operation(cmd);
    } else if (strstr(cmd, "||")) {
        or_operation(cmd);
    } else if (strstr(cmd, "; ")) {
        seperator_operation(cmd);
    }
    return true;
}

void and_operation(char* cmd) {
    
    sstring* sstr = cstr_to_sstring(cmd);
    vector* result = NULL;
    if (sstring_substitute(sstr, 0, " && ", "@") == 0) {
        result = sstring_split(sstr, '@');
    }

    //x && y
    vector_push_back(history_vector, cmd);
    int from_built_in = built_in_commands(vector_get(result, 0));
    if (from_built_in == 100) { //x is true
        vector_pop_back(history_vector);
        int from_built_in_sec = built_in_commands(vector_get(result, 1));
        if (from_built_in_sec == -100) {
            external_commands(vector_get(result, 1));
        } 
        vector_pop_back(history_vector);
    } else if (from_built_in == -100) { //x is TBD but not built-in
        bool next = external_commands(vector_get(result, 0)); 
        if (next) { 
            vector_pop_back(history_vector);
            if (built_in_commands(vector_get(result, 1)) == -100) { //x is true run y
                //puts("HERE");
                external_commands(vector_get(result, 1));
            } 
            vector_pop_back(history_vector);
        }
    } else {
        vector_pop_back(history_vector);
    }

    vector_destroy(result);
    sstring_destroy(sstr);
}

void or_operation(char* cmd) {
    sstring* sstr = cstr_to_sstring(cmd);
    vector* result = NULL;
    if (sstring_substitute(sstr, 0, " || ", "@") == 0) {
        result = sstring_split(sstr, '@');
    }
    //x || y
    vector_push_back(history_vector, cmd);
    int from_built_in = built_in_commands(vector_get(result, 0));
    if (from_built_in == 100) { //x is built-in and true;
        vector_pop_back(history_vector);
    } else if (from_built_in == 0) {// x is built-in and false
        vector_pop_back(history_vector);
        int from_built_in_sec = built_in_commands(vector_get(result, 1));
        if (from_built_in_sec == -100) {
            external_commands(vector_get(result, 1));
        }
        vector_pop_back(history_vector);
    } else if (from_built_in == -100) { //x is external
        bool next = external_commands(vector_get(result, 0));
        if (!next) {
            
            if (built_in_commands(vector_get(result, 1)) == -100) { //x is true run y
                //puts("HERE");
                external_commands(vector_get(result, 1));
            } 
            vector_pop_back(history_vector);
        }
        vector_pop_back(history_vector);
    }
    
    vector_destroy(result);
    sstring_destroy(sstr);
}

void seperator_operation(char* cmd) {
    sstring* sstr = cstr_to_sstring(cmd);
    vector* result = NULL;
    if (sstring_substitute(sstr, 0, "; ", "@") == 0) {
        result = sstring_split(sstr, '@');
    }
    vector_push_back(history_vector, cmd);

    int is_builtin_x = built_in_commands(vector_get(result, 0));
    if (is_builtin_x == -100) {
        external_commands(vector_get(result, 0));
    }
    vector_pop_back(history_vector);

    int is_builtin_y = built_in_commands(vector_get(result, 1));
    if (is_builtin_y == -100) {
        external_commands(vector_get(result, 1));
    }
    vector_pop_back(history_vector);
    
    vector_destroy(result);
    sstring_destroy(sstr);
}

bool external_commands(char* cmd) {
    //bool is_background = false;
    
    bool result = true;
    if (strcmp(cmd, "exit") == 0) {
        exit(1);
    }
    if(cmd[strlen(cmd) - 1] == '&') {
        //background process
        if (cmd[strlen(cmd) - 2] == ' ') {
            fflush(stdout);
            pid_t pid = fork();
            //puts("IM IN!!!");
            // SIGCHILD
            cmd[strlen(cmd) - 2] = 0;
            vector* args = strsplit(cmd, ' ');
            if (pid == -1) {
                print_fork_failed();
                result = false;
            } else if (pid == 0) {
                int childpid = getpid();
                if (setpgid(childpid, childpid) == -1) {
                    print_setpgid_failed();
                    result = false;
                    vector_destroy(args);
                    vector_destroy(process_vector);
                    vector_destroy(history_vector);
                    exit(1);
                }
                print_command_executed(childpid);
                // printf("BEFORE size is: %zu \n", vector_size(args));
                // printf("LAST ITEM IS is: %s \n", *((char**)vector_back(args)));
                // vector_pop_back(args);
                // printf("after size is: %zu \n", vector_size(args));
                // printf("LAST ITEM IS is: %s \n", *((char**)vector_back(args)));
                char** vf = (char**)vector_front(args);
                execvp(*vf, vf);
                print_exec_failed(*vf);
                puts("HIIII!!!");
                exit(1);
            } else {
                
                add_process(cmd, pid);
            }
        }
    } else {
        //normal external execution
        vector* args = strsplit(cmd, ' ');
        fflush(stdout);
        pid_t pid = fork();
        
        if (pid == -1) {
            print_fork_failed();
            result = false;
        } else if (pid == 0) {
            int childpid = getpid();
            print_command_executed(childpid);
            //print_command(childpid);
            //why vector_push_back(history_vector, cmd); does not work here? because it is in another process?
            char** vf = (char**)vector_front(args);

            //print_command(*vf);
            execvp(*vf, vf);
            print_exec_failed(*vf);
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0);
            vector_push_back(history_vector, cmd);
            if (!WIFEXITED(status) || status != 0) {
                print_wait_failed();
                result = false;
                vector_destroy(args);
                return result;
            }
        }
        vector_destroy(args);
    }
    
    
    return result;
}


void add_process(char* cmd, pid_t pid) {
    process* p = malloc(sizeof(process));
    if (p == NULL) {
        return;
    }
    p->command = malloc(sizeof(cmd) + 1); //why we need to copy
    strcpy(p->command, cmd);
    p->pid = pid;
    vector_push_back(process_vector, p);
}
