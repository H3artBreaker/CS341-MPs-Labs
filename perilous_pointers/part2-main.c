/**
 * perilous_pointers
 * CS 341 - Spring 2024
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);

    //void second_step(int *value);

    int* value = malloc(sizeof(int));
    *value = 132;
    second_step(value);
    free(value);
    value = NULL;

    //void double_step(int **value);
    int** ds = malloc(sizeof(int*));
    *ds = malloc(sizeof(int));
    **ds = 8942;
    double_step(ds);
    free(*ds);
    *ds = NULL;
    free(ds);

    //void strange_step(char *value);

    char v[] = {0,0,0,0,0,15,0,0,0}; 
    //In C, when you initialize an array with specific integer values like {0,0,0,0,0,15,0,0,0,0,0,0,0,0} these integers are directly represented by their binary values in memory. 
    //This is different from initializing a string with characters, where each character is stored as its ASCII value.
    //char *v = "000001500000";
    strange_step(v);

    // void empty_step(void *value)
    empty_step("123"); //void pointer can be used to point to any data type (it can hold any data type's memory address). Here, we passed in a char* but the function will be treated as a void pointer. 
    //In the function we cast it back to char*.

    // void two_step(void *s, char *s2)
    char *s2 = "rtyu";
    char *s = s2;
    two_step(s, s2);

    // void three_step(char *first, char *second, char *third)
    char* first = "123456";
    char* second = first + 2;
    char* third = second + 2;
    three_step(first, second, third);

    // void step_step_step(char *first, char *second, char *third)
    char fsss[] = {1, 1};
    char ssss[] = {2, 2, 9};
    char tsss[] = {3, 3, 3, 17};
    step_step_step(fsss, ssss, tsss);

    //void it_may_be_odd(char *a, int b)
    char imbo[] = {2};
    it_may_be_odd(imbo, 2);

    //void tok_step(char *str)
    char ts[] = " ,CS341";
    tok_step(ts);

    //void the_end(void *orange, void *blue)
    char blue[] = {1, 2, 0, 0};
    char* orange = blue;
    the_end(orange, blue);
    


    
    



    
    
    return 0;
}
