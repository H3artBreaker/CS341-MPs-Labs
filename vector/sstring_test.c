/**
 * vector
 * CS 341 - Spring 2024
 */
//#include "sstring.h"

// int main() {
//     // TODO create some tests
//     // sstring *str1 = cstr_to_sstring("abc");
//     // sstring *str2 = cstr_to_sstring("def");
//     // int len = sstring_append(str1, str2); // len == 6
//     // printf("sstring is %s with length %d \n ", sstring_to_cstr(str1), len); // == "abcdef"
//     sstring *str1 = cstr_to_sstring("abcdeefg");
//     vector* vec = sstring_split(str1, 'e'); // == [ "abcd", "", "fg" ]);
//     for (size_t i = 0; i < vector_size(vec); i++) {
//         printf("each part is %s \n ", (char*)vector_get(vec, i));
//     }
//     return 0;
// }

/**
 * vector
 * CS 341 - Spring 2024
 */
// #include "vector.c"
#include "sstring.h"

void p_elements(vector* v) {
    printf("[");
    size_t i = 0;
    if (i < vector_size(v)) printf("%p", vector_get(v, i));
    for (i = 1; i < vector_size(v); i++) {
        printf(",%p", vector_get(v, i));
    }
    printf("]\n");
}
int main(int argc, char *argv[]) {
   sstring* ss1 = cstr_to_sstring("apple,banana,orange,,"); 
    char* str1 = sstring_to_cstr(ss1);
    printf("%s\n", str1);
    free(str1);
    vector* v1 = sstring_split(ss1, ',');
    p_elements(v1);
    vector_destroy(v1);
    sstring_destroy(ss1);
    // sstring* ss2 = cstr_to_sstring("This is a sentence.");
    // char* str2 = sstring_to_cstr(ss2);
    // printf("%s\n", str2);
    // free(str2);
    // vector* v2 = sstring_split(ss2, ' ');
    // p_elements(v2);
    // vector_destroy(v2);
    // sstring_destroy(ss2);
    return 0;
}