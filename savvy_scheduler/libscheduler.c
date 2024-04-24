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

static double total_waiting = 0;
static double total_response = 0;
static double total_turnaround = 0;
static double size = 0;
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
    double remaining_time;
    double start_time;
    double end_time;
    double response_time;
    double turnaround_time;
    double wait_time;
    double restart_time;
    int rr;
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
    job* ja = (job*)a;
    job* jb = (job*)b;

    job_info* jainfo = ((job_info*)ja->metadata);
    job_info* jbinfo = ((job_info*)jb->metadata);
    if (jainfo->remaining_time > jbinfo->remaining_time) { //b priority
        return 1;
    }

    if (jainfo->remaining_time < jbinfo->remaining_time) { //a priority
        return -1;
    }

    return break_tie(a, b);
}

int comparer_rr(const void *a, const void *b) {
    job* ja = (job*)a;
    job* jb = (job*)b;

    job_info* jainfo = ((job_info*)ja->metadata);
    job_info* jbinfo = ((job_info*)jb->metadata);
    if (jainfo->rr > jbinfo->rr) { //b priority
        return 1;
    }

    if (jainfo->rr < jbinfo->rr) { //a priority
        return -1;
    }

    return break_tie(a, b);
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
    ji->id = job_number;
    ji->arrive_time = time;
    ji->running_time = sched_data->running_time;
    ji->priority = sched_data->priority;
    ji->remaining_time = sched_data->running_time;
    ji->restart_time = 0;
    
    newjob->metadata = ji;
    priqueue_offer(&pqueue, newjob);
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    if (priqueue_size(&pqueue) == 0) {
        return NULL;
    }
    job* peek = priqueue_peek(&pqueue);
    job_info* info = (job_info*)peek->metadata;

    if (job_evicted == NULL) {
        if (info->restart_time == 0) {
            info->start_time = time;
        }
        info->rr += 1;
        return peek;
    }
    

    //if job_evicted != NULL;
    job_info* curr_info = job_evicted->metadata;
    curr_info->remaining_time = curr_info->remaining_time - (time - curr_info->restart_time);

    curr_info->restart_time = time;

    if (pqueue_scheme == RR) {
        job* out = priqueue_peek(&pqueue);
        priqueue_poll(&pqueue);
        priqueue_offer(&pqueue, out);
        
        if (info->restart_time == 0) {
            info->start_time = time;
        }
        info->rr += 1;
        return peek;
    }
    
    if (pqueue_scheme != PPRI && pqueue_scheme != PSRTF) {
        return job_evicted;
    }
    
    if ((pqueue_scheme == PPRI || pqueue_scheme == PSRTF)) {
        if (info->restart_time == 0) {
            ((job_info*)peek->metadata)->start_time = time;
        }
        return peek;
    } 

    if (info->restart_time == 0) {
        ((job_info*)peek->metadata)->start_time = time;
    }
   
    return peek;
}

void scheduler_job_finished(job *job_done, double time) {
    job_info* info = job_done->metadata;
    info->end_time = time;
    info->response_time = info->start_time - info->arrive_time;
    info->wait_time = info->end_time - info->arrive_time - info->running_time;
    info->turnaround_time = info->end_time - info->arrive_time;

    total_turnaround += info->turnaround_time;
    total_response += info->response_time;
    total_waiting += info->wait_time;
    size++;
    priqueue_poll(&pqueue);
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    return total_waiting / size;
}

double scheduler_average_turnaround_time() {
    return total_turnaround / size;
}

double scheduler_average_response_time() {
    return total_response / size;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
