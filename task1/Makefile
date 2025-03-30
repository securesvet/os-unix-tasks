CC = gcc
CFLAGS = -Wall -Wextra -O2

all: main generate_test_file

main: main.c
	$(CC) $(CFLAGS) main.c -o main

generate_test_file: generate_test_file.c
	$(CC) $(CFLAGS) generate_test_file.c -o generate_test_file

clean:
	rm -f main generate_test_file fileA fileB fileC fileD fileA.gz fileB.gz result.txt
