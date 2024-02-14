/**
 * mini_memcheck
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Your tests here using malloc and free
    void *p1 = malloc(30);
    void *p2 = calloc(0, 20);
    int i = 4;
    //void *p3 = realloc(&i, 50);
    free(p1);
    free(p1);
    // free(p3);
    //printf("FINISH \n");
    return 0;
}