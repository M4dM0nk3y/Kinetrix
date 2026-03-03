/* Kinetrix ESP32 Code Generator
 * Target: ESP32 / ESP8266
 * Output: Arduino-compatible C++ with ESP32 extensions
 * Compile with: Arduino IDE (ESP32 board package) or arduino-cli
 */

#include "codegen.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Forward declarations
static void esp32_expression(CodeGen *gen, ASTNode *node);
static void esp32_statement(CodeGen *gen, ASTNode *node);

// ============================================================
// EXPRESSION GENERATION
// ============================================================

static void esp32_expression(CodeGen *gen, ASTNode *node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_NUMBER:
            codegen_emit(gen, "%g", node->data.number.value);
            break;
        case NODE_STRING:
            codegen_emit(gen, "\"%s\"", node->data.string.value);
            break;
        case NODE_BOOL:
            codegen_emit(gen, "%s", node->data.boolean.value ? "true" : "false");
            break;
        case NODE_IDENTIFIER:
            codegen_emit(gen, "%s", node->data.identifier.name);
            break;

        case NODE_BINARY_OP: {
            const char *op_str = "?";
            switch (node->data.binary_op.op) {
                case OP_ADD: op_str = "+";  break;
                case OP_SUB: op_str = "-";  break;
                case OP_MUL: op_str = "*";  break;
                case OP_DIV: op_str = "/";  break;
                case OP_MOD: op_str = "%";  break;
                case OP_EQ:  op_str = "=="; break;
                case OP_NEQ: op_str = "!="; break;
                case OP_LT:  op_str = "<";  break;
                case OP_GT:  op_str = ">";  break;
                case OP_LTE: op_str = "<="; break;
                case OP_GTE: op_str = ">="; break;
                case OP_AND: op_str = "&&"; break;
                case OP_OR:  op_str = "||"; break;
                default: break;
            }
            if (node->data.binary_op.op == OP_MOD) {
                codegen_emit(gen, "((int)(");
                esp32_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, ") %% (int)(");
                esp32_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, "))");
            } else if (node->data.binary_op.op == OP_ADD && 
                       ((node->data.binary_op.left->value_type && node->data.binary_op.left->value_type->kind == TYPE_STRING) || 
                        (node->data.binary_op.right->value_type && node->data.binary_op.right->value_type->kind == TYPE_STRING))) {
                codegen_emit(gen, "(String(");
                esp32_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, ") + String(");
                esp32_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, "))");
            } else if (node->data.binary_op.op == OP_DIV) {
                codegen_emit(gen, "((");
                esp32_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, ") == 0 ? 0 : ((");
                esp32_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, ") / (");
                esp32_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, ")))");
            } else {
                codegen_emit(gen, "(");
                esp32_expression(gen, node->data.binary_op.left);
                codegen_emit(gen, " %s ", op_str);
                esp32_expression(gen, node->data.binary_op.right);
                codegen_emit(gen, ")");
            }
            break;
        }

        case NODE_UNARY_OP:
            if (node->data.unary_op.op == OP_NOT) {
                codegen_emit(gen, "!(");
                esp32_expression(gen, node->data.unary_op.operand);
                codegen_emit(gen, ")");
            } else {
                codegen_emit(gen, "-(");
                esp32_expression(gen, node->data.unary_op.operand);
                codegen_emit(gen, ")");
            }
            break;

        case NODE_CALL:
            codegen_emit(gen, "%s(", node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                if (i > 0) codegen_emit(gen, ", ");
                esp32_expression(gen, node->data.call.args[i]);
            }
            codegen_emit(gen, ")");
            break;

        case NODE_ANALOG_READ:
            // ESP32 has 12-bit ADC (0-4095) — expose raw value
            codegen_emit(gen, "analogRead(A");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ")");
            break;

        case NODE_GPIO_READ:
            codegen_emit(gen, "digitalRead(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ")");
            break;

        case NODE_ARRAY_ACCESS:
            esp32_expression(gen, node->data.array_access.array);
            codegen_emit(gen, "[(int)(");
            esp32_expression(gen, node->data.array_access.index);
            codegen_emit(gen, ")]");
            break;

        case NODE_PULSE_READ:
            codegen_emit(gen, "pulseIn(");
            esp32_expression(gen, node->data.gpio.pin);
            if (node->data.gpio.value) {
                codegen_emit(gen, ", HIGH, ");
                esp32_expression(gen, node->data.gpio.value);
                codegen_emit(gen, ")");
            } else {
                codegen_emit(gen, ", HIGH, 23200)");
            }
            break;

        case NODE_I2C_READ:
            codegen_emit(gen, "(Wire.beginTransmission(");
            esp32_expression(gen, node->data.i2c.address);
            codegen_emit(gen, "), Wire.write(");
            esp32_expression(gen, node->data.i2c.data);    // register address
            codegen_emit(gen, "), Wire.endTransmission(false), Wire.requestFrom(");
            esp32_expression(gen, node->data.i2c.address);
            codegen_emit(gen, ", 1), Wire.read())");
            break;

        case NODE_RADIO_AVAILABLE:
            codegen_emit(gen, "radio_available()"); /* Expecting user-provided wrapper or C++ shim */
            break;

        case NODE_RADIO_READ:
            codegen_emit(gen, "radio_read()");
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
            esp32_expression(gen, node->data.math_func.arg1);
            if (node->data.math_func.arg2) {
                codegen_emit(gen, ", ");
                esp32_expression(gen, node->data.math_func.arg2);
            }
            codegen_emit(gen, ")");
            break;
        }

        case NODE_CAST:
            codegen_emit(gen, "(%s)(", type_to_ctype(node->data.cast_op.target_type));
            esp32_expression(gen, node->data.cast_op.operand);
            codegen_emit(gen, ")");
            break;

        case NODE_STRUCT_ACCESS:
            esp32_expression(gen, node->data.struct_access.object);
            codegen_emit(gen, ".%s", node->data.struct_access.member);
            break;

        case NODE_SERIAL_RECV:
            codegen_emit(gen, "Serial.read()");
            break;

        case NODE_SPI_TRANSFER:
            codegen_emit(gen, "SPI.transfer(");
            esp32_expression(gen, node->data.spi_transfer.data);
            codegen_emit(gen, ")");
            break;

        case NODE_I2C_DEVICE_READ:
            codegen_emit(gen, "(Wire.beginTransmission(");
            esp32_expression(gen, node->data.i2c_device_read.device_addr);
            codegen_emit(gen, "), Wire.write(");
            esp32_expression(gen, node->data.i2c_device_read.reg_addr);
            codegen_emit(gen, "), Wire.endTransmission(false), Wire.requestFrom(");
            esp32_expression(gen, node->data.i2c_device_read.device_addr);
            codegen_emit(gen, ", 1), Wire.read())");
            break;

        default:
            codegen_emit(gen, "0 /* unsupported expr */");
            break;
    }
}

