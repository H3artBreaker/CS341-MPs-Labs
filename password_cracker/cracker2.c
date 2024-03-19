// /**
//  * password_cracker
//  * CS 341 - Spring 2024
//  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <pthread.h>
#include "cracker1.h"
#include "format.h"
#include "utils.h"

typedef struct {
    char *username;
    char *targetHash;
    char *partialPassword;
} CrackingTask;

static CrackingTask *currentTask = NULL;

static volatile int isFinished = 0;
static int numberOfThreads = 0;
static int isSuccess = 0;
static char* resultPassword = NULL;
static int totalAttempts = 0;

pthread_barrier_t barrierSync;
pthread_mutex_t mutexLock = PTHREAD_MUTEX_INITIALIZER;

void *performCracking(void* arg) {
    long threadId = (long)arg;
    struct crypt_data cryptData;
    cryptData.initialized = 0;
    char *trialPassword;
    
    while (1) {
        pthread_barrier_wait(&barrierSync);
        if (isFinished) break;

        trialPassword = strdup(currentTask->partialPassword);
        int count = 0;
        long startPosition, range;
        int remaining = strlen(trialPassword) - getPrefixLength(trialPassword);
        
        getSubrange(remaining, numberOfThreads, threadId, &startPosition, &range);
        setStringPosition(trialPassword + getPrefixLength(trialPassword), startPosition);
        
        v2_print_thread_start(threadId, currentTask->username, startPosition, trialPassword);
        int change = 2;
        for (long i = 0; i < range; i++) {
            if (isSuccess) {
                change = 1;
                break;
            } // If another thread has found the password

            count++;
            if (strcmp(crypt_r(trialPassword, "xx", &cryptData), currentTask->targetHash) == 0) {
                pthread_mutex_lock(&mutexLock);
                if (!isSuccess) {
                    isSuccess = 1;
                    change = 0;
                    free(resultPassword);
                    resultPassword = strdup(trialPassword); // Store the successful attempt
                }
                pthread_mutex_unlock(&mutexLock);
                break;
            }

            incrementString(trialPassword);
        }

        totalAttempts += count;

        free(trialPassword); // Free the trial password allocated for this thread

        v2_print_thread_result(threadId, count, change);

        pthread_barrier_wait(&barrierSync); // Sync up before moving to the next task or finishing
    }

    return NULL;
}

int start(size_t thread_count) {
    pthread_barrier_init(&barrierSync, NULL, thread_count + 1);
    numberOfThreads = thread_count;
    pthread_t threads[thread_count];
    
    currentTask = calloc(1, sizeof(CrackingTask));
    currentTask->username = calloc(20, sizeof(char));
    currentTask->targetHash = calloc(20, sizeof(char));
    currentTask->partialPassword = calloc(20, sizeof(char));
    resultPassword = calloc(10, sizeof(char));

    for (size_t i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, performCracking, (void*)(i + 1));
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, stdin) != -1) {
        if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';
        
        sscanf(line, "%s %s %s", currentTask->username, currentTask->targetHash, currentTask->partialPassword);

        v2_print_start_user(currentTask->username);

        double startTime = getTime(), startCpuTime = getCPUTime();
        
        isFinished = 0;
        pthread_barrier_wait(&barrierSync); // Kick off this round of cracking
        
        pthread_barrier_wait(&barrierSync); // Wait for all threads to finish

        double totalTime = getTime() - startTime, totalCpuTime = getCPUTime() - startCpuTime;
        v2_print_summary(currentTask->username, resultPassword, totalAttempts, totalTime, totalCpuTime, !isSuccess);

        isSuccess = 0; // Reset success flag for the next task
        totalAttempts = 0; // Reset total attempts counter
        free(resultPassword);
        resultPassword = calloc(10, sizeof(char)); // Prepare for next task
    }
    
    isFinished = 1;
    pthread_barrier_wait(&barrierSync); // Signal threads to exit

    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // Cleanup
    free(currentTask->username);
    free(currentTask->targetHash);
    free(currentTask->partialPassword);
    free(currentTask);
    free(resultPassword);
    free(line);

    pthread_mutex_destroy(&mutexLock);
    pthread_barrier_destroy(&barrierSync);

    return 0;
}

