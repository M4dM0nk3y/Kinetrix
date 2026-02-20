/* Kinetrix Code Generator Implementation - Multi-Target Dispatcher */

#include "codegen.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// ── Target helpers ─────────────────────────────────────────────────────────

const char* target_name(Target t) {
    switch (t) {
        case TARGET_ARDUINO: return "Arduino";
        case TARGET_ESP32:   return "ESP32";
        case TARGET_RPI:     return "Raspberry Pi (Python)";
        case TARGET_PICO:    return "Raspberry Pi Pico (MicroPython)";
        case TARGET_ROS2:    return "ROS2";
        default:             return "Unknown";
    }
}

const char* target_extension(Target t) {
    switch (t) {
        case TARGET_ARDUINO: return ".ino";
        case TARGET_ESP32:   return ".cpp";
        case TARGET_RPI:     return ".py";
        case TARGET_PICO:    return ".py";
        case TARGET_ROS2:    return ".cpp";
        default:             return ".txt";
    }
}

// ── Constructor / Destructor ───────────────────────────────────────────────

CodeGen* codegen_create(FILE *output) {
    return codegen_create_for_target(output, TARGET_ARDUINO);
}

CodeGen* codegen_create_for_target(FILE *output, Target target) {
    CodeGen *gen = malloc(sizeof(CodeGen));
    gen->output           = output;
    gen->indent_level     = 0;
    gen->temp_var_counter = 0;
    gen->loop_counter     = 0;
    gen->target           = target;
    return gen;
}

void codegen_free(CodeGen *gen) { free(gen); }

// ── Shared emit helpers ────────────────────────────────────────────────────

void codegen_emit_indent(CodeGen *gen) {
    for (int i = 0; i < gen->indent_level; i++) fprintf(gen->output, "  ");
}

void codegen_emit(CodeGen *gen, const char *format, ...) {
    va_list args; va_start(args, format);
    vfprintf(gen->output, format, args);
    va_end(args);
}

void codegen_emit_line(CodeGen *gen, const char *format, ...) {
    codegen_emit_indent(gen);
    va_list args; va_start(args, format);
    vfprintf(gen->output, format, args);
    va_end(args);
    fprintf(gen->output, "\n");
}

// ── Main dispatcher ────────────────────────────────────────────────────────

void codegen_generate(CodeGen *gen, ASTNode *program) {
    switch (gen->target) {
        case TARGET_ESP32:   codegen_generate_esp32(gen, program);   break;
        case TARGET_RPI:     codegen_generate_rpi(gen, program);     break;
        case TARGET_PICO:    codegen_generate_pico(gen, program);    break;
        case TARGET_ROS2:    codegen_generate_ros2(gen, program);    break;
        case TARGET_ARDUINO:
        default:             codegen_generate_arduino(gen, program); break;
    }
}



// Forward declarations
static void codegen_expression(CodeGen *gen, ASTNode *node);
static void codegen_statement(CodeGen *gen, ASTNode *node);

