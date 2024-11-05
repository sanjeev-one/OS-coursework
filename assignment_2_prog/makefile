# Variables
CC = gcc
CFLAGS = -o

# Targets
all: check_random_exists process 2_run

# Ensure process is always compiled by marking it as phony
.PHONY: process

# Compile sigtrap.c to create process executable
process: sigtrap.c
	$(CC) $(CFLAGS) process sigtrap.c

# Compile pcb.c and 2.c to create the 2 executable
2: pcb.c 2.c
	$(CC) $(CFLAGS) 2 pcb.c 2.c

# Check if the input file "random" exists, and create it if it doesn't
check_random_exists:
	@if [ ! -f "random" ]; then \
		make generate_random_file; \
	fi

# Compile and run random.c to generate "random" input file if needed
generate_random_file: random.c
	$(CC) -o random random.c
	./random random

# Run the 2 executable with the "random" input file by default
2_run: 2 check_random_exists
	./2 random

# Option to specify a custom input file
run_with_input:
	@echo "Please enter the name of your input file (default: random): "; \
	read input_file; \
	if [ -z "$$input_file" ]; then \
		input_file="random"; \
	fi; \
	if [ ! -f "$$input_file" ]; then \
		echo "Error: Input file '$$input_file' does not exist."; \
		exit 1; \
	fi; \
	./2 $$input_file

# Option to just run random.c to create a new "random" input file
run_random: random.c
	$(CC) -o random random.c
	./random random

# Clean up generated files
clean:
	rm -f process 2 random
