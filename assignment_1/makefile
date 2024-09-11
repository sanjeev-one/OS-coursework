CC = gcc
CFLAGS = -lpthread
TARGET = 1
SOURCE = assignment_1.c

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

.PHONY: run
run: $(TARGET)
	./$(TARGET) 101 2 3 4

.PHONY: clean
clean:
	rm -f $(TARGET)