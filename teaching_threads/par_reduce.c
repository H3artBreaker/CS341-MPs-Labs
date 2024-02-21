/**
 * teaching_threads
 * CS 341 - Spring 2024
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
struct Task_In_Thread {
    reducer reduce_func;
    int base_case;
    int list_len;
    int* list;
};

typedef struct Task_In_Thread Task; 
/* You should create a start routine for your threads. */
void* start_routine(void* task) {
    Task* task_def = (Task*)task;
    int result = task_def->base_case;
    int* list = task_def->list;
    reducer reduce_func = task_def->reduce_func;
    for (int i = 0; i < task_def->list_len; ++i) {
            result = reduce_func(result, list[i]);
    }
    int * res = malloc(sizeof(int));
    *res = result;
    pthread_exit(res);
}


int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    int min_threads = (list_len > num_threads) ? num_threads : list_len;
    pthread_t tids[min_threads];
    int main_list[min_threads];
    Task* task_arr[min_threads];
    int num_of_listElem = list_len / min_threads;
    int remainder = (list_len % min_threads == 0) ? 0 : 1; 
    int start = 0;
    for (int i = 0; i < min_threads; i++) {
        Task* task = (Task*)malloc(sizeof(Task));
        task_arr[i] = task;
        if (task == NULL) {
            perror("TASK IS NULL!");
            return 0;
        }
        task->reduce_func = reduce_func;
        task->base_case = base_case;
        //create sublist
        if (i == (min_threads - 1) && remainder == 1) { // for last string if there is a remainder
            task->list_len = list_len - i * num_of_listElem;
            task->list = malloc(sizeof(int) * (task->list_len));
        } else {
            task->list_len = num_of_listElem;
            task->list = malloc(sizeof(int) * num_of_listElem);
        }
        for (int j = 0; j < task->list_len; j++) {
            (task->list)[j] = list[start + j];
        }
        start += task->list_len;

        //create thread;
        int result = pthread_create(&tids[i], NULL, start_routine, task);
        if (result != 0) {
            perror("pthread_create() error");
            exit(3);
        }
        //free(task->list); //why it fails here?
        // free(task);
    }

    for (int i = 0; i < min_threads; i++) {
        void* ret;
        pthread_join(tids[i], &ret); //stored the result into the main_List?
        main_list[i] = *((int*)ret);
        free(ret);
        free(task_arr[i]->list);
        free(task_arr[i]);
    }
    int result = reduce(main_list, min_threads, reduce_func, base_case);
    
    return result;
}

