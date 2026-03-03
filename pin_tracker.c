/* Pin Tracker for Hardware Diagnostics */

#include "ast.h"
#include <stdlib.h>
#include <string.h>

// Helper to check if pin already tracked
static int pin_exists(int *pins, int count, int pin) {
    for (int i = 0; i < count; i++) {
        if (pins[i] == pin) return 1;
    }
    return 0;
}

// Recursive function to scan AST for GPIO operations
static void scan_for_pins(ASTNode *node, int *out_pins, int *out_count, int *in_pins, int *in_count) {
    if (!node) return;
    
    switch (node->type) {
        // Output Pins
        case NODE_GPIO_WRITE:
        case NODE_ANALOG_WRITE:
        case NODE_SERVO_WRITE:
        case NODE_TONE:
        case NODE_NOTONE:
            if (*out_count >= 50) break;
            if (node->data.gpio.pin && node->data.gpio.pin->type == NODE_NUMBER) {
                int pin = (int)node->data.gpio.pin->data.number.value;
                if (!pin_exists(out_pins, *out_count, pin)) {
                    out_pins[(*out_count)++] = pin;
                }
            }
            break;
            
        // Input Pins
        case NODE_GPIO_READ:
        case NODE_ANALOG_READ:
        case NODE_PULSE_READ:
            if (*in_count >= 50) break;
            if (node->data.gpio.pin && node->data.gpio.pin->type == NODE_NUMBER) {
                int pin = (int)node->data.gpio.pin->data.number.value;
                if (!pin_exists(in_pins, *in_count, pin)) {
                    in_pins[(*in_count)++] = pin;
                }
            }
            break;
            
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++) {
                scan_for_pins(node->data.block.statements[i], out_pins, out_count, in_pins, in_count);
            }
            break;
            
        case NODE_IF:
            scan_for_pins(node->data.if_stmt.then_block, out_pins, out_count, in_pins, in_count);
            scan_for_pins(node->data.if_stmt.else_block, out_pins, out_count, in_pins, in_count);
            break;
            
        case NODE_WHILE:
            scan_for_pins(node->data.while_loop.body, out_pins, out_count, in_pins, in_count);
            break;
            
        case NODE_REPEAT:
            scan_for_pins(node->data.repeat_loop.body, out_pins, out_count, in_pins, in_count);
            break;
            
        case NODE_FOREVER:
            scan_for_pins(node->data.forever_loop.body, out_pins, out_count, in_pins, in_count);
            break;
            
        case NODE_VAR_DECL:
            scan_for_pins(node->data.var_decl.initializer, out_pins, out_count, in_pins, in_count);
            break;
            
        case NODE_ASSIGNMENT:
            scan_for_pins(node->data.assignment.value, out_pins, out_count, in_pins, in_count);
            break;
            
        case NODE_CALL:
            for (int i = 0; i < node->data.call.arg_count; i++) {
                scan_for_pins(node->data.call.args[i], out_pins, out_count, in_pins, in_count);
            }
            break;
            
        default:
            break;
    }
}

// Track pins used in a program for diagnostics and initialization
void ast_track_pins(ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;
    
    int out_pins[50];  // Max 50 pins
    int out_count = 0;
    
    int in_pins[50];
    int in_count = 0;
    
    // Scan main block
    scan_for_pins(program->data.program.main_block, out_pins, &out_count, in_pins, &in_count);
    
    // Scan all functions
    for (int i = 0; i < program->data.program.function_count; i++) {
        scan_for_pins(program->data.program.functions[i]->data.function_def.body, out_pins, &out_count, in_pins, &in_count);
    }
    
    // Store OUT results
    if (out_count > 0) {
        program->data.program.pins_used = malloc(sizeof(int) * out_count);
        memcpy(program->data.program.pins_used, out_pins, sizeof(int) * out_count);
        program->data.program.pin_count = out_count;
    }
    
    // Store IN results
    if (in_count > 0) {
        program->data.program.in_pins_used = malloc(sizeof(int) * in_count);
        memcpy(program->data.program.in_pins_used, in_pins, sizeof(int) * in_count);
        program->data.program.in_pin_count = in_count;
    }
}
