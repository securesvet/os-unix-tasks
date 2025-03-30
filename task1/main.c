#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define DEFAULT_BLOCK_SIZE 4096

void create_sparse_file(const char *input_file, const char *output_file, size_t block_size) {
    int input_fd = (input_file) ? open(input_file, O_RDONLY) : STDIN_FILENO;
    if (input_fd < 0) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd < 0) {
        perror("Error opening output file");
        close(input_fd);
        exit(EXIT_FAILURE);
    }

    char *buffer = (char *)malloc(block_size);
    if (!buffer) {
        perror("Memory allocation failed");
        close(input_fd);
        close(output_fd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    off_t offset = 0;

    while ((bytes_read = read(input_fd, buffer, block_size)) > 0) {
        int is_zero = 1;
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i] != 0) {
                is_zero = 0;
                break;
            }
        }

        if (is_zero) {
            if (lseek(output_fd, bytes_read, SEEK_CUR) == (off_t)-1) {
                perror("Error seeking in output file");
                free(buffer);
                close(input_fd);
                close(output_fd);
                exit(EXIT_FAILURE);
            }
        } else {
            if (write(output_fd, buffer, bytes_read) != bytes_read) {
                perror("Error writing to output file");
                free(buffer);
                close(input_fd);
                close(output_fd);
                exit(EXIT_FAILURE);
            }
        }

        offset += bytes_read;
    }

    if (bytes_read < 0) {
        perror("Error reading input file");
    }

    if (ftruncate(output_fd, offset) == -1) {
        perror("Error truncating output file");
    }

    free(buffer);
    close(input_fd);
    close(output_fd);
}

int main(int argc, char *argv[]) {
    const char *input_file = NULL, *output_file = NULL;
    size_t block_size = DEFAULT_BLOCK_SIZE;
    int opt;

    while ((opt = getopt(argc, argv, "b:i:o:")) != -1) {
        switch (opt) {
            case 'b':
                block_size = strtoul(optarg, NULL, 10);
                if (block_size == 0) {
                    fprintf(stderr, "Invalid block size\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                input_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -o output_file [-i input_file] [-b block_size]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!output_file) {
        fprintf(stderr, "Output file must be specified\n");
        exit(EXIT_FAILURE);
    }

    create_sparse_file(input_file, output_file, block_size);
    return 0;
}
