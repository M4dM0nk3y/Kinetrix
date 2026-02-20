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
static void scan_for_pins(ASTNode *node, int *pins, int *count) {
    if (!node || *count >= 50) return;
    
    switch (node->type) {
        case NODE_GPIO_WRITE:
        case NODE_GPIO_READ:
        case NODE_ANALOG_WRITE:
        case NODE_ANALOG_READ:
        case NODE_SERVO_WRITE:
        case NODE_TONE:
        case NODE_NOTONE:
            // Extract pin number if it's a constant
            if (node->data.gpio.pin && node->data.gpio.pin->type == NODE_NUMBER) {
                int pin = (int)node->data.gpio.pin->data.number.value;
                if (!pin_exists(pins, *count, pin)) {
                    pins[(*count)++] = pin;
                }
            }
            break;
            
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++) {
                scan_for_pins(node->data.block.statements[i], pins, count);
            }
            break;
            
        case NODE_IF:
            scan_for_pins(node->data.if_stmt.then_block, pins, count);
            scan_for_pins(node->data.if_stmt.else_block, pins, count);
            break;
            
        case NODE_WHILE:
            scan_for_pins(node->data.while_loop.body, pins, count);
            break;
            
        case NODE_REPEAT:
            scan_for_pins(node->data.repeat_loop.body, pins, count);
            break;
            
        case NODE_FOREVER:
            scan_for_pins(node->data.forever_loop.body, pins, count);
            break;
            
        default:
            break;
    }
}

// Track pins used in a program for diagnostics
void ast_track_pins(ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;
    
    int pins[50];  // Max 50 pins
    int count = 0;
    
    // Scan main block
    scan_for_pins(program->data.program.main_block, pins, &count);
    
    // Scan all functions
    for (int i = 0; i < program->data.program.function_count; i++) {
        scan_for_pins(program->data.program.functions[i]->data.function_def.body, pins, &count);
    }
    
    // Store results
    if (count > 0) {
        program->data.program.pins_used = malloc(sizeof(int) * count);
        memcpy(program->data.program.pins_used, pins, sizeof(int) * count);
        program->data.program.pin_count = count;
    }
}
