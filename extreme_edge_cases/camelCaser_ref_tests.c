/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"

int main() {
    // Enter the string you want to test with the reference here.
    // char *input = " DDsa. 9S 6st.";
    char *input = " DDsa. 9S 6st . 7sTd.";
    //char *input = " DDsa. 9S 6st. 3Rings. 3r4ings 7sTd.";

    // This function prints the reference implementation output on the terminal.
    print_camelCaser(input);

    // Put your expected output for the given input above.
    char *correct[] = {"ddsa","9s6St","3rings","3r4ings7Std",NULL};
    char *wrong[] = {"hello", "welcomeToCs241", NULL};

    // Compares the expected output you supplied with the reference output.
    printf("check_output test 1: %d\n", check_output(input, correct));
    printf("check_output test 2: %d\n", check_output(input, wrong));

    // Feel free to add more test cases.
    return 0;
}
