//author: panyuec2
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>


#define BUF_SIZE 999

void redact_digits(const char *input) {
    while (*input) {
        if (*input >= '0' && *input <= '9')
            putchar('*');
        else
            putchar(*input);
        input++;
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_filename> <command1> <command2>\n", argv[0]);
        exit(1);
    }

    int fd1[2]; int fd2[2]; int fd3[2]; int fd4[2];
    if (pipe(fd1) == -1 || pipe(fd2) == -1 || pipe(fd3) == -1 || pipe(fd4) == -1) {
        perror("pipe error");
        exit(1);
    }

    const char* filename = argv[1];
    int use_alternative = 0;
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        filename = (access(filename, F_OK) == -1) ? "/dev/urandom" : "/etc/services"; //https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
        file_fd = open(filename, O_RDONLY);
        if (file_fd == -1) {
            perror("Failed to open alternative files");
            exit(2);
        }
        use_alternative = 1;
    }

    //send all the data from file to the first command pipe
    char buf[BUF_SIZE];
    size_t byte_read = 0;
    if (use_alternative) {
        // Code to execute if filename matches "/dev/urandom" or "/etc/services"
        byte_read = read(file_fd, buf, BUF_SIZE);
        if (byte_read == BUF_SIZE) {
            write(fd1[1], buf, byte_read);
        } else {
            perror("Read error");
            close(file_fd);
            exit(2);
        }
    } else {
        while ((byte_read = read(file_fd, buf, BUF_SIZE)) > 0) {
            write(fd1[1], buf, byte_read);
        }
    }

    close(file_fd);
    close(fd1[1]); // Parent closes write end

    //run the first command
    int pid1 = fork();
    if (pid1 < 0) {
        perror("FORK1 ERROR");
        exit(2);
    } else if (pid1 == 0) {
        dup2(fd1[0], STDIN_FILENO);
        close(fd1[0]);
        close(fd2[0]);
        dup2(fd2[1], STDOUT_FILENO);

        execlp(argv[2], argv[2], (char *)NULL);
        perror("execlp command1");
        exit(2);
    } else {
        int status;
        waitpid(pid1, &status, 0);

        close(fd1[0]);
        close(fd2[1]);
    }


    char output_buf[BUF_SIZE];
    char copy_buf[BUF_SIZE];  
    ssize_t count = read(fd2[0], output_buf, BUF_SIZE);
    if (count == -1) {
        perror("read from command1");
        exit(2);
    }
    close(fd2[0]);
    memcpy(copy_buf, output_buf, count);  // Copy the data

    char path[1024];
    snprintf(path, sizeof(path), "%s/mypassword", getenv("HOME"));

    int out_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd == -1) {
        perror("open mypassword");
        exit(2);
    }
    write(out_fd, output_buf, count);
    close(out_fd);

    write(fd3[1], copy_buf, count);
    close(fd3[1]);


    int pid2 = fork();
    if (pid2 < 0) {
        perror("FORK2 ERROR");
        exit(2);
    } else if (pid2 == 0) {
        dup2(fd3[0], STDIN_FILENO);
        close(fd3[0]);
        dup2(fd4[1], STDOUT_FILENO);
        close(fd4[1]);
        execlp(argv[3], argv[3], (char *)NULL);
    } else {
        int status;
        waitpid(pid2, &status, 0);
    }

    char final_buf[BUF_SIZE + 1];
    ssize_t final_count = read(fd4[0], final_buf, BUF_SIZE);
    printf("THE FINAL COUNT IS %lu\n", final_count);
    close(fd4[0]);
    if (final_count == -1) {
        perror("read from command2");
        exit(2);
    }
    output_buf[final_count] = '\0';
    // printf("Buffer has %s \n", final_buf);
    // printf("before redact\n");
    redact_digits(final_buf);
    
    return 0;
}