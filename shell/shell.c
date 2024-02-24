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
#include <ctype.h>

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
process_info* process_info_create(char* cmd, pid_t pid);
void check_bg_finished();
void kill_signal_cmd(pid_t pid_kill);
void stop_signal_cmd(pid_t pid_stop);
void cont_signal_cmd(pid_t pid_cont);
static char* history_file = NULL;

extern char *optarg;

typedef struct process {
    char *command;
    pid_t pid;
} process;

void check_bg_finished() {
    size_t i = 0;
    for (; i < vector_size(process_vector); i++) {
        pid_t each_pid = ((process*)vector_get(process_vector, i))->pid;
        int status;
        int result = waitpid(each_pid, &status, WNOHANG);
        if (WIFEXITED(status) && result != 0) { 
            vector_erase(process_vector, i);
            i--;   //do I have to -- here? -answer yes
        }
    }
}

void kill_signal_cmd(pid_t pid_kill) {
    for (size_t i = 0; i < vector_size(process_vector); i++) {
        process* p = (process*) vector_get(process_vector, i);
        if (pid_kill == p->pid) {
            kill(pid_kill, SIGKILL);
            print_killed_process(pid_kill, p->command);
            return;
        }
    }
    print_no_process_found(pid_kill);
}

void stop_signal_cmd(pid_t pid_stop) {
    for (size_t i = 0; i < vector_size(process_vector); i++) {
        process* p = (process*) vector_get(process_vector, i);
        if (pid_stop == p->pid) {
            kill(pid_stop, SIGSTOP);
            print_stopped_process(pid_stop, p->command);
            return;
        }
    }
    print_no_process_found(pid_stop);
}

void cont_signal_cmd(pid_t pid_cont) {
    for (size_t i = 0; i < vector_size(process_vector); i++) {
        process* p = (process*) vector_get(process_vector, i);
        if (pid_cont == p->pid) {
            kill(pid_cont, SIGCONT);
            print_continued_process(pid_cont, p->command);
            return;
        }
    }
    print_no_process_found(pid_cont);
}


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
    history_vector = string_vector_create();
    bool opend_history = false;

    int opt = 0;
    while ((opt = getopt(argc, argv, "h:f:")) != -1) {
        char * file_path = get_full_path(optarg);
        if (opt == 'h') {
            load_history(file_path);  
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
        check_bg_finished(); //is there a race condition here? when I typed echo hi, then type ps, it still has echo hi (before I add check bg in ps) -answer yes
        FILE* user_input = stdin;
        if (getline(&buffer, &capacity, user_input) != -1) {
            if (buffer[strlen(buffer) - 1] == '\n') {
                buffer[strlen(buffer) - 1] = 0;
            }
            if (strcmp(buffer, "exit") == 0) {
                break;
            }

            if (!strncmp(buffer, "kill", 4)) {
                pid_t pid_kill;
                int kill_status = sscanf(buffer + 4, "%d", &pid_kill);
                if (kill_status == -1) {
                    print_invalid_command(buffer);
                } else {
                    kill_signal_cmd(pid_kill);
                }
                continue;
            } else if (!strncmp(buffer, "stop", 4)) {
                pid_t pid_stop;
                int stop_status = sscanf(buffer + 4, "%d", &pid_stop);
                if (stop_status == -1) {
                    print_invalid_command(buffer);
                } else {
                    stop_signal_cmd(pid_stop);
                }
                continue;
            } else if (!strncmp(buffer, "cont", 4)) {
                pid_t pid_cont;
                int cont_status = sscanf(buffer + 4, "%d", &pid_cont);
                if (cont_status == -1) {
                    print_invalid_command(buffer);
                } else {
                    cont_signal_cmd(pid_cont);
                }
                continue;
            }

            if (strstr(buffer, "&&") || strstr(buffer, "||") != NULL || strstr(buffer, "; ") != NULL) {
                logical_commands(buffer);
            } else if (built_in_commands(buffer) == -100) {
                
                external_commands(buffer);
            }
        } else {
            break;
        }
        //check if any background finishes 
        check_bg_finished();
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

    } else if (strcmp(vector_get(cmd_args, 0), "ps") == 0 && vector_size(cmd_args) == 1) {
        result = 100;
        print_process_info_header();
        //printf("size is %zu \n", vector_size(process_vector));
        check_bg_finished();
        add_process("./shell", getpid()); 
        for (size_t i = 0; i < vector_size(process_vector); i++) {
            process* proc = (process*)vector_get(process_vector, i);
            if (kill(proc->pid, 0) != -1) {
                process_info* proc_info = process_info_create(proc->command, proc->pid);
                print_process_info(proc_info);
                continue;
            }
            print_no_process_found((int)proc->pid);
        }
    }

    vector_destroy(cmd_args);

    return result;
}


