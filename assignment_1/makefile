CC = gcc
CFLAGS = -lpthread
TARGET = 1
SOURCE = assignment_1.c

$(TARGET): $(SOURCE)
	$(CC)  -o $(TARGET) $(SOURCE) $(CFLAGS)

.PHONY: run
run: $(TARGET)
	./$(TARGET) 

# ... existing code ...