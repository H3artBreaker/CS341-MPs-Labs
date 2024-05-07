//author: panyuec2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


void swap(char* mmap1, char* mmap2, size_t size);


void reverse(char* data, size_t size) {
    for (size_t i = 0; i < size / 2; i++) {
        char temp = data[i];
        data[i] = data[size - i - 1];
        data[size - i - 1] = temp;
    }
}

int main(int argc, char** argv) {
    //either file1 or file2 are a symbolic link 
    if (argc != 3) {
        perror("Missing arguments; Usage: ./scramble <file1> <file2>\n");
        exit(1);
    }

    const char *file1 = argv[1];
    const char *file2 = argv[2];

    struct stat stat1, stat2;

    if (lstat(file1, &stat1) == -1 || lstat(file2, &stat2) == -1) {
        perror("Error opening files");
        return 2;
    }

    if (S_ISLNK(stat1.st_mode) || S_ISLNK(stat2.st_mode)) {
        printf("Symlink!\n");
        return 3;
    }

    if (stat1.st_size == 0 || stat2.st_size == 0) {
        printf("Empty!\n");
        return 4;
    }

    int fd1 = open(file1, O_RDWR);
    int fd2 = open(file2, O_RDWR);

    if (fd1 == -1 || fd2 == -1) {
        perror("Error opening files");
        return 2;
    }

    char* mmap1 = mmap(NULL, stat1.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
    char* mmap2 = mmap(NULL, stat2.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);

    if (mmap1 == MAP_FAILED || mmap2 == MAP_FAILED) {
        perror("Error mapping files");
        close(fd1);
        close(fd2);
        return 2;
    }

    if (stat1.st_ino == stat2.st_ino && stat1.st_dev == stat2.st_dev) {
        //we reverse it 
        reverse(mmap1, stat1.st_size);
    } else {
        size_t size = (stat1.st_size < stat2.st_size) ? stat1.st_size : stat2.st_size;
        swap(mmap1, mmap2, size);
    }

    munmap(mmap1, stat1.st_size);
    munmap(mmap2, stat2.st_size);
    close(fd1);
    close(fd2);

    return 0;
}

void swap(char* mmap1, char* mmap2, size_t size) {
    for (size_t i = 0; i < size; i += 2) {
        char temp = mmap1[i];
        mmap1[i] = mmap2[i];
        mmap2[i] = temp;
    }
}