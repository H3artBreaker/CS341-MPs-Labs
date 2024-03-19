/**
 * password_cracker
 * CS 341 - Spring 2024
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "./includes/queue.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <crypt.h>

static queue* q;
static int q_size = 0;
static int recovered = 0;
static int fail = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void* crack(void* t_id) {
    long tid = (long)t_id; 
    struct crypt_data cdata;
    cdata.initialized = 0;

    while (1) {
        pthread_mutex_lock(&m);
        if (q_size == 0) { // Check queue size inside the lock to avoid race conditions
            pthread_mutex_unlock(&m);
            break;
        }

        char* elem = queue_pull(q); 
        q_size--;
        pthread_mutex_unlock(&m);

        char username[16], hash[16], known[16];
        sscanf(elem, "%s %s %s", username, hash, known); // Extract task details

        v1_print_thread_start(tid, username);
        double cpu_start = getThreadCPUTime();

        char* guess_point = known + getPrefixLength(known);
        setStringPosition(guess_point, 0); // Initialize guess_point for guessing
        int hash_count = 0, found = 0;

        do {
            char* hash_try = crypt_r(known, "xx", &cdata);
            hash_count++;
            if (strcmp(hash, hash_try) == 0) {
                found = 1;
                break;
            }
        } while (incrementString(guess_point));

        pthread_mutex_lock(&m);
        if (found) recovered++;
        else fail++;
        pthread_mutex_unlock(&m);

        v1_print_thread_result(tid, username, known, hash_count, getThreadCPUTime() - cpu_start, !found);
        free(elem);
    }

    return NULL;
}



int start(size_t thread_q_size) {
    // TODO your code here, make sure to use thread_q_size!
    // Remember to ONLY crack passwords in other threads

    //create queue 
    q = queue_create(0); 
    char* buffer = NULL;
    size_t len = 0;
    while (getline(&buffer, &len, stdin) != -1) {
        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) -1] = '\0';
        }
        queue_push(q, strdup(buffer));
        q_size++;
    }
    free(buffer);
    buffer = NULL;

    //create threads
    pthread_t tid[thread_q_size];

    for (size_t i = 0; i < thread_q_size; i++) {
        pthread_create(&tid[i], NULL, crack, (void*)(i + 1));
    }

    // join threads
    for(size_t i = 0; i < thread_q_size; i++){
        pthread_join(tid[i], NULL);
    }

    v1_print_summary(recovered, fail);
    pthread_mutex_destroy(&m);
    queue_destroy(q);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}



