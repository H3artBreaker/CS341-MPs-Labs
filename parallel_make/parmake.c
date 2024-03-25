/**
 * parallel_make
 * CS 341 - Spring 2024
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdbool.h>
#include "set.h"
#include <time.h>

bool cycle_detection(graph* rag, rule_t* vertex, set* visiting, set* visited);

bool run_cmds(rule_t* rule) {
    int success = 1;
    for (size_t j = 0; j < vector_size(rule->commands); j++) {
        char* cmd = (char*)vector_get(rule->commands, j);
        int sys_status = system(cmd);
        if (sys_status == -1) {
            success = 0;
            break;
        } else {
            if (WIFEXITED(sys_status)) {
                int exit_status = WEXITSTATUS(sys_status);
                if (exit_status != 0) {
                    success = 0;
                    break;
                }
            }
        }
    }
    if (success == 1) {
        return true;
    }
    return false;
}

/*
1 - satisfied
0 - failed
*/

int run_deps_cmds(graph* rag, rule_t* rule, vector* deps) {
    //printf("target is %s \n", rule->target);
    int result = 1;
    if (vector_size(deps) == 0) {
        //check if the rule's name is on the disk 
        struct stat buffer;
        //printf("rule's on disk? is %u \n", stat(rule->target, &buffer));
        if (stat(rule->target, &buffer) != 0) { //not on disk
            if (rule->state == 0) {
                bool cmd_status = run_cmds(rule);
                //mark the rule as satisfied  satisfied->1 failed->-1
                if (cmd_status == 1) {
                    rule->state = 1;
                } else {
                    rule->state = -1;
                    result = 0;
                }
            }  
        } else { //on the disk
            rule->state = 1; // not sure if it is necessary
            result = 2;
        }
        
        return result;
    }


    int dep_on_not_disk = 0;
    //vector* comp_time = shallow_vector_create();
    for (size_t i = 0; i < vector_size(deps); i++) {
        char* subRule_str = vector_get(deps, i);
        //printf("the subRule is %s\n", subRule_str);
        rule_t* subRule = graph_get_vertex_value(rag, subRule_str);
        vector* sub_deps = graph_neighbors(rag, subRule_str);
        int temp = run_deps_cmds(rag, subRule, sub_deps); // == 1 -> on disk and executes cmd successfully

        if (temp == 1) {
            dep_on_not_disk = 1;
        }

        result = (result == 1) ? temp : result; //if already failed before, we should not change the result 
        vector_destroy(sub_deps);
    }

    struct stat buffer2;

    if (result == 0) { //meaning a dependecy rule failed
        rule->state = -1;
        result = 0;
    } else if (stat(rule->target, &buffer2) != 0 || dep_on_not_disk == 1) { //check if the rule is on the disk 
        if (rule->state == 0) {
            bool cmd_status = run_cmds(rule);
            if (cmd_status == true) {
                rule->state = 1;
            } else {
                rule->state = -1;
                result = 0;
            }
        }
    } else if (vector_size(deps) != 0) {
        struct stat ori;
        if (stat(rule->target, &ori) != 0) {
            perror("Failed to get stats for file1");
            return -1; // Error occurred
        }
        for (size_t k = 0; k < vector_size(deps); k++) {
            struct stat cmp;
            char* elem = vector_get(deps, k);
            printf(" elem is %s \n", elem);
            if (stat(elem, &cmp) != 0) {
                //printf(" target is %s \n", rule->target);
                perror("Failed to get stats for file1");
                return -1; // Error occurred
            }
            double diff = difftime(cmp.st_mtime, ori.st_mtime);
            if (diff > 0) { //cmp is newer 
                //run cmd
                bool cmd_status = run_cmds(rule);
                if (cmd_status == 1) {
                    rule->state = 1;
                } else {
                    rule->state = -1;
                    result = 0;
                }
                
                break;
            }
        }
    }

    //vector_destroy(comp_time);
    
    
    return result;
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    graph* rag = parser_parse_makefile(makefile, targets);
    vector* goal_rules_vertex = graph_neighbors(rag, ""); //all vertexes are the target name in string

    for (size_t i = 0; i < vector_size(goal_rules_vertex); i++) {
        char* target = (char*)vector_get(goal_rules_vertex, i);
        rule_t * gr = (rule_t *)graph_get_vertex_value(rag, target);

        set* visited = shallow_set_create();
        set* visiting = shallow_set_create();
        
        if (cycle_detection(rag, gr, visiting, visited) == 0) {
            //determine if we need to run the rule's cmds
            vector* deps = graph_neighbors(rag, target);
            if (vector_size(deps) >= 1) {
                run_deps_cmds(rag, gr, deps);
                
            } else { 
                //check if the rule is the name of a file on the disk
                struct stat buffer;
                if (stat(gr->target, &buffer) != 0) {
                    bool cmd_status = run_cmds(gr);
                    //mark the rule as satisfied  satisfied -> 1; failed -> 0
                    if (cmd_status == true) {
                        gr->state = 1;
                    } else {
                        gr->state = 0;
                    }
                }
            }
            vector_destroy(deps);
        } else {
            print_cycle_failure(gr->target);
        }

        set_destroy(visited);
        set_destroy(visiting);
    }

    vector_destroy(goal_rules_vertex);
    graph_destroy(rag);
    return 0;
}

