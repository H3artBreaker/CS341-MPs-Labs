/**
 * critical_concurrency
 * CS 341 - Spring 2024
 */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

#define NUM_THREADS 5
#define NUM_ELEMENTS 10

void *producer(void *arg) {
    queue *q = (queue *)arg;
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        int *data = malloc(sizeof(int));
        *data = i;
        queue_push(q, data);
        printf("Produced: %d\n", *data);
    }
    pthread_exit(NULL);
}

void *consumer(void *arg) {
    queue *q = (queue *)arg;
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        int *data = (int *)queue_pull(q);
        printf("Consumed: %d\n", *data);
        free(data);
    }
    pthread_exit(NULL);
}

int main() {
    // queue *q = queue_create(-1); // Unlimited queue size

    // pthread_t producer_thread, consumer_thread;
    // pthread_create(&producer_thread, NULL, producer, q);
    // pthread_create(&consumer_thread, NULL, consumer, q);

    // pthread_join(producer_thread, NULL);
    // pthread_join(consumer_thread, NULL);
    queue *q = queue_create(-1);

for (int i = 0; i < NUM_ELEMENTS; i++) {
    int *data = malloc(sizeof(int));
    *data = i;
    queue_push(q, data);
    printf("Produced: %d\n", *data);
}

for (int i = 0; i < NUM_ELEMENTS; i++) {
    int *data = (int *)queue_pull(q);
    printf("Consumed: %d\n", *data);
    free(data);
}

    queue_destroy(q);

    return 0;
}