// ============================================================
// STATEMENT GENERATION
// ============================================================

static void esp32_statement(CodeGen *gen, ASTNode *node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_VAR_DECL:
            codegen_emit_indent(gen);
            if (node->data.var_decl.is_const)
                codegen_emit(gen, "const ");
            if (node->data.var_decl.is_shared)
                codegen_emit(gen, "volatile ");
            
            codegen_emit(gen, "%s %s", type_to_ctype(node->data.var_decl.declared_type), node->data.var_decl.name);
            
            if (node->data.var_decl.initializer) {
                codegen_emit(gen, " = ");
                esp32_expression(gen, node->data.var_decl.initializer);
            }
            codegen_emit(gen, ";\n");
            break;

        case NODE_ASSIGNMENT:
            codegen_emit_indent(gen);
            esp32_expression(gen, node->data.assignment.target);
            codegen_emit(gen, " = ");
            esp32_expression(gen, node->data.assignment.value);
            codegen_emit(gen, ";\n");
            break;

        case NODE_BUFFER_PUSH:
            codegen_emit_line(gen,
                "%s[%s_head %% (sizeof(%s)/sizeof(%s[0]))] = ",
                node->data.buffer_push.buffer_name,
                node->data.buffer_push.buffer_name,
                node->data.buffer_push.buffer_name,
                node->data.buffer_push.buffer_name);
            esp32_expression(gen, node->data.buffer_push.value);
            codegen_emit(gen, "; %s_head++;\n", node->data.buffer_push.buffer_name);
            break;

        /* ---- Interrupts ---- */
        case NODE_INTERRUPT_PIN: {
            const char *mode_str = "RISING";
            if (node->data.interrupt_pin.mode == INT_MODE_FALLING)  mode_str = "FALLING";
            if (node->data.interrupt_pin.mode == INT_MODE_CHANGING) mode_str = "CHANGE";
            codegen_emit_line(gen,
                "attachInterrupt(digitalPinToInterrupt(%d), _isr_pin_%d, %s);\n",
                node->data.interrupt_pin.pin_number,
                node->data.interrupt_pin.pin_number,
                mode_str);
            break;
        }

        case NODE_INTERRUPT_TIMER:
            codegen_emit_line(gen, "hw_timer_t * timer_%d = timerBegin(%d, 80, true);\n", node->data.interrupt_timer.timer_id, node->data.interrupt_timer.timer_id % 4);
            codegen_emit_line(gen, "timerAttachInterrupt(timer_%d, &_isr_timer_%d, true);\n", node->data.interrupt_timer.timer_id, node->data.interrupt_timer.timer_id);
            codegen_emit_line(gen, "timerAlarmWrite(timer_%d, %d, true);\n", node->data.interrupt_timer.timer_id, 
                node->data.interrupt_timer.interval * (node->data.interrupt_timer.is_us ? 1 : 1000));
            codegen_emit_line(gen, "timerAlarmEnable(timer_%d);\n", node->data.interrupt_timer.timer_id);
            break;

        /* ---- UART ---- */
        case NODE_SERIAL_OPEN:
            codegen_emit_line(gen, "Serial.begin(%d);\n", node->data.serial_open.baud_rate);
            break;

        case NODE_SERIAL_SEND:
            codegen_emit_indent(gen);
            codegen_emit(gen, "Serial.print(");
            esp32_expression(gen, node->data.serial_send.value);
            codegen_emit(gen, ");\n");
            break;

        /* ---- I2C high-level ---- */
        case NODE_I2C_OPEN:
            codegen_emit_line(gen, "Wire.begin();\n");
            break;

        case NODE_I2C_DEVICE_WRITE:
            codegen_emit_indent(gen);
            codegen_emit(gen, "(Wire.beginTransmission(");
            esp32_expression(gen, node->data.i2c_device_write.device_addr);
            codegen_emit(gen, "), Wire.write(");
            esp32_expression(gen, node->data.i2c_device_write.value);
            codegen_emit(gen, "), Wire.endTransmission());\n");
            break;

        case NODE_I2C_DEVICE_READ_ARRAY:
            codegen_emit_indent(gen);
            codegen_emit(gen, "Wire.beginTransmission(");
            esp32_expression(gen, node->data.i2c_device_read_array.device_addr);
            codegen_emit(gen, "); Wire.write(");
            esp32_expression(gen, node->data.i2c_device_read_array.reg_addr);
            codegen_emit(gen, "); Wire.endTransmission(false); Wire.requestFrom(");
            esp32_expression(gen, node->data.i2c_device_read_array.device_addr);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.i2c_device_read_array.count);
            codegen_emit_line(gen, ");");
            codegen_emit_indent(gen);
            codegen_emit_line(gen, "for (int _i2c_i = 0; _i2c_i < (int)(");
            esp32_expression(gen, node->data.i2c_device_read_array.count);
            codegen_emit(gen, "); _i2c_i++) {\n");
            gen->indent_level++;
            codegen_emit_indent(gen);
            codegen_emit(gen, "if (Wire.available()) %s[_i2c_i] = Wire.read();\n", node->data.i2c_device_read_array.array_name);
            codegen_emit_indent(gen);
            codegen_emit(gen, "else %s[_i2c_i] = 0;\n", node->data.i2c_device_read_array.array_name);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;

        /* ---- SPI ---- */
        case NODE_SPI_OPEN:
            codegen_emit_line(gen, "SPI.begin(); SPI.setClockDivider(SPI_CLOCK_DIV%d);\n",
                /* closest divider */ (16000000 / (node->data.spi_open.frequency > 0 ? node->data.spi_open.frequency : 1000000) > 128 ? 128 : 16));
            break;

        case NODE_SPI_TRANSFER:
            codegen_emit_indent(gen);
            codegen_emit(gen, "SPI.transfer(");
            esp32_expression(gen, node->data.spi_transfer.data);
            codegen_emit(gen, ");\n");
            break;

        /* ---- Named devices ---- */
        case NODE_DEVICE_DEF:
            codegen_emit_indent(gen);
            codegen_emit(gen, "const int %s = ", node->data.device_def.device_name);
            esp32_expression(gen, node->data.device_def.address_or_baud);
            codegen_emit(gen, ";\n");
            break;

        case NODE_DEVICE_WRITE:
            codegen_emit_line(gen, "/* write device '%s': ", node->data.device_write.device_name);
            esp32_expression(gen, node->data.device_write.value);
            codegen_emit(gen, " */\n");
            break;

        /* ---- Error handling ---- */
        case NODE_TRY:
            codegen_emit_line(gen, "try {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.try_stmt.try_block);
            gen->indent_level--;
            codegen_emit_line(gen, "} catch (...) {\n");
            if (node->data.try_stmt.error_block) {
                gen->indent_level++;
                esp32_statement(gen, node->data.try_stmt.error_block);
                gen->indent_level--;
            }
            codegen_emit_line(gen, "}\n");
            break;

        case NODE_WATCHDOG_ENABLE:
            codegen_emit_line(gen, "esp_task_wdt_init(%d, true);\n", node->data.watchdog_enable.timeout_ms / 1000);
            codegen_emit_line(gen, "esp_task_wdt_add(NULL);\n");
            break;

        case NODE_WATCHDOG_FEED:
            codegen_emit_line(gen, "esp_task_wdt_reset();\n");
            break;

        case NODE_RADIO_SEND:
            codegen_emit_indent(gen);
            codegen_emit(gen, "radio_send_peer(");
            esp32_expression(gen, node->data.radio_send.peer_id);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.radio_send.data);
            codegen_emit(gen, ");\n");
            break;

        case NODE_DISABLE_INTERRUPTS:
            codegen_emit_line(gen, "noInterrupts();\n");
            break;

        case NODE_OTA_ENABLE:
            /* OTA setup is handled at the program level in setup() */
            break;

        case NODE_ENABLE_INTERRUPTS:
            codegen_emit_line(gen, "interrupts();\n");
            break;

        case NODE_ASSERT:
            codegen_emit_indent(gen);
            codegen_emit(gen, "if (!(");
            esp32_expression(gen, node->data.assert_stmt.condition);
            codegen_emit(gen, ")) { ");
            if (node->data.assert_stmt.action) {
                esp32_expression(gen, node->data.assert_stmt.action);
                codegen_emit(gen, "; ");
            } else {
                codegen_emit(gen, "while(1); /* halt */ ");
            }
            codegen_emit(gen, "}\n");
            break;

        /* ---- Structs ---- */
        case NODE_STRUCT_DEF: {
            codegen_emit_line(gen, "typedef struct {\n");
            gen->indent_level++;
            for (int i = 0; i < node->data.struct_def.field_count; i++) {
                StructField *f = &node->data.struct_def.fields[i];
                codegen_emit_line(gen, "%s %s;\n", type_to_ctype(f->type), f->name);
            }
            gen->indent_level--;
            codegen_emit_line(gen, "} %s;\n", node->data.struct_def.name);
            break;
        }

        case NODE_STRUCT_INSTANCE:
            codegen_emit_line(gen, "%s %s;\n",
                node->data.struct_instance.struct_type,
                node->data.struct_instance.var_name);
            break;

        /* ---- Tasks ---- */
        case NODE_TASK_DEF:
            codegen_emit_line(gen, "/* task '%s' defined globally via xTaskCreate */\n",
                node->data.task_def.name);
            break;

        case NODE_TASK_START:
            codegen_emit_line(gen, "xTaskCreate(_task_func_%s, \"%s\", 4096, NULL, 1, NULL);\n",
                node->data.task_start.task_name,
                node->data.task_start.task_name);
            break;

        case NODE_IF:
            codegen_emit_line(gen, "if (");
            esp32_expression(gen, node->data.if_stmt.condition);
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.if_stmt.then_block);
            gen->indent_level--;
            codegen_emit_line(gen, "}");
            if (node->data.if_stmt.else_block) {
                codegen_emit(gen, " else {\n");
                gen->indent_level++;
                esp32_statement(gen, node->data.if_stmt.else_block);
                gen->indent_level--;
                codegen_emit_line(gen, "}\n");
            } else {
                codegen_emit(gen, "\n");
            }
            break;

        case NODE_WHILE:
            codegen_emit_line(gen, "while (");
            esp32_expression(gen, node->data.while_loop.condition);
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.while_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;

        case NODE_REPEAT: {
            int id = gen->loop_counter++;
            codegen_emit_line(gen, "for (int _i%d = 0; _i%d < (int)(", id, id);
            esp32_expression(gen, node->data.repeat_loop.count);
            codegen_emit(gen, "); _i%d++) {\n", id);
            gen->indent_level++;
            esp32_statement(gen, node->data.repeat_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
        }

        case NODE_FOR: {
            int loop_id = gen->loop_counter++;
            codegen_emit_indent(gen);
            codegen_emit(gen, "int _start_%d = (", loop_id);
            esp32_expression(gen, node->data.for_loop.start_expr);
            codegen_emit(gen, ");\n");
            
            codegen_emit_indent(gen);
            codegen_emit(gen, "int _end_%d = (", loop_id);
            esp32_expression(gen, node->data.for_loop.end_expr);
            codegen_emit(gen, ");\n");
            
            codegen_emit_indent(gen);
            if (node->data.for_loop.step_expr) {
                codegen_emit(gen, "int _step_%d = (", loop_id);
                esp32_expression(gen, node->data.for_loop.step_expr);
                codegen_emit(gen, ");\n");
            } else {
                codegen_emit(gen, "int _step_%d = (_start_%d <= _end_%d) ? 1 : -1;\n", loop_id, loop_id, loop_id);
            }
            
            codegen_emit_line(gen, "for (int %s = _start_%d; _step_%d > 0 ? %s <= _end_%d : %s >= _end_%d; %s += _step_%d) {\n",
                node->data.for_loop.var_name, loop_id, loop_id,
                node->data.for_loop.var_name, loop_id,
                node->data.for_loop.var_name, loop_id,
                node->data.for_loop.var_name, loop_id);
            gen->indent_level++;
            esp32_statement(gen, node->data.for_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
        }

        case NODE_FOREVER:
            codegen_emit_line(gen, "while (1) {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.forever_loop.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;

        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++)
                esp32_statement(gen, node->data.block.statements[i]);
            break;

        case NODE_RETURN:
            codegen_emit_line(gen, "return");
            if (node->data.return_stmt.value) {
                codegen_emit(gen, " ");
                esp32_expression(gen, node->data.return_stmt.value);
            }
            codegen_emit(gen, ";\n");
            break;

        case NODE_BREAK:
            codegen_emit_line(gen, "break;\n");
            break;

        case NODE_CONTINUE:
            codegen_emit_line(gen, "continue;\n");
            break;

        case NODE_GPIO_WRITE:
            codegen_emit_indent(gen);
            codegen_emit(gen, "digitalWrite(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;

        case NODE_ANALOG_WRITE:
            codegen_emit_indent(gen);
            codegen_emit(gen, "_ledc_analogWrite(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;

        case NODE_WAIT:
            codegen_emit_indent(gen);
            codegen_emit(gen, "delay(");
            esp32_expression(gen, node->data.unary.child);
            codegen_emit(gen, ");\n");
            break;

        case NODE_PRINT:
            codegen_emit_indent(gen);
            codegen_emit(gen, "Serial.print(");
            esp32_expression(gen, node->data.unary.child);
            codegen_emit(gen, ");\n");
            break;

        case NODE_PRINTLN:
            codegen_emit_indent(gen);
            codegen_emit(gen, "Serial.println(");
            esp32_expression(gen, node->data.unary.child);
            codegen_emit(gen, ");\n");
            break;

        case NODE_CALL:
            codegen_emit_indent(gen);
            esp32_expression(gen, node);
            codegen_emit(gen, ";\n");
            break;

        case NODE_TONE:
            codegen_emit_line(gen, "tone(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ", ");
            esp32_expression(gen, node->data.gpio.value);
            codegen_emit(gen, ");\n");
            break;

        case NODE_NOTONE:
            codegen_emit_line(gen, "noTone(");
            esp32_expression(gen, node->data.gpio.pin);
            codegen_emit(gen, ");\n");
            break;

        case NODE_FUNCTION_DEF:
            if (node->data.function_def.is_extern) {
                codegen_emit_line(gen, "/* Extern %s function: %s */", 
                                  node->data.function_def.extern_lang, 
                                  node->data.function_def.name);
                break;
            }
            codegen_emit_line(gen, "%s %s(", 
                node->data.function_def.return_type ? type_to_ctype(node->data.function_def.return_type) : "void",
                node->data.function_def.name);
            for (int i = 0; i < node->data.function_def.param_count; i++) {
                if (i > 0) codegen_emit(gen, ", ");
                const char *pty = "float";
                if (node->data.function_def.param_types && node->data.function_def.param_types[i])
                    pty = type_to_ctype(node->data.function_def.param_types[i]);
                codegen_emit(gen, "%s %s", pty, node->data.function_def.param_names[i]);
            }
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            esp32_statement(gen, node->data.function_def.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;

        default:
            break;
    }
}

// ============================================================
// RECURSIVE PRE-PASSES (ISR HOISTING)
// ============================================================

static void esp32_assign_timer_ids(ASTNode *node, int *counter) {
    if (!node) return;
    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->data.program.function_count; i++) esp32_assign_timer_ids(node->data.program.functions[i], counter);
            esp32_assign_timer_ids(node->data.program.main_block, counter);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++) esp32_assign_timer_ids(node->data.block.statements[i], counter);
            break;
        case NODE_FUNCTION_DEF: esp32_assign_timer_ids(node->data.function_def.body, counter); break;
        case NODE_TASK_DEF: esp32_assign_timer_ids(node->data.task_def.body, counter); break;
        case NODE_IF:
            esp32_assign_timer_ids(node->data.if_stmt.then_block, counter);
            esp32_assign_timer_ids(node->data.if_stmt.else_block, counter);
            break;
        case NODE_WHILE: esp32_assign_timer_ids(node->data.while_loop.body, counter); break;
        case NODE_FOR: esp32_assign_timer_ids(node->data.for_loop.body, counter); break;
        case NODE_REPEAT: esp32_assign_timer_ids(node->data.repeat_loop.body, counter); break;
        case NODE_INTERRUPT_PIN: esp32_assign_timer_ids(node->data.interrupt_pin.body, counter); break;
        case NODE_INTERRUPT_TIMER:
            node->data.interrupt_timer.timer_id = (*counter)++;
            esp32_assign_timer_ids(node->data.interrupt_timer.body, counter);
            break;
        default: break;
    }
}

static void esp32_hoist_isrs(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->data.program.function_count; i++) esp32_hoist_isrs(gen, node->data.program.functions[i]);
            esp32_hoist_isrs(gen, node->data.program.main_block);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++) esp32_hoist_isrs(gen, node->data.block.statements[i]);
            break;
        case NODE_FUNCTION_DEF: esp32_hoist_isrs(gen, node->data.function_def.body); break;
        case NODE_TASK_DEF: esp32_hoist_isrs(gen, node->data.task_def.body); break;
        case NODE_IF:
            esp32_hoist_isrs(gen, node->data.if_stmt.then_block);
            esp32_hoist_isrs(gen, node->data.if_stmt.else_block);
            break;
        case NODE_WHILE: esp32_hoist_isrs(gen, node->data.while_loop.body); break;
        case NODE_FOR: esp32_hoist_isrs(gen, node->data.for_loop.body); break;
        case NODE_REPEAT: esp32_hoist_isrs(gen, node->data.repeat_loop.body); break;
        case NODE_INTERRUPT_PIN:
            codegen_emit_line(gen, "void IRAM_ATTR _isr_pin_%d() {\n", node->data.interrupt_pin.pin_number);
            gen->indent_level++;
            esp32_statement(gen, node->data.interrupt_pin.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n\n");
            esp32_hoist_isrs(gen, node->data.interrupt_pin.body);
            break;
        case NODE_INTERRUPT_TIMER:
            codegen_emit_line(gen, "void IRAM_ATTR _isr_timer_%d() {\n", node->data.interrupt_timer.timer_id);
            gen->indent_level++;
            esp32_statement(gen, node->data.interrupt_timer.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n\n");
            esp32_hoist_isrs(gen, node->data.interrupt_timer.body);
            break;
        default: break;
    }
}

