#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define FILE_SIZE (4 * 1024 * 1024 + 1)

void create_test_file(const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }

    lseek(fd, 0, SEEK_SET);
    write(fd, "1", 1);
    lseek(fd, 10000, SEEK_SET);
    write(fd, "1", 1);
    lseek(fd, FILE_SIZE - 1, SEEK_SET);
    write(fd, "1", 1);

    ftruncate(fd, FILE_SIZE);
    close(fd);
}

int main() {
    create_test_file("fileA");
    return 0;
}