// Generate expression
static void codegen_expression(CodeGen *gen, ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_NUMBER:
            codegen_emit(gen, "%g", node->data.number.value);
            break;
            
        case NODE_STRING:
            codegen_emit(gen, "\"%s\"", node->data.string.value);
            break;
            
        case NODE_BOOL:
            codegen_emit(gen, "%d", node->data.boolean.value);
            break;
            
        case NODE_IDENTIFIER:
            codegen_emit(gen, "%s", node->data.identifier.name);
            break;
            
        case NODE_BINARY_OP: {
            const char *op_str = "";
            switch (node->data.binary_op.op) {
                case OP_ADD: op_str = "+"; break;
                case OP_SUB: op_str = "-"; break;
                case OP_MUL: op_str = "*"; break;
                case OP_DIV: op_str = "/"; break;
                case OP_MOD: op_str = "%"; break;
                case OP_EQ:  op_str = "=="; break;
                case OP_NEQ: op_str = "!="; break;
                case OP_LT:  op_str = "<"; break;
                case OP_GT:  op_str = ">"; break;
                case OP_LTE: op_str = "<="; break;
                case OP_GTE: op_str = ">="; break;
                case OP_AND: op_str = "&&"; break;
                case OP_OR:  op_str = "||"; break;
                default: op_str = "?"; break;
            }
            
            // Special handling for modulo - cast to int
            if (node->data.binary_op.op == OP_MOD) {
                codegen_emit(gen, "((int)(");
                codegen_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, ") %s (int)(", op_str);
                codegen_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, "))");
            } else {
                codegen_emit(gen, "(");
                codegen_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, " %s ", op_str);
                codegen_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, ")");
            }
            break;
        }
            
        case NODE_CALL:
            codegen_emit(gen, "%s(", node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                if (i > 0) codegen_emit(gen, ", ");
                codegen_expression(gen, node->data.call.args[i]);
            }
            codegen_emit(gen, ")");
            break;
        
        case NODE_UNARY_OP:
            if (node->data.unary_op.op == OP_NOT) {
                codegen_emit(gen, "!(");
                codegen_expression(gen, node->data.unary_op.operand);
                codegen_emit(gen, ")");
            } else if (node->data.unary_op.op == OP_NEG) {
                codegen_emit(gen, "-(");
                codegen_expression(gen, node->data.unary_op.operand);
                codegen_emit(gen, ")");
            }
            break;


            
        case NODE_ARRAY_ACCESS:
            codegen_expression(gen, node->data.array_access.array);
            codegen_emit(gen, "[(int)(");
            codegen_expression(gen, node->data.array_access.index);
            codegen_emit(gen, ")]");
            break;
            
        case NODE_ANALOG_READ:
            codegen_emit(gen, "analogRead(A");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ")");
            break;
            
        case NODE_GPIO_READ:
            codegen_emit(gen, "digitalRead(");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ")");
            break;
            
        case NODE_PULSE_READ:
            codegen_emit(gen, "pulseIn(");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", HIGH)");
            break;
            
        case NODE_I2C_READ:
            codegen_emit(gen, "(Wire.requestFrom(");
            codegen_expression(gen, node->data.i2c.address);
            codegen_emit(gen, ", 1), Wire.read())");
            break;
            
        case NODE_MATH_FUNC: {
            const char *func_name = "";
            switch (node->data.math_func.func) {
                case MATH_SIN: func_name = "sin"; break;
                case MATH_COS: func_name = "cos"; break;
                case MATH_TAN: func_name = "tan"; break;
                case MATH_SQRT: func_name = "sqrt"; break;
                case MATH_ASIN: func_name = "asin"; break;
                case MATH_ACOS: func_name = "acos"; break;
                case MATH_ATAN: func_name = "atan"; break;
                case MATH_ATAN2: func_name = "atan2"; break;
            }
            codegen_emit(gen, "%s(", func_name);
            codegen_expression(gen, node->data.math_func.arg1);
            if (node->data.math_func.arg2) {
                codegen_emit(gen, ", ");
                codegen_expression(gen, node->data.math_func.arg2);
            }
            codegen_emit(gen, ")");
            break;
        }
            
        default:
            codegen_emit(gen, "/* unknown expression */");
            break;
    }
}

