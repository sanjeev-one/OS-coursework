# Variables
CC = gcc
CFLAGS = -o

# Targets
all: process scheduler_run

# Compile sigtrap.c to create process executable
process: sigtrap.c
	$(CC) $(CFLAGS) process sigtrap.c

# Compile pcb.c and scheduler.c to create the scheduler executable
scheduler: pcb.c scheduler.c
	$(CC) $(CFLAGS) scheduler pcb.c scheduler.c

# Check if the input file "random_input" exists, and create it if it doesn't
check_random_exists:
	@if [ ! -f "random_input" ]; then \
		make generate_random_file; \
	fi

# Compile and run random.c to generate "random_input" file if needed
generate_random_file: random.c
	@rm -rf random
	@$(CC) -o random random.c -lm  # Add -lm to link the math library
	@./random random_input

# Run the scheduler executable with the "random_input" file by default
scheduler_run: scheduler check_random_exists
	./scheduler random_input

# Option to specify a custom input file (only checks process and scheduler)
run_with_input: process scheduler
	@echo "Please enter the name of your input file (default: random_input): "; \
	read input_file; \
	if [ -z "$$input_file" ]; then \
		input_file="random_input"; \
	fi; \
	if [ ! -f "$$input_file" ]; then \
		echo "Error: Input file '$$input_file' does not exist."; \
		exit 1; \
	fi; \
	./scheduler $$input_file

# Option to just run random.c to create a new "random_input" file
run_random: random.c
	$(CC) -o random random.c -lm  # Add -lm to link the math library
	./random random_input

# Clean up generated files
clean:
	rm -f process scheduler random random_input