bool cycle_detection(graph* rag, rule_t* vertex, set* visiting, set* visited) {
    // Check if the vertex is null
    if (vertex == NULL) {
        return false;
    }

    if (set_contains(visiting, vertex->target)) {
        // Found a vertex that is already being visited in this DFS path, which indicates a cycle.
        return true;
    }
    if (set_contains(visited, vertex->target)) {
        // This vertex has been fully explored and does not lead to a cycle.
        return false;
    }

    set_add(visiting, vertex->target); // Mark this vertex as being visited in the current path.

    //printf("target in cycle is %s \n", vertex->target);

    vector* vertex_to = graph_neighbors(rag, vertex->target);
    if (vertex_to != NULL) { // Check if vertex_to is not NULL
        for (size_t i = 0; i < vector_size(vertex_to); i++) {
            char* next_target = vector_get(vertex_to, i);
            if (next_target == NULL) {
                continue; // Skip if next_target is NULL
            }
            rule_t* next_vertex = graph_get_vertex_value(rag, next_target);
            if (next_vertex == NULL) {
                continue; // Skip if next_vertex is NULL
            }
            if (cycle_detection(rag, next_vertex, visiting, visited)) {
                vector_destroy(vertex_to);
                return true;
            }
        }
    }

    // Move vertex from visiting to visited to indicate it's fully explored.
    vector_destroy(vertex_to); // Free the vector to prevent memory leaks
    set_remove(visiting, vertex->target);
    set_add(visited, vertex->target);

    return false;
}





// bool cycle_detection(graph* rag, rule_t* vertex, set* visiting, set* visited) {
//     if (set_contains(visiting, vertex)) {
//         // Found a vertex that is already being visited in this DFS path, which indicates a cycle.
//         return true;
//     }
//     if (set_contains(visited, vertex)) {
//         // This vertex has been fully explored and does not lead to a cycle.
//         return false;
//     }

//     set_add(visiting, vertex); // Mark this vertex as being visited in the current path.

//     printf("target in cycle is %s \n", vertex->target);

//     vector* vertex_to = graph_neighbors(rag, vertex->target);
//     for (size_t i = 0; i < vector_size(vertex_to); i++) {
//         printf("hello\n");
//         rule_t* next = vector_get(vertex_to, i);
//         if (cycle_detection(rag, next, visiting, visited)) {
//             return true;
//         }
//     }

//     // Move vertex from visiting to visited to indicate it's fully explored.
//     vector_destroy(vertex_to);
//     set_remove(visiting, vertex);
//     set_add(visited, vertex);

//     return false;
// }
