/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int main() {
    //Feel free to add more test cases of your own!
    if (test_camelCaser(&camel_caser, &destroy)) {
        printf("SUCCESS\n");
    } else {
        printf("FAILED\n");
    }
    //const char *input_str = " DDsa. 9S 6st.";
    //const char *input_str = " DDsa. 9S 6st. 3Rings. 3r4ings 7sTd.";
    // const char *input_str = " DDsa. 9S 6st . 7sTd.";
    // char **output = camel_caser(input_str);
    // // printf("%s \n", input_str);
    // // printf("Starts!!! \n");
    // //printf("The output is : %s \n ", output[0]);
    // for (size_t i = 0; output[i] != NULL; i++) {
    //     printf("The output is : %s at i = %zu \n", output[i], i);
    // }

    return 0;
}