// ============================================================
// PROGRAM ENTRY POINT
// ============================================================

void codegen_generate_esp32(CodeGen *gen, ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;

    // Header
    codegen_emit_line(gen, "// Generated by Kinetrix Compiler (Target: ESP32)");
    codegen_emit_line(gen, "// Board: ESP32 Dev Module");
    codegen_emit_line(gen, "// Upload via: Arduino IDE with ESP32 board package\n");
    codegen_emit_line(gen, "#include <Arduino.h>\n");
    codegen_emit_line(gen, "#include <WiFi.h>");
    codegen_emit_line(gen, "#include <ArduinoOTA.h>");
    codegen_emit_line(gen, "#include <esp_now.h>");
    codegen_emit_line(gen, "#include <esp_task_wdt.h>");
    codegen_emit_line(gen, "#include <esp32-hal-ledc.h>");
    codegen_emit_line(gen, "\n/* ESP32-specific declarations */");
    
    /* PWM Tracker map for dynamic LEDC channels */
    codegen_emit_line(gen, "int _esp32_pwm_channels[40] = {0};");
    codegen_emit_line(gen, "int _esp32_next_pwm_channel = 0;");
    
    codegen_emit_line(gen, "void _ledc_analogWrite(uint8_t pin, int value) {");
    codegen_emit_line(gen, "  if (_esp32_pwm_channels[pin] == 0) {");
    codegen_emit_line(gen, "    _esp32_pwm_channels[pin] = _esp32_next_pwm_channel + 1;");
    codegen_emit_line(gen, "    ledcSetup(_esp32_next_pwm_channel, 5000, 8); // 5kHz, 8-bit");
    codegen_emit_line(gen, "    ledcAttachPin(pin, _esp32_next_pwm_channel);");
    codegen_emit_line(gen, "    _esp32_next_pwm_channel++;");
    codegen_emit_line(gen, "  }");
    codegen_emit_line(gen, "  ledcWrite(_esp32_pwm_channels[pin] - 1, value);");
    codegen_emit_line(gen, "}\n");
    
    codegen_emit_line(gen, "/* ESP-NOW state */");
    codegen_emit_line(gen, "volatile float _esp_now_last_val = 0.0;");
    codegen_emit_line(gen, "volatile bool _esp_now_has_data = false;");
    codegen_emit_line(gen, "void _esp_now_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len) {");
    codegen_emit_line(gen, "  if (data_len == sizeof(float)) {");
    codegen_emit_line(gen, "    memcpy((void*)&_esp_now_last_val, data, sizeof(float));");
    codegen_emit_line(gen, "    _esp_now_has_data = true;");
    codegen_emit_line(gen, "  }");
    codegen_emit_line(gen, "}");
    codegen_emit_line(gen, "void radio_send_peer(int peer_id, float value) {");
    codegen_emit_line(gen, "  uint8_t mac_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};");
    codegen_emit_line(gen, "  esp_now_peer_info_t peerInfo = {};");
    codegen_emit_line(gen, "  memcpy(peerInfo.peer_addr, mac_addr, 6);");
    codegen_emit_line(gen, "  peerInfo.channel = 0;");
    codegen_emit_line(gen, "  peerInfo.encrypt = false;");
    codegen_emit_line(gen, "  if (!esp_now_is_peer_exist(mac_addr)) esp_now_add_peer(&peerInfo);");
    codegen_emit_line(gen, "  esp_now_send(mac_addr, (uint8_t*)&value, sizeof(float));");
    codegen_emit_line(gen, "}");
    codegen_emit_line(gen, "float radio_read() {");
    codegen_emit_line(gen, "  _esp_now_has_data = false;");
    codegen_emit_line(gen, "  return _esp_now_last_val;");
    codegen_emit_line(gen, "}");
    codegen_emit_line(gen, "bool radio_available() {");
    codegen_emit_line(gen, "  return _esp_now_has_data;");
    codegen_emit_line(gen, "}\n");

    // Hoist tasks as FreeRTOS functions
    if (program->data.program.main_block &&
        program->data.program.main_block->type == NODE_BLOCK) {
        ASTNode *block = program->data.program.main_block;
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *stmt = block->data.block.statements[i];
            if (stmt && stmt->type == NODE_TASK_DEF) {
                codegen_emit_line(gen, "void _task_func_%s(void *pvParameters) {\n", stmt->data.task_def.name);
                gen->indent_level++;
                codegen_emit_line(gen, "while (1) {\n");
                gen->indent_level++;
                gen->inside_task = 1;
                esp32_statement(gen, stmt->data.task_def.body);
                gen->inside_task = 0;
                codegen_emit_line(gen, "vTaskDelay(10 / portTICK_PERIOD_MS); // watchdog yield\n");
                gen->indent_level--;
                codegen_emit_line(gen, "}\n");
                gen->indent_level--;
                codegen_emit_line(gen, "}\n\n");
            }
        }
    }

    // Hoist function definitions
    if (program->data.program.main_block &&
        program->data.program.main_block->type == NODE_BLOCK) {
        ASTNode *block = program->data.program.main_block;
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *s = block->data.block.statements[i];
            if (s && s->type == NODE_FUNCTION_DEF) {
                if (s->data.function_def.is_extern) {
                    codegen_emit_line(gen, "extern %s %s(", 
                        s->data.function_def.return_type ? type_to_ctype(s->data.function_def.return_type) : "void",
                        s->data.function_def.name);
                    for (int j = 0; j < s->data.function_def.param_count; j++) {
                        if (j > 0) codegen_emit(gen, ", ");
                        const char *pty = "float";
                        if (s->data.function_def.param_types && s->data.function_def.param_types[j])
                            pty = type_to_ctype(s->data.function_def.param_types[j]);
                        codegen_emit(gen, "%s %s", pty, s->data.function_def.param_names[j]);
                    }
                    codegen_emit(gen, ");\n\n");
                    continue;
                }
                codegen_emit_line(gen, "%s %s(", 
                    s->data.function_def.return_type ? type_to_ctype(s->data.function_def.return_type) : "void",
                    s->data.function_def.name);
                for (int j = 0; j < s->data.function_def.param_count; j++) {
                    if (j > 0) codegen_emit(gen, ", ");
                    const char *pty = "float";
                    if (s->data.function_def.param_types && s->data.function_def.param_types[j])
                        pty = type_to_ctype(s->data.function_def.param_types[j]);
                    codegen_emit(gen, "%s %s", pty, s->data.function_def.param_names[j]);
                }
                codegen_emit(gen, ") {\n");
                gen->indent_level++;
                esp32_statement(gen, s->data.function_def.body);
                gen->indent_level--;
                codegen_emit_line(gen, "}\n\n");
            }
        }
    }

    // Hoist all deep ISRs recursively
    int timer_id_counter = 0;
    esp32_assign_timer_ids(program, &timer_id_counter);
    esp32_hoist_isrs(gen, program);

    // Hoist global variables
    if (program->data.program.main_block &&
        program->data.program.main_block->type == NODE_BLOCK) {
        ASTNode *block = program->data.program.main_block;
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *s = block->data.block.statements[i];
            if (s && (s->type == NODE_VAR_DECL || s->type == NODE_ARRAY_DECL || s->type == NODE_BUFFER_DECL)) {
                esp32_statement(gen, s);
            }
        }
    }

    // setup()
    codegen_emit_line(gen, "void setup() {");
    gen->indent_level++;
    codegen_emit_line(gen, "Serial.begin(115200);  // ESP32 default baud");
    codegen_emit_line(gen, "analogReadResolution(12);  // ESP32 12-bit ADC (0-4095)");
    codegen_emit_line(gen, "WiFi.mode(WIFI_STA);");
    codegen_emit_line(gen, "esp_now_init();");
    codegen_emit_line(gen, "esp_now_register_recv_cb(_esp_now_recv_cb);");
    if (program->data.program.pins_used) {
        for (int i = 0; i < program->data.program.pin_count; i++) {
            codegen_emit_line(gen, "pinMode(%d, OUTPUT);", program->data.program.pins_used[i]);
        }
    }
    
    if (program->data.program.in_pins_used) {
        for (int i = 0; i < program->data.program.in_pin_count; i++) {
            codegen_emit_line(gen, "pinMode(%d, INPUT);", program->data.program.in_pins_used[i]);
        }
    }
    
    /* Scan for OTA node and emit WiFi + ArduinoOTA setup */
    const char *ota_hostname = NULL;
    const char *ota_password = NULL;
    if (program->data.program.main_block &&
        program->data.program.main_block->type == NODE_BLOCK) {
        ASTNode *block = program->data.program.main_block;
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *s = block->data.block.statements[i];
            if (s && s->type == NODE_OTA_ENABLE) {
                ota_hostname = s->data.ota_enable.hostname;
                ota_password = s->data.ota_enable.password;
                break;
            }
        }
    }
    if (ota_hostname) {
        codegen_emit_line(gen, "  // OTA Fleet Update Setup");
        codegen_emit_line(gen, "  WiFi.mode(WIFI_STA);");
        codegen_emit_line(gen, "  WiFi.begin(); // Connect to saved WiFi or use WiFiManager");
        codegen_emit_line(gen, "  Serial.print(\"Connecting to WiFi\");");
        codegen_emit_line(gen, "  int _wifi_tries = 0;");
        codegen_emit_line(gen, "  while (WiFi.status() != WL_CONNECTED && _wifi_tries < 30) {");
        codegen_emit_line(gen, "    delay(500);");
        codegen_emit_line(gen, "    Serial.print(\".\");");
        codegen_emit_line(gen, "    _wifi_tries++;");
        codegen_emit_line(gen, "  }");
        codegen_emit_line(gen, "  Serial.println();");
        codegen_emit_line(gen, "  if (WiFi.status() == WL_CONNECTED) {");
        codegen_emit_line(gen, "    Serial.print(\"IP: \"); Serial.println(WiFi.localIP());");
        codegen_emit_line(gen, "  }");
        codegen_emit_line(gen, "  ArduinoOTA.setHostname(\"%s\");", ota_hostname);
        if (ota_password) {
            codegen_emit_line(gen, "  ArduinoOTA.setPassword(\"%s\");", ota_password);
        }
        codegen_emit_line(gen, "  ArduinoOTA.onStart([]() { Serial.println(\"OTA Update starting...\"); });");
        codegen_emit_line(gen, "  ArduinoOTA.onEnd([]() { Serial.println(\"OTA Update complete!\"); });");
        codegen_emit_line(gen, "  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {");
        codegen_emit_line(gen, "    Serial.printf(\"Progress: %%u%%%%\\r\", (progress / (total / 100)));");
        codegen_emit_line(gen, "  });");
        codegen_emit_line(gen, "  ArduinoOTA.onError([](ota_error_t error) { Serial.printf(\"OTA Error[%%u]\\n\", error); });");
        codegen_emit_line(gen, "  ArduinoOTA.begin();");
        codegen_emit_line(gen, "  Serial.println(\"OTA ready. Hostname: %s\");", ota_hostname);
    }
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");

    // loop()
    codegen_emit_line(gen, "void loop() {");
    gen->indent_level++;
    if (ota_hostname) {
        codegen_emit_line(gen, "ArduinoOTA.handle();  // Check for OTA updates");
    }
    gen->temp_var_counter = 0;
    if (program->data.program.main_block) {
        ASTNode *block = program->data.program.main_block;
        if (block->type == NODE_BLOCK) {
            for (int i = 0; i < block->data.block.statement_count; i++) {
                ASTNode *s = block->data.block.statements[i];
                if (s && s->type != NODE_FUNCTION_DEF && s->type != NODE_TASK_DEF &&
                    s->type != NODE_VAR_DECL && s->type != NODE_ARRAY_DECL && s->type != NODE_BUFFER_DECL &&
                    s->type != NODE_OTA_ENABLE) {
                    esp32_statement(gen, s);
                }
            }
        } else {
            esp32_statement(gen, block);
        }
    }
    gen->indent_level--;
    codegen_emit_line(gen, "}");
}
