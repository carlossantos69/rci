# Compiler
CC = gcc
# Compiler flags
CFLAGS = -Wall -g

# Source files
SRCS = join.c tejo.c main.c
# Object files
OBJS = $(SRCS:.c=.o)
# Executable name
TARGET = cor

# Main target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
	@rm -f $(OBJS)

# Compile each .c file into .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony target to clean up auxiliary compiled files
.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)