// Generate statement
static void codegen_statement(CodeGen *gen, ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_VAR_DECL:
            codegen_emit_line(gen, "float %s", node->data.var_decl.name);
            if (node->data.var_decl.initializer) {
                codegen_emit(gen, " = ");
                codegen_expression(gen, node->data.var_decl.initializer);
            }
            if (node->data.var_decl.is_array) {
                codegen_emit(gen, "[%d]", node->data.var_decl.array_size);
            }
            codegen_emit(gen, ";\n");
            break;
            
        case NODE_ASSIGNMENT: {
            codegen_emit_indent(gen);
            codegen_expression(gen, node->data.assignment.target);
            codegen_emit(gen, " = ");
            codegen_expression(gen, node->data.assignment.value);
            codegen_emit(gen, ";\n");
            break;
        }
            
        case NODE_IF:
            codegen_emit_line(gen, "if (");
            codegen_expression(gen, node->data.if_stmt.condition);
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            codegen_statement(gen, node->data.if_stmt.then_block);
            gen->indent_level--;
            codegen_emit_line(gen, "}");
            if (node->data.if_stmt.else_block) {
                codegen_emit(gen, " else {\n");
                gen->indent_level++;
                codegen_statement(gen, node->data.if_stmt.else_block);
                gen->indent_level--;
                codegen_emit_line(gen, "}\n");
            } else {
                codegen_emit(gen, "\n");
            }
            break;
            
        case NODE_WHILE:
            codegen_emit_line(gen, "while (");
            codegen_expression(gen, node->data.while_loop.condition);
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            codegen_statement(gen, node->data.while_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
            
        case NODE_REPEAT: {
            int loop_id = gen->loop_counter++;
            codegen_emit_line(gen, "for (int _i%d = 0; _i%d < (int)(", loop_id, loop_id);
            codegen_expression(gen, node->data.repeat_loop.count);
            codegen_emit(gen, "); _i%d++) {\n", loop_id);
            gen->indent_level++;
            codegen_statement(gen, node->data.repeat_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
        }
            
        case NODE_FOREVER:
            codegen_emit_line(gen, "while (1) {\n");
            gen->indent_level++;
            codegen_statement(gen, node->data.forever_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
            
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++) {
                codegen_statement(gen, node->data.block.statements[i]);
            }
            break;
            
        case NODE_RETURN:
            codegen_emit_line(gen, "return");
            if (node->data.return_stmt.value) {
                codegen_emit(gen, " ");
                codegen_expression(gen, node->data.return_stmt.value);
            }
            codegen_emit(gen, ";\n");
            break;
            
        case NODE_BREAK:
            codegen_emit_line(gen, "break;\n");
            break;
            
        case NODE_GPIO_WRITE:
            codegen_emit_indent(gen);
            codegen_emit(gen, "digitalWrite(");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            codegen_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;
            
        case NODE_ANALOG_WRITE:
            codegen_emit_indent(gen);
            codegen_emit(gen, "analogWrite(");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            codegen_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;
            
        case NODE_SERVO_WRITE:
            codegen_emit_line(gen, "analogWrite(");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", (");
            codegen_expression(gen, node->data.gpio.value);
            codegen_emit(gen, " * 255 / 180));\n");
            break;
            
        case NODE_I2C_BEGIN:
            codegen_emit_line(gen, "Wire.begin();\n");
            break;
            
        case NODE_I2C_START:
            codegen_emit_line(gen, "Wire.beginTransmission(");
            codegen_expression(gen, node->data.i2c.address);
            codegen_emit(gen, ");\n");
            break;
            
        case NODE_I2C_SEND:
            codegen_emit_line(gen, "Wire.write((byte)(");
            codegen_expression(gen, node->data.i2c.data);
            codegen_emit(gen, "));\n");
            break;
            
        case NODE_I2C_STOP:
            codegen_emit_line(gen, "Wire.endTransmission();\n");
            break;
            
        case NODE_TONE:
            codegen_emit_line(gen, "tone(");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            codegen_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;
            
        case NODE_NOTONE:
            codegen_emit_line(gen, "noTone(");
            codegen_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ");\n");
            break;
            
        case NODE_WAIT:
            codegen_emit_indent(gen);
            codegen_emit(gen, "delay(");
            codegen_expression(gen, node->data.unary.child);
            codegen_emit(gen, ");\n");
            break;
            
        case NODE_PRINT:
            codegen_emit_indent(gen);
            codegen_emit(gen, "Serial.println(");
            codegen_expression(gen, node->data.unary.child);
            codegen_emit(gen, ");\n");
            break;
            
        case NODE_CALL:
            codegen_emit_indent(gen);
            codegen_expression(gen, node);
            codegen_emit(gen, ";\n");
            break;
            
        case NODE_FUNCTION_DEF:
            // Emit function definition inline (for functions defined inside program block)
            if (node->data.function_def.is_extern) {
                codegen_emit_line(gen, "/* Extern %s function: %s */", 
                                  node->data.function_def.extern_lang, 
                                  node->data.function_def.name);
                break;
            }
            codegen_emit_line(gen, "void %s(", node->data.function_def.name);
            for (int i = 0; i < node->data.function_def.param_count; i++) {
                if (i > 0) codegen_emit(gen, ", ");
                codegen_emit(gen, "float %s", node->data.function_def.param_names[i]);
            }
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            codegen_statement(gen, node->data.function_def.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
            
        default:
            codegen_emit_line(gen, "/* unknown statement */\n");
            break;
    }
}

