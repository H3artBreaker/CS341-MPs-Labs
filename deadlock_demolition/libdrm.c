/**
 * deadlock_demolition
 * CS 341 - Spring 2024
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>
#include <stdio.h>

struct drm_t {
    pthread_mutex_t mutex;
};

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
graph* drm_graph;



drm_t *drm_init() {
    drm_t* drm = malloc(sizeof(drm_t));
    pthread_mutex_init(&(drm->mutex), NULL);
    pthread_mutex_lock(&m);
    if (drm_graph == NULL) {
        drm_graph = shallow_graph_create();
    }
    if (!graph_contains_vertex(drm_graph, drm)) {
        graph_add_vertex(drm_graph, drm);
    }
    pthread_mutex_unlock(&m);
    return drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    pthread_mutex_lock(&m);
    if (drm != NULL && thread_id != NULL) {
        if (!graph_contains_vertex(drm_graph, drm) || !graph_contains_vertex(drm_graph, thread_id)) {
            return 0;
        }
        // see if an edge from the drm to the thread exists;
        vector* to_thread = graph_antineighbors(drm_graph, thread_id);
        for (size_t i = 0; i < vector_size(to_thread); i++) { //may improve
            if ((drm_t*)vector_get(to_thread, i) == drm) {
                graph_remove_edge(drm_graph, drm, thread_id);
                pthread_mutex_unlock(&(drm->mutex));
                pthread_mutex_unlock(&m);
                return 1;
            }
        }
    }
    pthread_mutex_unlock(&m);
    return 0;
}

bool is_cycle(graph* g, drm_t* start, set* visited) {
    if (set_contains(visited, start)) {
        return true;
    }
    set_add(visited, start);
    vector* vertex_to = graph_neighbors(g, start);
    for (size_t i = 0; i < vector_size(vertex_to); i++) {
        void* next = vector_get(vertex_to, i);
        if (is_cycle(g, next, visited)) {
            return true;
        }
    }

    //set_remove(visited, start);

    return false;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    //add thread to the graph
    pthread_mutex_lock(&m);
    if (!graph_contains_vertex(drm_graph, thread_id)) {
        graph_add_vertex(drm_graph, thread_id);
        //graph_set_vertex_value(drm_graph, thread_id, "thread");
    }
    
    //check deadlock
    //1st case a thread trying to lock a mutex it already owns.
    vector* to_thread = graph_antineighbors(drm_graph, thread_id);
    for (size_t i = 0; i < vector_size(to_thread); i++) {
        if ((drm_t*)vector_get(to_thread, i) == drm) {
            pthread_mutex_unlock(&m);
            return 0;
        }
    }

    //Add the appropriate edge to the Resource Allocation Graph. (requesting)
    graph_add_edge(drm_graph, thread_id, drm);

    //2nd case if the newly added edge creates a cycle in the Resource Allocation Graph.
    set* visited = shallow_set_create();
    if (is_cycle(drm_graph, drm, visited)) {
        graph_remove_edge(drm_graph, thread_id, drm);
        pthread_mutex_unlock(&m);
        return 0;
    }

    //no cycle  //check if the resource is used by others 
    pthread_mutex_unlock(&m); //the reason we do unlock hers is the next line does not change the shared resource. 
                                //Therefore, it is better to keep the critical section as tight as possible rather than leaving a whole thing

    pthread_mutex_lock(&(drm->mutex)); // if the mutex is locked, this thread will be waiting. when it is not locked, we will change the edge then.
    pthread_mutex_lock(&m); // we lock again is because we are gonna change the resource, the graph. As we know, generally one shared resource corresponds to one mutex

    graph_remove_edge(drm_graph, thread_id, drm);
    graph_add_edge(drm_graph, drm, thread_id);

    pthread_mutex_unlock(&m);
    return 1;
}


void drm_destroy(drm_t *drm) {
    /* Your code here */
    graph_remove_vertex(drm_graph, drm);
    pthread_mutex_destroy(&(drm->mutex));
    pthread_mutex_destroy(&m);
    free(drm);
    return;
}
