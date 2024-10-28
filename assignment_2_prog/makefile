# Variables
CC = gcc
CFLAGS = -o

# Targets
all: process 2_run

# Compile sigtrap.c to create process executable
process: sigtrap.c
	$(CC) $(CFLAGS) process sigtrap.c

# Compile pcb.c and 2.c to create the 2 executable
2: pcb.c 2.c
	$(CC) $(CFLAGS) 2 pcb.c 2.c

# Run the 2 executable with test1 argument
2_run: 2
	./2 test1