process_info* process_info_create(char* cmd, pid_t pid) {
    process_info* p_info = malloc(sizeof(process_info));
    p_info->pid = pid;
    p_info->command = (char*)calloc(strlen(cmd) + 1, 0);
    if (p_info->command == NULL) {
        print_no_process_found((int)pid);
        free(p_info);
        return NULL;
    } 
    strcpy(p_info->command, cmd);

    // get nthreads && vsize && state
    char path[100];
    snprintf(path, 100, "/proc/%d/status", pid);
    FILE* fd = fopen(path, "r");
    if (fd == NULL) {
        print_script_file_error();
        return NULL;
    }
    char* line = NULL;
    size_t len = 0;
    //printf("HERE \n");
    while (getline(&line, &len, fd) != -1) {
        if (strncmp(line, "Threads:", 8) == 0) {
            char* t = line + 8;
            while (*t && isspace(*t)) {
                t++;
            }
            if (*t) {
                p_info->nthreads = strtol(t, NULL, 20);
            }
        } else if (strncmp(line, "VmSize:", 7) == 0) { //check!
            char* v = line + 7;
            while (*v && isspace(*v)) {
                v++;
            }
            if (*v) {
                printf("v is %d", *v);
                //p_info->vsize = strtoul(v, NULL, 20); //any conversion here?
                sscanf(v, "%lu", &p_info->vsize);
                //p_info->vsize = p_info->vsize / 1024;
            }
        } else if (strncmp(line, "State:", 6) == 0) {
            char* s = line + 6;
            while (*s && isspace(*s)) {
                s++;
            }
            if (*s) {
                p_info->state = *s;
            }
        }
    }
   
    fclose(fd);


    //get starttime, utime, stime (in HH:MM format) 
    unsigned long long starttime;
    unsigned long utime = 0, stime = 0;
    snprintf(path, 100, "/proc/%d/stat", pid);
    FILE* fd2 = fopen(path, "r");
    if (fd2 == NULL) {
        print_script_file_error();
        return NULL;
    }
    char time_buffer[1000];
    if (!fgets(time_buffer, 1000, fd2)) {
        fclose(fd2);
        return NULL;
    }

    // char *token;
    // int count = 0;
    // token = strtok(time_buffer, " ");
    // while (token != NULL) {
    //     count++;
    //     if (count == 22) { // starttime is the 22nd field
    //         starttime = strtoull(token, NULL, 10);
    //         break;
    //     }
    //     token = strtok(NULL, " ");
    // }

    // if (count < 22) {
    //     printf("Czo is %d \n", count);
    //     exit(1);
    // }

    //printf("11111111111111\n");
    vector* stat_file = strsplit(time_buffer, ' ');
    printf("SIZE IS %zu\n", vector_size(stat_file));
    starttime = strtoul(vector_get(stat_file, 21), NULL, 10); //are they correct by using vectors?
    utime = strtoul(vector_get(stat_file, 13), NULL, 10); 
    stime = strtoul(vector_get(stat_file, 14), NULL, 10); 
    vector_destroy(stat_file);
    fclose(fd2);

    //get btime
    //printf("222222222222\\n");
    unsigned long btime;
    FILE* fd3 = fopen("/proc/stat", "r");
    if (!fd3) {
        print_script_file_error();
        return NULL;
    }
    while(fgets(line, 100, fd3)) {
      if(!strncmp(line, "btime", 5)) {
        char *b = line + 6;
        while(isspace(*b)) ++b;
        btime = strtol(b, NULL, 10);
      }
    }
    fclose(fd3);

    free(line);
    line = NULL;
    //get process start time (in HH:MM format)
    time_t start_str_in_secs = btime + starttime / (unsigned long)sysconf(_SC_CLK_TCK);
    if (!localtime(&start_str_in_secs)) {
        exit(1);
    }
    char start_str_buffer[100];
    size_t bytes = time_struct_to_string(start_str_buffer, 100, localtime(&start_str_in_secs));
    if (bytes < 0) {
        exit(1);
    }
    p_info->start_str = malloc(bytes + 1);
    strcpy(p_info->start_str, start_str_buffer);

    //get process execution time (in M:SS format)
    unsigned long total_cpu_time_sec = (utime + stime) / (unsigned long) sysconf(_SC_CLK_TCK);
    char cpu_buffer[100];
    int cpu_bytes = execution_time_to_string(cpu_buffer, 100, (size_t)total_cpu_time_sec / 60, (size_t)total_cpu_time_sec % 60);
    printf("CPU BUFFER IS %s \n", cpu_buffer);
    if (cpu_buffer < 0) {
        exit(1);
    }
    p_info->time_str = malloc(cpu_bytes + 1);
    strcpy(p_info->time_str, cpu_buffer);

    

    return p_info;

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
    bool result = true;
    if (strcmp(cmd, "exit") == 0) {
        exit(1);
    }
    if(cmd[strlen(cmd) - 1] == '&') {
        //background process
        if (cmd[strlen(cmd) - 2] == ' ') {
            fflush(stdout);
            pid_t pid = fork();
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
                // vector_pop_back(args);    //why this does not work?
                // printf("after size is: %zu \n", vector_size(args));
                // printf("LAST ITEM IS is: %s \n", *((char**)vector_back(args)));
                char** vf = (char**)vector_front(args);
                execvp(*vf, vf);
                print_exec_failed(*vf);
                exit(1);
            } else {
                
                add_process(cmd, pid);
            }
        }
    } else {
        //normal external execution
        fflush(stdout);
        char* file_name;
        FILE* file;
        int fd;
        bool has_redi_output = false, has_redi_append = false, has_redi_input = false;
        if (strstr(cmd, ">>") != NULL) {
            has_redi_append = true;
            char *substr = strstr(cmd, ">>");
            file_name = substr + 2;
            if (isspace((unsigned char)*file_name)) file_name++;
            file = fopen(file_name, "a+");
            if (file == NULL) {
                return result;
            }
            fd = fileno(file);
        } else if (strstr(cmd, "<") != NULL) {
            has_redi_input = true;
            file_name = strchr(cmd, '<') + 1;
            if (isspace((unsigned char)*file_name)) file_name++;
            file = fopen(file_name, "r");
            if (file == NULL) {
                return result;
            }
            fd = fileno(file);
        } else if (strstr(cmd, ">") != NULL) {
            has_redi_output = true;
            file_name = strchr(cmd, '>') + 1;
            if (isspace((unsigned char)*file_name)) file_name++;
            file = fopen(file_name, "w");
            if (file == NULL) {
                return result;
            }
            fd = fileno(file);
        }
        vector* args = strsplit(cmd, ' ');
        pid_t pid = fork();
        
        if (pid == -1) {
            print_fork_failed();
            result = false;
        } else if (pid == 0) {
            int childpid = getpid();
            print_command_executed(childpid);
            //why vector_push_back(history_vector, cmd); does not work here? because it is in another process? yes
            if (has_redi_output || has_redi_append) {
                dup2(fd, STDOUT_FILENO);
                close(fd);     
                vector_set(args, vector_size(args) - 2, 0);
            }
            if (has_redi_input) {
                dup2(fd, STDIN_FILENO); //do not understand 
                close(fd);     
                vector_set(args, vector_size(args) - 2, 0); 
            }
            //printf("CMD IS %s \n", *((char**)vector_front(args)));
            char** vf = (char**)vector_front(args);
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
