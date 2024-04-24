/**
 * savvy_scheduler
 * CS 341 - Spring 2024
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */
    double arrive_time;
    double running_time;
    double priority;
    double remainting_time;
    double start_time;
    double end_time;
    double response_time;
    double turnaround_time;
    double wait_time;
} job_info;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
    job* ja = (job*)a;
    job* jb = (job*)b;

    job_info* jainfo = ((job_info*)ja->metadata);
    job_info* jbinfo = ((job_info*)jb->metadata);
    if (jainfo->arrive_time > jbinfo->arrive_time) { //b arrives first
        return 1;
    }

    if (jainfo->arrive_time < jbinfo->arrive_time) { //a arrives first
        return -1;
    }

    return 0;
}

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
    job* ja = (job*)a;
    job* jb = (job*)b;

    job_info* jainfo = ((job_info*)ja->metadata);
    job_info* jbinfo = ((job_info*)jb->metadata);
    if (jainfo->priority > jbinfo->priority) { //b priority
        return 1;
    }

    if (jainfo->priority < jbinfo->priority) { //a priority
        return -1;
    }

    return break_tie(a, b);
}

int comparer_psrtf(const void *a, const void *b) {
    
    return 0;
}

int comparer_rr(const void *a, const void *b) {
    // TODO: Implement me!
    return 0;
}

int comparer_sjf(const void *a, const void *b) {
    job* ja = (job*)a;
    job* jb = (job*)b;

    job_info* jainfo = ((job_info*)ja->metadata);
    job_info* jbinfo = ((job_info*)jb->metadata);
    if (jainfo->running_time > jbinfo->running_time) { //b short
        return 1;
    }

    if (jainfo->running_time < jbinfo->running_time) { //a short
        return -1;
    }

    return break_tie(a, b);
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO: Implement me!
    job_info* ji = calloc(1, sizeof(job_info));
    *ji = (job_info){job_number, time, sched_data->running_time, sched_data->priority};
    newjob->metadata = ji;
    priqueue_offer(&pqueue, newjob);
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    if (priqueue_size(&pqueue) == 0) {
        return NULL;
    }
    if (pqueue_scheme != PPRI && pqueue_scheme != PSRTF && job_evicted != NULL) {
        return job_evicted;
    }
    if ((pqueue_scheme == PPRI || pqueue_scheme == PSRTF) && job_evicted != NULL) {
        return priqueue_peek(&pqueue);
    } 
    job* r = priqueue_peek(&pqueue);
    
    return r;
}

void scheduler_job_finished(job *job_done, double time) {
    free((job_info*)job_done->metadata);
    (job_done->metadata) = NULL;
    priqueue_poll(&pqueue);
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return 9001;
}

double scheduler_average_turnaround_time() {
    // TODO: Implement me!
    return 9001;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return 9001;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
    // for (size_t i = 0; i < priqueue_size(&pqueue); i++) {
        
    // }
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
