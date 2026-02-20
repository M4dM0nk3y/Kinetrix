# Kinetrix Compiler Makefile
# Multi-pass architecture with AST

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = 

# Source files
SRCS = ast.c symbol_table.c error.c parser.c codegen.c pin_tracker.c diagnostics.c
OBJS = $(SRCS:.c=.o)

# Output
TARGET = kcc

# Default target
all: $(TARGET)

# New V3.0 compiler
$(TARGET): $(OBJS) compiler_v3.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Demo compiler
demo: $(OBJS) compiler_v3_demo.c
	$(CC) $(CFLAGS) -o kcc_demo $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Legacy compiler (for comparison)
legacy: compiler.c
	$(CC) $(CFLAGS) -o kcc_legacy compiler.c

clean:
	rm -f $(OBJS) $(TARGET) kcc_demo kcc_legacy *.ino

test: $(TARGET)
	./$(TARGET) test_led.kx

.PHONY: all clean test legacy demo

