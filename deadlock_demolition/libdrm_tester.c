// /**
//  * deadlock_demolition
//  * CS 341 - Spring 2024
//  */
// #include "libdrm.h"

// #include <assert.h>
// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// int main() {
//     drm_t *drm = drm_init();
//     // TODO your tests here
//     drm_destroy(drm);
//     //drm = drm_init();

//     return 0;
// }

#include "libdrm.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void* thread_func(void* arg) {
    drm_t* my_drm = (drm_t*)arg;
    pthread_t thread_id = pthread_self();

    printf("Thread %ld: Attempting to lock DRM...\n", (long)thread_id);
    if (drm_wait(my_drm, &thread_id) == 1) {
        printf("Thread %ld: Locked DRM.\n", (long)thread_id);
        sleep(1); // Simulate some work with the locked resource
        printf("Thread %ld: Attempting to unlock DRM...\n", (long)thread_id);
        if (drm_post(my_drm, &thread_id) == 1) {
            printf("Thread %ld: Unlocked DRM.\n", (long)thread_id);
        } else {
            printf("Thread %ld: Failed to unlock DRM.\n", (long)thread_id);
        }
    } else {
        printf("Thread %ld: Could not lock DRM, potential deadlock avoided.\n", (long)thread_id);
    }

    return NULL;
}

int main() {
    const int num_threads = 2;
    pthread_t threads[num_threads];
    drm_t* drms[num_threads];

    // Initialize DRM locks
    for (int i = 0; i < num_threads; ++i) {
        drms[i] = drm_init();
        if (!drms[i]) {
            fprintf(stderr, "Failed to initialize DRM lock %d.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Create threads that will try to lock and unlock the DRMs
    for (int i = 0; i < num_threads; ++i) {
        if (pthread_create(&threads[i], NULL, thread_func, drms[i])) {
            fprintf(stderr, "Failed to create thread %d.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Clean up DRM locks
    for (int i = 0; i < num_threads; ++i) {
        drm_destroy(drms[i]);
    }

    return EXIT_SUCCESS;
}
