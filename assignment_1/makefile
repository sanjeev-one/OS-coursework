
#flags and file names
CC = gcc
CFLAGS = -lpthread
TARGET = 1
SOURCE = assignment_1.c
#link target file to source
$(TARGET): $(SOURCE)
	$(CC)  -o $(TARGET) $(SOURCE) $(CFLAGS)
# setup run command that compiles and runs code
.PHONY: run
run: $(TARGET)
	./$(TARGET) 

