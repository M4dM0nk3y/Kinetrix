# Kinetrix Compiler Makefile
# Multi-pass architecture with AST

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
RELEASE_CFLAGS = -Wall -Wextra -std=c99 -O2 -DNDEBUG
LDFLAGS = 

# Source files
SRCS = ast.c symbol_table.c error.c parser.c codegen.c codegen_esp32.c codegen_rpi.c codegen_pico.c codegen_ros2.c pin_tracker.c diagnostics.c
OBJS = $(SRCS:.c=.o)

# Output
TARGET = kcc

# Default target
all: $(TARGET)

OUTPUT_DIR = build

# New V3.0 compiler
$(TARGET): $(OBJS) compiler_v3.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Windows cross-compilation (release)
windows: $(SRCS) compiler_v3.c
	mkdir -p $(OUTPUT_DIR)
	x86_64-w64-mingw32-gcc $(RELEASE_CFLAGS) -o $(OUTPUT_DIR)/kcc.exe $^ $(LDFLAGS)

# Linux cross-compilation (release)
linux: $(SRCS) compiler_v3.c
	mkdir -p $(OUTPUT_DIR)
	x86_64-linux-gnu-gcc $(RELEASE_CFLAGS) -o $(OUTPUT_DIR)/kcc_linux $^ $(LDFLAGS)

# Mac compilation (Universal/Native, release)
mac: $(SRCS) compiler_v3.c
	mkdir -p $(OUTPUT_DIR)
	$(CC) $(RELEASE_CFLAGS) -o $(OUTPUT_DIR)/kcc_mac $^ $(LDFLAGS)

release: mac windows linux

# Demo compiler
demo: $(OBJS) compiler_v3_demo.c
	$(CC) $(CFLAGS) -o kcc_demo $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET) kcc_demo *.ino

test: $(TARGET)
	./$(TARGET) test_led.kx

.PHONY: all clean test demo