// Generate complete Arduino program
void codegen_generate_arduino(CodeGen *gen, ASTNode *program) {
    if (program == NULL || program->type != NODE_PROGRAM) {
        fprintf(stderr, "Error: Invalid program node\n");
        return;
    }
    
    // Emit includes
    codegen_emit_line(gen, "#include <Wire.h>\n");
    
    // Hoist function definitions from main block (emit them before setup)
    if (program->data.program.main_block &&
        program->data.program.main_block->type == NODE_BLOCK) {
        ASTNode *block = program->data.program.main_block;
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *stmt = block->data.block.statements[i];
            if (stmt && stmt->type == NODE_FUNCTION_DEF) {
                if (stmt->data.function_def.is_extern) {
                    codegen_emit_line(gen, "extern void %s(", stmt->data.function_def.name);
                    for (int j = 0; j < stmt->data.function_def.param_count; j++) {
                        if (j > 0) codegen_emit(gen, ", ");
                        codegen_emit(gen, "float %s", stmt->data.function_def.param_names[j]);
                    }
                    codegen_emit(gen, ");\n\n");
                    continue;
                }
                codegen_emit_line(gen, "void %s(", stmt->data.function_def.name);
                for (int j = 0; j < stmt->data.function_def.param_count; j++) {
                    if (j > 0) codegen_emit(gen, ", ");
                    codegen_emit(gen, "float %s", stmt->data.function_def.param_names[j]);
                }
                codegen_emit(gen, ") {\n");
                gen->indent_level++;
                codegen_statement(gen, stmt->data.function_def.body);
                gen->indent_level--;
                codegen_emit_line(gen, "}\n\n");
            }
        }
    }
    
    // Emit setup function
    codegen_emit_line(gen, "void setup() {\n");
    gen->indent_level++;
    codegen_emit_line(gen, "Serial.begin(9600);\n");
    codegen_emit_line(gen, "Serial.setTimeout(100);\n");
    codegen_emit_line(gen, "for (int i = 2; i <= 13; i++) pinMode(i, OUTPUT);\n");
    gen->indent_level--;
    codegen_emit_line(gen, "}\n\n");
    
    // Emit loop function (skip function defs - already hoisted)
    codegen_emit_line(gen, "void loop() {\n");
    gen->indent_level++;
    
    if (program->data.program.main_block) {
        ASTNode *block = program->data.program.main_block;
        if (block->type == NODE_BLOCK) {
            for (int i = 0; i < block->data.block.statement_count; i++) {
                ASTNode *stmt = block->data.block.statements[i];
                if (stmt && stmt->type != NODE_FUNCTION_DEF) {
                    codegen_statement(gen, stmt);
                }
            }
        } else {
            codegen_statement(gen, block);
        }
    }
    
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
}

