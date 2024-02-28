/**
 * critical_concurrency
 * CS 341 - Spring 2024
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    queue* q = malloc(sizeof(queue));
    if (q == NULL) {
        return NULL;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->max_size = max_size;
    pthread_cond_init(&(q->cv), NULL);
    pthread_mutex_init(&(q->m), NULL);

    return NULL;
}

void queue_destroy(queue *this) {
    if (this == NULL) {
        return;
    }
    queue_node* curr = this->head;
    queue_node* after = curr->next;
    while (curr != this->tail) {
        free(curr);
        curr = after;
        after = after->next;
    }
    if (curr == this->tail && after == NULL) {
        free(curr);
    }
    pthread_cond_destroy(&(this->cv));
    pthread_mutex_destroy(&(this->m));
    free(this);
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    while (this->size == this->max_size) {
        pthread_cond_wait(&(this->cv), &(this->m));
    }
    queue_node* qn = malloc(sizeof(queue_node));
    qn->data = data;
    qn->next = NULL;

    if (this->head == NULL) {
        this->head = qn;
    } else {
        this->tail->next = qn;
    }

    this->tail = qn;

    (this->size)++;
    pthread_cond_broadcast(&(this->cv)); // wake the cv in pull
    pthread_mutex_unlock(&(this->m));
}

void *queue_pull(queue *this) {
    pthread_mutex_lock(&(this->m));
    while (this->size == 0) {
        pthread_cond_wait(&(this->cv), &(this->m));
    }
    queue_node* qn = this->head;

    this->head = qn->next;

    if (this->head == NULL) {
        this->tail = NULL;
    }

    (this->size)--;
    pthread_cond_broadcast(&(this->cv)); // wake the cv in push
    pthread_mutex_unlock(&(this->m));
    void* result = qn->data;
    free(qn);
    return result;
}
