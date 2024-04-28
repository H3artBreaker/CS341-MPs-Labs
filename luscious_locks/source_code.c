#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Comment Removed
#define NUM_PHASES 6
// Comment Removed
// Comment Removed
int my_number;

// Comment Removed
bool phase0(char *input) {
    return !strcmp(input, "phase0\n");
}

    // Comment Removed
bool phase1(char *input) {
    int num;
    if (sscanf(input, "%d\n", &num) != 1) {
        return false;
    }

    return num == my_number;
}

// Comment Removed
bool phase2(char *input) {
    int nums[5];
    if (sscanf(input, "%d %d %d %d %d\n", nums, nums + 1, nums + 2, nums + 3,
                nums + 4) != 5) {
        return false;
    }

    for (int i = 1; i < 5; i++) {
        if (nums[i] < my_number || nums[i] < nums[i - 1]) {
            return false;
        }
    }

    return true;
}

// Comment Removed
bool phase3(char *input) {
    int nums[7];
    if (sscanf(input, "%d %d %d %d %d %d %d\n", &nums[0], &nums[1], &nums[2],
                &nums[3], &nums[4], &nums[5], &nums[6]) != 7) {
        return false;
    }

    int a = 0;
    int b = 1;
    int c = 1;
    while (a < my_number % 5) {
        int d = b + c;
        b = c;
        c = d;
        a++;
    }

    if (nums[0] != b || nums[1] != c) {
        return false;
    }
    //b == 3 == nums[0], c == 5 == nums[1]
    // 3 5 8 13 21 34 55 
    int x = nums[0];  //x = 3
    int y = nums[1];  //y = 5
    int z = 2;
    while (z < 7) {
        if (nums[z] != x + y) {
            return false;
        }
        x = y;
        y = nums[z];
        z++;
    }

    return true;
}

// Comment Removed
static int phase4_required = 5;
static int phase4_count = 0;
bool phase4(char *input) {
    if (strlen(input) <= 2) {
        return phase4_count == phase4_required;
    }

    if (input[1] != input[0] + (my_number % 3)) {
        return false;
    }

    phase4_count++;

    return phase4(input + 1);
}

// Comment Removed
bool phase5(char *input) {
    int x;
    if (sscanf(input, "%d\n", &x) != 1) {
        return false;
    }

    int z = 0;
    while (z < 233 * 225 * 173 * 125) {
        z++;
    }

    int a = z & 241;

    return (x & a) == (my_number & a);
}

// Comment Removed
// Comment Removed
bool (*phases[NUM_PHASES])(char *input) = {phase0, phase1, phase2,
                                        phase3, phase4, phase5};

// Comment Removed
// Comment Removed
int generate_number(char *netid) {
    unsigned int n = netid[0];
    for (int i = 1; i < strlen(netid); i++) {
        n ^= netid[i];
    }

    srand(n);
    return rand();
}

// Comment Removed
// Comment Removed
void run(FILE *input) {
    char *line = NULL;
    size_t len = 0;

    size_t phase = 0;

    // Comment Removed
    while (phase < NUM_PHASES && getline(&line, &len, input) != -1) {
        if (!((phases[phase])(line))) {
            printf("Failed on phase %zu\n", phase);
            exit(1);
        }

        printf("Completed phase %zu\n", phase);
        phase++;
    }

    // Comment Removed
    if (phase < NUM_PHASES) {
        printf("Failed on phase %zu\n", phase);
        exit(1);
    }

    free(line);
}

// Comment Removed
int main(int argc, char **argv) {
    // Comment Removed
    if (argc != 2 && argc != 3) {
        printf("Interactive mode: ./safe <netid>\n");
        printf("File mode:        ./safe <netid> <input_file>\n");
        exit(1);
    }

    // Comment Removed
    FILE *input = stdin;
    if (argc == 3) {
        input = fopen(argv[2], "r");
        if (!input) {
            perror(argv[2]);
            exit(1);
        }
    }

    // Comment Removed
    my_number = generate_number(argv[1]);
    run(input);

    // Comment Removed
    if (argc == 3) {
        fclose(input);
    }

    printf("Sucessfully unlocked the safe!\n");

    return 0;
}