/* Kinetrix Code Generator - Multi-Target Backend
 * Supports: Arduino, ESP32, Raspberry Pi, Pico (MicroPython), ROS2
 */

#ifndef KINETRIX_CODEGEN_H
#define KINETRIX_CODEGEN_H

#include "ast.h"
#include <stdio.h>

// Supported compilation targets
typedef enum {
    TARGET_ARDUINO = 0,   // Arduino Uno/Mega/Nano (.ino)
    TARGET_ESP32,         // ESP32 / ESP8266 (.cpp)
    TARGET_RPI,           // Raspberry Pi Python/RPi.GPIO (.py)
    TARGET_PICO,          // Raspberry Pi Pico MicroPython (.py)
    TARGET_ROS2           // ROS2 C++ node (.cpp)
} Target;

// Code generator context
typedef struct {
    FILE    *output;
    int      indent_level;
    int      temp_var_counter;
    int      loop_counter;
    Target   target;           // Active compilation target
} CodeGen;

// Create/destroy code generator
CodeGen* codegen_create(FILE *output);
CodeGen* codegen_create_for_target(FILE *output, Target target);
void     codegen_free(CodeGen *gen);

// Main code generation (dispatches to correct backend)
void codegen_generate(CodeGen *gen, ASTNode *program);
void codegen_generate_diagnostics(CodeGen *gen, int *pins, int pin_count);

// Per-target generators
void codegen_generate_arduino(CodeGen *gen, ASTNode *program);
void codegen_generate_esp32(CodeGen *gen, ASTNode *program);
void codegen_generate_rpi(CodeGen *gen, ASTNode *program);
void codegen_generate_pico(CodeGen *gen, ASTNode *program);
void codegen_generate_ros2(CodeGen *gen, ASTNode *program);

// Shared helper utilities
void codegen_emit_indent(CodeGen *gen);
void codegen_emit(CodeGen *gen, const char *format, ...);
void codegen_emit_line(CodeGen *gen, const char *format, ...);

// Target name helper
const char* target_name(Target t);
const char* target_extension(Target t);

#endif // KINETRIX_CODEGEN_H

