/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"

int main() {
    // Enter the string you want to test with the reference here.
    // char *input = " DDsa. 9S 6st.";
    //char *input = " DDsa. 9S 6st . 7sTd.";
    //char *input = " DDsa. 9S 6st. 3Rings. 3r4ings 7sTd.";
    //char *input = "\5 test.";
    char* input = "\x0f\x0f restofthestring.";
    //char *input = "3 rings.";

    // This function prints the reference implementation output on the terminal.
    print_camelCaser(input);

    // Put your expected output for the given input above.
    char *correct[] = {"\xf", "Restofthestring", NULL};
    // printf("The size of input is %ld, and the size of correct is %ld \n", sizeof(input), sizeof(correct));
    char *wrong[] = {"hello", "welcomeToCs241", NULL};

    // Compares the expected output you supplied with the reference output.
    printf("check_output test 1: %d\n", check_output(input, correct));
    printf("check_output test 2: %d\n", check_output(input, wrong));

    // Feel free to add more test cases.
    return 0;
}
