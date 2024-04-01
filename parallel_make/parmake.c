/**
 * parallel_make
 * CS 341 - Spring 2024
 */

#include "format.h"
#include "graph.h"
#include "queue.h"
#include "parmake.h"
#include "parser.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdbool.h>
#include "set.h"
#include <time.h>
#include <pthread.h>

graph* rag;
queue* q;
set* topo_set;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
size_t thread_num = 1;


bool cycle_detection(graph* rag, rule_t* vertex, set* visiting, set* visited);

int run_cmds(rule_t* rule) {
    int success = 1;
    for (size_t j = 0; j < vector_size(rule->commands); j++) {
        char* cmd = (char*)vector_get(rule->commands, j);
        int sys_status = system(cmd);
        if (sys_status == -1) {
            success = -1;
            break;
        } else {
            if (WIFEXITED(sys_status)) {
                int exit_status = WEXITSTATUS(sys_status);
                if (exit_status != 0) {
                    //printf("FAILED\n");
                    success = -1;
                    break;
                }
            }
        }
    }
    return success;
}



void q_push_topoSort(char* target) {
    if (set_contains(topo_set, target)) {
        return;
    }
    set_add(topo_set, target);
    vector* deps = graph_neighbors(rag, target);

    //using dfs to push the leaf dependency rule into the queue in topological sort order
    for (size_t i = 0; i < vector_size(deps); i++) {
        char* subTarget_str = (char*)vector_get(deps, i);
        q_push_topoSort(subTarget_str);
    }

    if (vector_size(deps) == 0) {
        queue_push(q, target);
    }

    rule_t *rule = (rule_t *)graph_get_vertex_value(rag, target);
    rule->state = vector_size(deps);

    vector_destroy(deps);
}

void* run_deps_cmds(void* arg) {
    while (1) {
        pthread_t tid = pthread_self();
        
        char* target = (char*)queue_pull(q);
        printf("The target is %s in thread id %lu\n", target, (unsigned long)tid);
        if (target == NULL) {
            break;
        }
        
        //printf("The target is %s in thread id %lu\n", target, (unsigned long)tid);

        pthread_mutex_lock(&m);
        rule_t *rule = (rule_t *) graph_get_vertex_value(rag, target);
        pthread_mutex_unlock(&m);

        //check if it is on the disk 
        struct stat argStat;
        int stat_runCmd = 0;

        pthread_mutex_lock(&m);
        vector* deps = graph_neighbors(rag, target);
        pthread_mutex_unlock(&m);

        if (stat(rule->target, &argStat) != 0) { //not on disk //0 means on disk, 1 means not on the disk
            stat_runCmd = 1;
        } else {
            for (size_t i = 0; i < vector_size(deps); i++) {//if it has dependency
                char* subTarget_str = (char*)vector_get(deps, i);
                pthread_mutex_lock(&m);
                rule_t* subRule = (rule_t*)graph_get_vertex_value(rag, subTarget_str);
                pthread_mutex_unlock(&m);
                struct stat subStat;
                //check if itself is on the disk or if its dependency has newer modification (st_mtime is the last access time in number of seconds and nanoseconds since the epoch. st_mtime)
                if (stat(subRule->target, &subStat) != 0 || (difftime(subStat.st_mtime, argStat.st_mtime) > 0)) {
                    stat_runCmd = 1;
                    break;
                }
            }
        }
        //printf("THE target %s STAT RUN CMD IS %u \n", rule->target, stat_runCmd );
        if (stat_runCmd == 1) {
            if (rule->state == 0) {
                rule->state = run_cmds(rule); // 1 -> satisfied; -1 -> failed
            }
        }

        //check if we need to add its parent in (the node that depends on the current target)
        //printf("THE target %s state IS %u \n", rule->target, rule->state);
        if (rule->state != -1) {
            pthread_mutex_lock(&m);
            vector *parents = graph_antineighbors(rag, target);
            pthread_mutex_unlock(&m);

            VECTOR_FOR_EACH(parents, upRule, {
                pthread_mutex_lock(&m);
                rule_t * par = graph_get_vertex_value(rag, upRule);
                pthread_mutex_unlock(&m);
                par->state -= 1;
                if (par->state == 0) {
                    queue_push(q, upRule);
                }

            }); 

            vector_destroy(parents);
        }
        vector_destroy(deps);
        
    }

    return NULL;
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    rag = parser_parse_makefile(makefile, targets);
    q = queue_create(-1);
    vector* gr_vertexes = graph_neighbors(rag, ""); //all vertexes are the target name in string
    thread_num = num_threads;

    if (vector_size(gr_vertexes) == 0) {
        queue_destroy(q);
        graph_destroy(rag);
        vector_destroy(gr_vertexes);
        return 0;
    }
    topo_set = shallow_set_create();

    for (size_t i = 0; i < vector_size(gr_vertexes); i++) {
        char* target = (char*)vector_get(gr_vertexes, i);
        rule_t * gr = (rule_t *)graph_get_vertex_value(rag, target);

        set* visited = shallow_set_create();
        set* visiting = shallow_set_create();
        
        if (cycle_detection(rag, gr, visiting, visited) == 0) {
            q_push_topoSort(target);
        } else {
            print_cycle_failure(gr->target);
            gr->state = -1;
        }
        set_destroy(visited);
        set_destroy(visiting);
    }

    

    //initialize the pthreads
    pthread_t tids[num_threads];

    for (size_t i = 0; i < num_threads; i++) {
        pthread_create(tids + i, NULL, run_deps_cmds, NULL);
    }

    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(tids[i], NULL);
    }

    set_destroy(topo_set);
    queue_destroy(q);
    vector_destroy(gr_vertexes);
    graph_destroy(rag);
    pthread_mutex_destroy(&m);
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



//fixed 

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
//         char* next = vector_get(vertex_to, i);
//         rule_t* next_vertex = graph_get_vertex_value(rag, next);
//         if (cycle_detection(rag, next_vertex, visiting, visited)) {
//             return true;
//         }
//     }

//     // Move vertex from visiting to visited to indicate it's fully explored.
//     vector_destroy(vertex_to);
//     set_remove(visiting, vertex);
//     set_add(visited, vertex);

//     return false;
// }
