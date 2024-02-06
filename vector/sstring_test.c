/**
 * vector
 * CS 341 - Spring 2024
 */
#include "sstring.h"

int main() {
    // TODO create some tests
    // sstring *str1 = cstr_to_sstring("abc");
    // sstring *str2 = cstr_to_sstring("def");
    // int len = sstring_append(str1, str2); // len == 6
    // printf("sstring is %s with length %d \n ", sstring_to_cstr(str1), len); // == "abcdef"
    sstring *str1 = cstr_to_sstring("abcdeefg");
    vector* vec = sstring_split(str1, 'e'); // == [ "abcd", "", "fg" ]);
    for (size_t i = 0; i < vector_size(vec); i++) {
        printf("each part is %s \n ", (char*)vector_get(vec, i));
    }
    return 0;
}
