/* Kinetrix AST Implementation
 * V3.0: Added byte/buffer/struct types + all V3.0 node constructors
 */

#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TYPE SYSTEM IMPLEMENTATION
// ============================================================================

Type *type_void() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_VOID;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  t->struct_name = NULL;
  return t;
}

Type *type_int() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_INT;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  t->struct_name = NULL;
  return t;
}

Type *type_float() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_FLOAT;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  t->struct_name = NULL;
  return t;
}

Type *type_bool() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_BOOL;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  t->struct_name = NULL;
  return t;
}

Type *type_byte() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_BYTE;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  t->struct_name = NULL;
  return t;
}

Type *type_string() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_STRING;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  t->struct_name = NULL;
  return t;
}

Type *type_buffer(Type *element_type, int size) {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_BUFFER;
  t->element_type = element_type;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = size;
  t->struct_name = NULL;
  return t;
}

Type *type_struct(const char *name) {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_STRUCT;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  t->struct_name = strdup(name);
  return t;
}

Type *type_array(Type *element_type, int size) {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_ARRAY;
  t->element_type = element_type;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = size;
  return t;
}

Type *type_function(Type *return_type, Type **param_types, int param_count) {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_FUNCTION;
  t->element_type = NULL;
  t->return_type = return_type;
  t->param_types = param_types;
  t->param_count = param_count;
  t->array_size = 0;
  return t;
}

Type *type_inferred() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_INFERRED;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  return t;
}

Type *type_error() {
  Type *t = malloc(sizeof(Type));
  t->kind = TYPE_ERROR;
  t->element_type = NULL;
  t->return_type = NULL;
  t->param_types = NULL;
  t->param_count = 0;
  t->array_size = 0;
  return t;
}

int type_equals(Type *a, Type *b) {
  if (a == NULL || b == NULL)
    return 0;
  if (a->kind != b->kind)
    return 0;

  switch (a->kind) {
  case TYPE_ARRAY:
    return type_equals(a->element_type, b->element_type);
  case TYPE_FUNCTION:
    if (!type_equals(a->return_type, b->return_type))
      return 0;
    if (a->param_count != b->param_count)
      return 0;
    for (int i = 0; i < a->param_count; i++) {
      if (!type_equals(a->param_types[i], b->param_types[i]))
        return 0;
    }
    return 1;
  default:
    return 1;
  }
}

const char *type_to_string(Type *t) {
  if (t == NULL)
    return "null";
  static char buffer[256];
  switch (t->kind) {
  case TYPE_VOID:
    return "void";
  case TYPE_INT:
    return "int";
  case TYPE_FLOAT:
    return "float";
  case TYPE_BOOL:
    return "bool";
  case TYPE_BYTE:
    return "byte";
  case TYPE_STRING:
    return "string";
  case TYPE_ARRAY:
    snprintf(buffer, sizeof(buffer), "%s[%d]", type_to_string(t->element_type),
             t->array_size);
    return buffer;
  case TYPE_BUFFER:
    snprintf(buffer, sizeof(buffer), "buffer<%s>[%d]",
             type_to_string(t->element_type), t->array_size);
    return buffer;
  case TYPE_STRUCT:
    snprintf(buffer, sizeof(buffer), "struct %s",
             t->struct_name ? t->struct_name : "?");
    return buffer;
  case TYPE_FUNCTION:
    return "function";
  case TYPE_INFERRED:
    return "inferred";
  case TYPE_ERROR:
    return "error";
  default:
    return "unknown";
  }
}

const char *type_to_ctype(Type *t) {
  if (t == NULL)
    return "float";
  switch (t->kind) {
  case TYPE_INT:
    return "int";
  case TYPE_FLOAT:
    return "float";
  case TYPE_BOOL:
    return "bool";
  case TYPE_BYTE:
    return "uint8_t";
  case TYPE_STRING:
    return "const char*";
  case TYPE_VOID:
    return "void";
  case TYPE_STRUCT:
    return t->struct_name ? t->struct_name : "struct";
  default:
    return "float";
  }
}

Type *type_clone(Type *t) {
  if (t == NULL)
    return NULL;

  Type *clone = malloc(sizeof(Type));
  clone->kind = t->kind;
  clone->element_type = type_clone(t->element_type);
  clone->return_type = type_clone(t->return_type);
  clone->array_size = t->array_size;
  clone->param_count = t->param_count;

  if (t->param_types != NULL) {
    clone->param_types = malloc(sizeof(Type *) * t->param_count);
    for (int i = 0; i < t->param_count; i++) {
      clone->param_types[i] = type_clone(t->param_types[i]);
    }
  } else {
    clone->param_types = NULL;
  }

  return clone;
}

void type_free(Type *t) {
  if (t == NULL)
    return;

  type_free(t->element_type);
  type_free(t->return_type);

  if (t->param_types != NULL) {
    for (int i = 0; i < t->param_count; i++) {
      type_free(t->param_types[i]);
    }
    free(t->param_types);
  }

  free(t);
}

// ============================================================================
// AST NODE CONSTRUCTORS
// ============================================================================

static ASTNode *ast_create(NodeType type) {
  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = type;
  node->value_type = type_inferred();
  node->line = 0;
  return node;
}

ASTNode *ast_number(double value) {
  ASTNode *node = ast_create(NODE_NUMBER);
  node->data.number.value = value;
  // Infer type: if it's a whole number, it's int, otherwise float
  if (value == (int)value) {
    type_free(node->value_type);
    node->value_type = type_int();
  } else {
    type_free(node->value_type);
    node->value_type = type_float();
  }
  return node;
}

ASTNode *ast_string(const char *value) {
  ASTNode *node = ast_create(NODE_STRING);
  node->data.string.value = strdup(value);
  type_free(node->value_type);
  node->value_type = type_string();
  return node;
}

ASTNode *ast_bool(int value) {
  ASTNode *node = ast_create(NODE_BOOL);
  node->data.boolean.value = value;
  type_free(node->value_type);
  node->value_type = type_bool();
  return node;
}

ASTNode *ast_identifier(const char *name) {
  ASTNode *node = ast_create(NODE_IDENTIFIER);
  node->data.identifier.name = strdup(name);
  return node;
}

ASTNode *ast_binary_op(Operator op, ASTNode *left, ASTNode *right) {
  ASTNode *node = ast_create(NODE_BINARY_OP);
  node->data.binary_op.op = op;
  node->data.binary_op.left = left;
  node->data.binary_op.right = right;

  // Infer string concatenation
  if (op == OP_ADD &&
      ((left && left->value_type && left->value_type->kind == TYPE_STRING) ||
       (right && right->value_type &&
        right->value_type->kind == TYPE_STRING))) {
    type_free(node->value_type);
    node->value_type = type_string();
  }

  return node;
}

ASTNode *ast_unary_op(Operator op, ASTNode *operand) {
  ASTNode *node = ast_create(NODE_UNARY_OP);
  node->data.unary_op.op = op;
  node->data.unary_op.operand = operand;
  return node;
}

ASTNode *ast_call(const char *name, ASTNode **args, int arg_count) {
  ASTNode *node = ast_create(NODE_CALL);
  node->data.call.name = strdup(name);
  node->data.call.args = args;
  node->data.call.arg_count = arg_count;
  return node;
}

ASTNode *ast_array_access(ASTNode *array, ASTNode *index) {
  ASTNode *node = ast_create(NODE_ARRAY_ACCESS);
  node->data.array_access.array = array;
  node->data.array_access.index = index;
  return node;
}

ASTNode *ast_array_literal(ASTNode **elements, int element_count) {
  ASTNode *node = ast_create(NODE_ARRAY_LITERAL);
  node->data.array_literal.elements = elements;
  node->data.array_literal.element_count = element_count;
  return node;
}

ASTNode *ast_var_decl(const char *name, Type *type, ASTNode *initializer) {
  ASTNode *node = ast_create(NODE_VAR_DECL);
  node->data.var_decl.name = strdup(name);
  node->data.var_decl.declared_type = type;
  node->data.var_decl.initializer = initializer;
  node->data.var_decl.is_array = 0;
  node->data.var_decl.array_size = 0;
  return node;
}

/* Legacy helper kept as compat — creates a NODE_VAR_DECL with is_array=1.
 * Used internally by old code paths. New V3.0 code uses ast_array_decl()
 * which creates NODE_ARRAY_DECL instead. */
ASTNode *ast_array_decl_compat(const char *name, Type *element_type, int size) {
  ASTNode *node = ast_create(NODE_VAR_DECL);
  node->data.var_decl.name = strdup(name);
  node->data.var_decl.declared_type = type_array(element_type, size);
  node->data.var_decl.initializer = NULL;
  node->data.var_decl.is_array = 1;
  node->data.var_decl.array_size = size;
  return node;
}

ASTNode *ast_assignment(ASTNode *target, ASTNode *value) {
  ASTNode *node = ast_create(NODE_ASSIGNMENT);
  node->data.assignment.target = target;
  node->data.assignment.value = value;
  return node;
}

ASTNode *ast_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block) {
  ASTNode *node = ast_create(NODE_IF);
  node->data.if_stmt.condition = condition;
  node->data.if_stmt.then_block = then_block;
  node->data.if_stmt.else_block = else_block;
  return node;
}

ASTNode *ast_while(ASTNode *condition, ASTNode *body) {
  ASTNode *node = ast_create(NODE_WHILE);
  node->data.while_loop.condition = condition;
  node->data.while_loop.body = body;
  return node;
}

ASTNode *ast_repeat(ASTNode *count, ASTNode *body) {
  ASTNode *node = ast_create(NODE_REPEAT);
  node->data.repeat_loop.count = count;
  node->data.repeat_loop.body = body;
  return node;
}

ASTNode *ast_forever(ASTNode *body) {
  ASTNode *node = ast_create(NODE_FOREVER);
  node->data.forever_loop.body = body;
  return node;
}

ASTNode *ast_for(const char *var_name, ASTNode *start_expr, ASTNode *end_expr,
                 ASTNode *step_expr, ASTNode *body) {
  ASTNode *node = ast_create(NODE_FOR);
  node->data.for_loop.var_name = strdup(var_name);
  node->data.for_loop.start_expr = start_expr;
  node->data.for_loop.end_expr = end_expr;
  node->data.for_loop.step_expr = step_expr;
  node->data.for_loop.body = body;
  return node;
}

ASTNode *ast_block(ASTNode **statements, int statement_count) {
  ASTNode *node = ast_create(NODE_BLOCK);
  node->data.block.statements = statements;
  node->data.block.statement_count = statement_count;
  return node;
}

ASTNode *ast_return(ASTNode *value) {
  ASTNode *node = ast_create(NODE_RETURN);
  node->data.return_stmt.value = value;
  return node;
}

ASTNode *ast_break() { return ast_create(NODE_BREAK); }

ASTNode *ast_continue() { return ast_create(NODE_CONTINUE); }

// Robotics-specific constructors
ASTNode *ast_gpio_write(ASTNode *pin, ASTNode *value) {
  ASTNode *node = ast_create(NODE_GPIO_WRITE);
  node->data.gpio.pin = pin;
  node->data.gpio.value = value;
  return node;
}

ASTNode *ast_gpio_read(ASTNode *pin) {
  ASTNode *node = ast_create(NODE_GPIO_READ);
  node->data.gpio.pin = pin;
  node->data.gpio.value = NULL;
  return node;
}

ASTNode *ast_analog_read(ASTNode *pin) {
  ASTNode *node = ast_create(NODE_ANALOG_READ);
  node->data.gpio.pin = pin;
  node->data.gpio.value = NULL;
  return node;
}

ASTNode *ast_analog_write(ASTNode *pin, ASTNode *value) {
  ASTNode *node = ast_create(NODE_ANALOG_WRITE);
  node->data.gpio.pin = pin;
  node->data.gpio.value = value;
  return node;
}

ASTNode *ast_pulse_read(ASTNode *pin, ASTNode *timeout) {
  ASTNode *node = ast_create(NODE_PULSE_READ);
  node->data.gpio.pin = pin;
  node->data.gpio.value = timeout;
  return node;
}

ASTNode *ast_servo_write(ASTNode *pin, ASTNode *angle) {
  ASTNode *node = ast_create(NODE_SERVO_WRITE);
  node->data.gpio.pin = pin;
  node->data.gpio.value = angle;
  return node;
}

ASTNode *ast_i2c_begin() { return ast_create(NODE_I2C_BEGIN); }

ASTNode *ast_i2c_start(ASTNode *address) {
  ASTNode *node = ast_create(NODE_I2C_START);
  node->data.i2c.address = address;
  node->data.i2c.data = NULL;
  return node;
}

ASTNode *ast_i2c_send(ASTNode *data) {
  ASTNode *node = ast_create(NODE_I2C_SEND);
  node->data.i2c.address = NULL;
  node->data.i2c.data = data;
  return node;
}

ASTNode *ast_i2c_stop() { return ast_create(NODE_I2C_STOP); }

ASTNode *ast_i2c_read(ASTNode *address) {
  ASTNode *node = ast_create(NODE_I2C_READ);
  node->data.i2c.address = address;
  node->data.i2c.data = NULL;
  return node;
}

ASTNode *ast_tone(ASTNode *pin, ASTNode *frequency) {
  ASTNode *node = ast_create(NODE_TONE);
  node->data.gpio.pin = pin;
  node->data.gpio.value = frequency;
  return node;
}

ASTNode *ast_notone(ASTNode *pin) {
  ASTNode *node = ast_create(NODE_NOTONE);
  node->data.gpio.pin = pin;
  node->data.gpio.value = NULL;
  return node;
}

ASTNode *ast_wait(ASTNode *duration) {
  ASTNode *node = ast_create(NODE_WAIT);
  node->data.unary.child = duration;
  return node;
}

ASTNode *ast_print_stmt(ASTNode *value) {
  ASTNode *node = ast_create(NODE_PRINT);
  node->data.unary.child = value;
  return node;
}

ASTNode *ast_println_stmt(ASTNode *value) {
  ASTNode *node = ast_create(NODE_PRINTLN);
  node->data.unary.child = value;
  return node;
}

ASTNode *ast_math_func(MathFunc func, ASTNode *arg1, ASTNode *arg2) {
  ASTNode *node = ast_create(NODE_MATH_FUNC);
  node->data.math_func.func = func;
  node->data.math_func.arg1 = arg1;
  node->data.math_func.arg2 = arg2;
  type_free(node->value_type);
  node->value_type = type_float();
  return node;
}

ASTNode *ast_function_def(const char *name, char **param_names,
                          Type **param_types, int param_count,
                          Type *return_type, ASTNode *body) {
  ASTNode *node = ast_create(NODE_FUNCTION_DEF);
  node->data.function_def.name = strdup(name);
  node->data.function_def.param_names = param_names;
  node->data.function_def.param_types = param_types;
  node->data.function_def.param_count = param_count;
  node->data.function_def.return_type = return_type;
  node->data.function_def.body = body;
  node->data.function_def.is_extern = 0;
  node->data.function_def.extern_lang = NULL;
  return node;
}

ASTNode *ast_extern_function_def(const char *name, char **param_names,
                                 Type **param_types, int param_count,
                                 Type *return_type, const char *extern_lang) {
  ASTNode *node = ast_create(NODE_FUNCTION_DEF);
  node->data.function_def.name = strdup(name);
  node->data.function_def.param_names = param_names;
  node->data.function_def.param_types = param_types;
  node->data.function_def.param_count = param_count;
  node->data.function_def.return_type = return_type;
  node->data.function_def.body = NULL;
  node->data.function_def.is_extern = 1;
  node->data.function_def.extern_lang = strdup(extern_lang);
  return node;
}

ASTNode *ast_program(ASTNode **functions, int function_count,
                     ASTNode *main_block) {
  ASTNode *node = ast_create(NODE_PROGRAM);
  node->data.program.functions = functions;
  node->data.program.function_count = function_count;
  node->data.program.main_block = main_block;
  node->data.program.pins_used = NULL;
  node->data.program.pin_count = 0;
  return node;
}

// ============================================================================
// V3.0 AST NODE CONSTRUCTORS
// ============================================================================

ASTNode *ast_cast(Type *target_type, ASTNode *operand) {
  ASTNode *node = ast_create(NODE_CAST);
  node->data.cast_op.target_type = target_type;
  node->data.cast_op.operand = operand;
  node->value_type = type_clone(target_type);
  return node;
}

ASTNode *ast_array_decl(const char *name, Type *elem_type, int size) {
  ASTNode *node = ast_create(NODE_ARRAY_DECL);
  node->data.array_decl.name = strdup(name);
  node->data.array_decl.elem_type = elem_type;
  node->data.array_decl.size = size;
  return node;
}

/* Keep the old name as an alias for backward compat */
ASTNode *ast_array_decl_old(const char *name, Type *element_type, int size) {
  return ast_array_decl(name, element_type, size);
}

ASTNode *ast_buffer_decl(const char *name, Type *elem_type, int size) {
  ASTNode *node = ast_create(NODE_BUFFER_DECL);
  node->data.array_decl.name = strdup(name);
  node->data.array_decl.elem_type = elem_type;
  node->data.array_decl.size = size;
  return node;
}

ASTNode *ast_buffer_push(const char *buffer_name, ASTNode *value) {
  ASTNode *node = ast_create(NODE_BUFFER_PUSH);
  node->data.buffer_push.buffer_name = strdup(buffer_name);
  node->data.buffer_push.value = value;
  return node;
}

ASTNode *ast_struct_access(ASTNode *object, const char *member) {
  ASTNode *node = ast_create(NODE_STRUCT_ACCESS);
  node->data.struct_access.object = object;
  node->data.struct_access.member = strdup(member);
  return node;
}

ASTNode *ast_var_decl_ex(const char *name, Type *type, ASTNode *initializer,
                         int is_shared) {
  ASTNode *node = ast_var_decl(name, type, initializer);
  node->data.var_decl.is_shared = is_shared;
  return node;
}

ASTNode *ast_var_decl_const(const char *name, Type *type,
                            ASTNode *initializer) {
  ASTNode *node = ast_var_decl(name, type, initializer);
  node->data.var_decl.is_const = 1;
  return node;
}

/* --- Interrupts --- */

ASTNode *ast_interrupt_pin(int pin_number, InterruptMode mode, ASTNode *body) {
  ASTNode *node = ast_create(NODE_INTERRUPT_PIN);
  node->data.interrupt_pin.pin_number = pin_number;
  node->data.interrupt_pin.mode = mode;
  node->data.interrupt_pin.body = body;
  return node;
}

ASTNode *ast_interrupt_timer(int interval, int is_us, ASTNode *body) {
  ASTNode *node = ast_create(NODE_INTERRUPT_TIMER);
  node->data.interrupt_timer.interval = interval;
  node->data.interrupt_timer.is_us = is_us;
  node->data.interrupt_timer.body = body;
  node->data.interrupt_timer.timer_id = 0; /* Updated in codegen pass */
  return node;
}

/* --- UART --- */

ASTNode *ast_serial_open(int baud_rate) {
  ASTNode *node = ast_create(NODE_SERIAL_OPEN);
  node->data.serial_open.baud_rate = baud_rate;
  return node;
}

ASTNode *ast_serial_send(ASTNode *value) {
  ASTNode *node = ast_create(NODE_SERIAL_SEND);
  node->data.serial_send.value = value;
  return node;
}

ASTNode *ast_serial_recv() { return ast_create(NODE_SERIAL_RECV); }

/* --- I2C high-level --- */

ASTNode *ast_i2c_open() { return ast_create(NODE_I2C_OPEN); }

ASTNode *ast_i2c_device_read(ASTNode *device_addr, ASTNode *reg_addr) {
  ASTNode *node = ast_create(NODE_I2C_DEVICE_READ);
  node->data.i2c_device_read.device_addr = device_addr;
  node->data.i2c_device_read.reg_addr = reg_addr;
  return node;
}

ASTNode *ast_i2c_device_read_array(ASTNode *device_addr, ASTNode *reg_addr,
                                   ASTNode *count, const char *array_name) {
  ASTNode *node = ast_create(NODE_I2C_DEVICE_READ_ARRAY);
  node->data.i2c_device_read_array.device_addr = device_addr;
  node->data.i2c_device_read_array.reg_addr = reg_addr;
  node->data.i2c_device_read_array.count = count;
  node->data.i2c_device_read_array.array_name = strdup(array_name);
  return node;
}

ASTNode *ast_i2c_device_write(ASTNode *device_addr, ASTNode *value) {
  ASTNode *node = ast_create(NODE_I2C_DEVICE_WRITE);
  node->data.i2c_device_write.device_addr = device_addr;
  node->data.i2c_device_write.value = value;
  return node;
}

/* --- SPI --- */

ASTNode *ast_spi_open(int frequency) {
  ASTNode *node = ast_create(NODE_SPI_OPEN);
  node->data.spi_open.frequency = frequency;
  return node;
}

ASTNode *ast_spi_transfer(ASTNode *data) {
  ASTNode *node = ast_create(NODE_SPI_TRANSFER);
  node->data.spi_transfer.data = data;
  return node;
}

/* --- Named devices --- */

ASTNode *ast_device_def(const char *name, ProtocolType protocol,
                        ASTNode *addr) {
  ASTNode *node = ast_create(NODE_DEVICE_DEF);
  node->data.device_def.device_name = strdup(name);
  node->data.device_def.protocol = protocol;
  node->data.device_def.address_or_baud = addr;
  return node;
}

ASTNode *ast_device_read(const char *device_name, ProtocolType protocol,
                         ASTNode *reg) {
  ASTNode *node = ast_create(NODE_DEVICE_READ);
  node->data.device_read.device_name = strdup(device_name);
  node->data.device_read.protocol = protocol;
  node->data.device_read.reg = reg;
  return node;
}

ASTNode *ast_device_write(const char *device_name, ProtocolType protocol,
                          ASTNode *value) {
  ASTNode *node = ast_create(NODE_DEVICE_WRITE);
  node->data.device_write.device_name = strdup(device_name);
  node->data.device_write.protocol = protocol;
  node->data.device_write.value = value;
  return node;
}

/* --- Error handling --- */

ASTNode *ast_try(ASTNode *try_block, ASTNode *error_block) {
  ASTNode *node = ast_create(NODE_TRY);
  node->data.try_stmt.try_block = try_block;
  node->data.try_stmt.error_block = error_block;
  return node;
}

ASTNode *ast_watchdog_enable(int timeout_ms) {
  ASTNode *node = ast_create(NODE_WATCHDOG_ENABLE);
  node->data.watchdog_enable.timeout_ms = timeout_ms;
  return node;
}

ASTNode *ast_watchdog_feed() { return ast_create(NODE_WATCHDOG_FEED); }

ASTNode *ast_ota_enable(const char *hostname, const char *password) {
  ASTNode *node = ast_create(NODE_OTA_ENABLE);
  node->data.ota_enable.hostname =
      hostname ? strdup(hostname) : strdup("kinetrix");
  node->data.ota_enable.password = password ? strdup(password) : NULL;
  return node;
}

ASTNode *ast_disable_interrupts() {
  return ast_create(NODE_DISABLE_INTERRUPTS);
}

ASTNode *ast_enable_interrupts() { return ast_create(NODE_ENABLE_INTERRUPTS); }

ASTNode *ast_assert(ASTNode *condition, ASTNode *action) {
  ASTNode *node = ast_create(NODE_ASSERT);
  node->data.assert_stmt.condition = condition;
  node->data.assert_stmt.action = action;
  return node;
}

/* --- Structs --- */

ASTNode *ast_struct_def(const char *name, StructField *fields,
                        int field_count) {
  ASTNode *node = ast_create(NODE_STRUCT_DEF);
  node->data.struct_def.name = strdup(name);
  node->data.struct_def.fields = fields;
  node->data.struct_def.field_count = field_count;
  return node;
}

ASTNode *ast_struct_instance(const char *struct_type, const char *var_name) {
  ASTNode *node = ast_create(NODE_STRUCT_INSTANCE);
  node->data.struct_instance.struct_type = strdup(struct_type);
  node->data.struct_instance.var_name = strdup(var_name);
  return node;
}

/* --- Concurrency --- */

ASTNode *ast_task_def(const char *name, ASTNode *body) {
  ASTNode *node = ast_create(NODE_TASK_DEF);
  node->data.task_def.name = strdup(name);
  node->data.task_def.body = body;
  return node;
}

ASTNode *ast_task_start(const char *task_name) {
  ASTNode *node = ast_create(NODE_TASK_START);
  node->data.task_start.task_name = strdup(task_name);
  return node;
}

// ============================================================================
// AST UTILITIES
// ============================================================================

void ast_free(ASTNode *node) {
  if (node == NULL)
    return;

  type_free(node->value_type);

  switch (node->type) {
  case NODE_STRING:
    free(node->data.string.value);
    break;
  case NODE_IDENTIFIER:
    free(node->data.identifier.name);
    break;
  case NODE_BINARY_OP:
    ast_free(node->data.binary_op.left);
    ast_free(node->data.binary_op.right);
    break;
  case NODE_UNARY_OP:
    ast_free(node->data.unary_op.operand);
    break;
  case NODE_CALL:
    free(node->data.call.name);
    for (int i = 0; i < node->data.call.arg_count; i++) {
      ast_free(node->data.call.args[i]);
    }
    free(node->data.call.args);
    break;
  case NODE_ARRAY_ACCESS:
    ast_free(node->data.array_access.array);
    ast_free(node->data.array_access.index);
    break;
  case NODE_ARRAY_LITERAL:
    for (int i = 0; i < node->data.array_literal.element_count; i++) {
      ast_free(node->data.array_literal.elements[i]);
    }
    free(node->data.array_literal.elements);
    break;
  case NODE_VAR_DECL:
    free(node->data.var_decl.name);
    type_free(node->data.var_decl.declared_type);
    ast_free(node->data.var_decl.initializer);
    break;
  case NODE_ASSIGNMENT:
    ast_free(node->data.assignment.target);
    ast_free(node->data.assignment.value);
    break;
  case NODE_IF:
    ast_free(node->data.if_stmt.condition);
    ast_free(node->data.if_stmt.then_block);
    ast_free(node->data.if_stmt.else_block);
    break;
  case NODE_WHILE:
    ast_free(node->data.while_loop.condition);
    ast_free(node->data.while_loop.body);
    break;
  case NODE_REPEAT:
    ast_free(node->data.repeat_loop.count);
    ast_free(node->data.repeat_loop.body);
    break;
  case NODE_FOREVER:
    ast_free(node->data.forever_loop.body);
    break;
  case NODE_BLOCK:
    for (int i = 0; i < node->data.block.statement_count; i++) {
      ast_free(node->data.block.statements[i]);
    }
    free(node->data.block.statements);
    break;
  case NODE_RETURN:
    ast_free(node->data.return_stmt.value);
    break;
  case NODE_FUNCTION_DEF:
    free(node->data.function_def.name);
    for (int i = 0; i < node->data.function_def.param_count; i++) {
      free(node->data.function_def.param_names[i]);
      type_free(node->data.function_def.param_types[i]);
    }
    free(node->data.function_def.param_names);
    free(node->data.function_def.param_types);
    type_free(node->data.function_def.return_type);
    ast_free(node->data.function_def.body);
    break;
  case NODE_PROGRAM:
    for (int i = 0; i < node->data.program.function_count; i++) {
      ast_free(node->data.program.functions[i]);
    }
    free(node->data.program.functions);
    ast_free(node->data.program.main_block);
    break;
  // Handle other node types with children
  default:
    // Many nodes use the unary or gpio structure
    if (node->type == NODE_WAIT || node->type == NODE_PRINT) {
      ast_free(node->data.unary.child);
    } else if (node->type >= NODE_GPIO_WRITE && node->type <= NODE_NOTONE) {
      ast_free(node->data.gpio.pin);
      ast_free(node->data.gpio.value);
    } else if (node->type >= NODE_I2C_BEGIN && node->type <= NODE_I2C_READ) {
      ast_free(node->data.i2c.address);
      ast_free(node->data.i2c.data);
    } else if (node->type == NODE_MATH_FUNC) {
      ast_free(node->data.math_func.arg1);
      ast_free(node->data.math_func.arg2);
    }
    break;
  }

  free(node);
}

void ast_print(ASTNode *node, int indent) {
  if (node == NULL)
    return;

  for (int i = 0; i < indent; i++)
    printf("  ");

  switch (node->type) {
  case NODE_NUMBER:
    printf("NUMBER: %g\n", node->data.number.value);
    break;
  case NODE_STRING:
    printf("STRING: \"%s\"\n", node->data.string.value);
    break;
  case NODE_BOOL:
    printf("BOOL: %s\n", node->data.boolean.value ? "true" : "false");
    break;
  case NODE_IDENTIFIER:
    printf("ID: %s\n", node->data.identifier.name);
    break;
  case NODE_BINARY_OP:
    printf("BINARY_OP\n");
    ast_print(node->data.binary_op.left, indent + 1);
    ast_print(node->data.binary_op.right, indent + 1);
    break;
  case NODE_CALL:
    printf("CALL: %s\n", node->data.call.name);
    for (int i = 0; i < node->data.call.arg_count; i++) {
      ast_print(node->data.call.args[i], indent + 1);
    }
    break;
  case NODE_VAR_DECL:
    printf("VAR_DECL: %s : %s\n", node->data.var_decl.name,
           type_to_string(node->data.var_decl.declared_type));
    if (node->data.var_decl.initializer) {
      ast_print(node->data.var_decl.initializer, indent + 1);
    }
    break;
  case NODE_BLOCK:
    printf("BLOCK\n");
    for (int i = 0; i < node->data.block.statement_count; i++) {
      ast_print(node->data.block.statements[i], indent + 1);
    }
    break;
  case NODE_PROGRAM:
    printf("PROGRAM\n");
    for (int i = 0; i < node->data.program.function_count; i++) {
      ast_print(node->data.program.functions[i], indent + 1);
    }
    ast_print(node->data.program.main_block, indent + 1);
    break;
  default:
    printf("NODE_TYPE: %d\n", node->type);
    break;
  }
}

/* --- Radio APIs --- */
ASTNode *ast_radio_send(ASTNode *peer_id, ASTNode *data) {
  ASTNode *node = ast_create(NODE_RADIO_SEND);
  node->data.radio_send.peer_id = peer_id;
  node->data.radio_send.data = data;
  return node;
}

ASTNode *ast_radio_available() { return ast_create(NODE_RADIO_AVAILABLE); }

ASTNode *ast_radio_read() { return ast_create(NODE_RADIO_READ); }

/* --- Library Wrapper APIs --- */

ASTNode *ast_servo_attach(ASTNode *pin) {
  ASTNode *node = ast_create(NODE_SERVO_ATTACH);
  node->data.servo_attach.pin = pin;
  return node;
}

ASTNode *ast_servo_move(ASTNode *angle) {
  ASTNode *node = ast_create(NODE_SERVO_MOVE);
  node->data.servo_write.angle = angle;
  return node;
}

ASTNode *ast_servo_detach(ASTNode *pin) {
  ASTNode *node = ast_create(NODE_SERVO_DETACH);
  node->data.servo_detach.pin = pin;
  return node;
}

ASTNode *ast_distance_read(ASTNode *trigger_pin, ASTNode *echo_pin) {
  ASTNode *node = ast_create(NODE_DISTANCE_READ);
  node->data.distance_read.trigger_pin = trigger_pin;
  node->data.distance_read.echo_pin = echo_pin;
  return node;
}

ASTNode *ast_dht_attach(ASTNode *pin, int dht_type) {
  ASTNode *node = ast_create(NODE_DHT_ATTACH);
  node->data.dht_attach.pin = pin;
  node->data.dht_attach.dht_type = dht_type;
  return node;
}

ASTNode *ast_dht_read_temp() { return ast_create(NODE_DHT_READ_TEMP); }

ASTNode *ast_dht_read_humid() { return ast_create(NODE_DHT_READ_HUMID); }

ASTNode *ast_neopixel_init(ASTNode *pin, ASTNode *count) {
  ASTNode *node = ast_create(NODE_NEOPIXEL_INIT);
  node->data.neopixel_init.pin = pin;
  node->data.neopixel_init.count = count;
  return node;
}

ASTNode *ast_neopixel_set(ASTNode *index, ASTNode *r, ASTNode *g, ASTNode *b) {
  ASTNode *node = ast_create(NODE_NEOPIXEL_SET);
  node->data.neopixel_set.index = index;
  node->data.neopixel_set.r = r;
  node->data.neopixel_set.g = g;
  node->data.neopixel_set.b = b;
  return node;
}

ASTNode *ast_neopixel_show() { return ast_create(NODE_NEOPIXEL_SHOW); }

ASTNode *ast_neopixel_clear() { return ast_create(NODE_NEOPIXEL_CLEAR); }

ASTNode *ast_lcd_init(ASTNode *cols, ASTNode *rows) {
  ASTNode *node = ast_create(NODE_LCD_INIT);
  node->data.lcd_init.cols = cols;
  node->data.lcd_init.rows = rows;
  return node;
}

ASTNode *ast_lcd_print(ASTNode *text, ASTNode *line) {
  ASTNode *node = ast_create(NODE_LCD_PRINT);
  node->data.lcd_print.text = text;
  node->data.lcd_print.line = line;
  return node;
}

ASTNode *ast_lcd_clear() { return ast_create(NODE_LCD_CLEAR); }

/* --- Library Wrapper APIs (Wave 2 - Motion & Motor) --- */

ASTNode *ast_stepper_attach(ASTNode *step_pin, ASTNode *dir_pin) {
  ASTNode *node = ast_create(NODE_STEPPER_ATTACH);
  node->data.stepper_attach.step_pin = step_pin;
  node->data.stepper_attach.dir_pin = dir_pin;
  return node;
}

ASTNode *ast_stepper_speed(ASTNode *speed) {
  ASTNode *node = ast_create(NODE_STEPPER_SPEED);
  node->data.unary.child = speed;
  return node;
}

ASTNode *ast_stepper_move(ASTNode *steps) {
  ASTNode *node = ast_create(NODE_STEPPER_MOVE);
  node->data.stepper_move.steps = steps;
  return node;
}

ASTNode *ast_motor_attach(ASTNode *en_pin, ASTNode *fwd_pin, ASTNode *rev_pin) {
  ASTNode *node = ast_create(NODE_MOTOR_ATTACH);
  node->data.motor_attach.en_pin = en_pin;
  node->data.motor_attach.fwd_pin = fwd_pin;
  node->data.motor_attach.rev_pin = rev_pin;
  return node;
}

ASTNode *ast_motor_move(int direction, ASTNode *speed) {
  ASTNode *node = ast_create(NODE_MOTOR_MOVE);
  node->data.motor_move.direction = direction;
  node->data.motor_move.speed = speed;
  return node;
}

ASTNode *ast_motor_stop() { return ast_create(NODE_MOTOR_STOP); }

ASTNode *ast_encoder_attach(ASTNode *pin_a, ASTNode *pin_b) {
  ASTNode *node = ast_create(NODE_ENCODER_ATTACH);
  node->data.encoder_attach.pin_a = pin_a;
  node->data.encoder_attach.pin_b = pin_b;
  return node;
}

ASTNode *ast_encoder_read() { return ast_create(NODE_ENCODER_READ); }

ASTNode *ast_encoder_reset() { return ast_create(NODE_ENCODER_RESET); }

ASTNode *ast_esc_attach(ASTNode *pin) {
  ASTNode *node = ast_create(NODE_ESC_ATTACH);
  node->data.esc_attach.pin = pin;
  return node;
}

ASTNode *ast_esc_throttle(ASTNode *throttle) {
  ASTNode *node = ast_create(NODE_ESC_THROTTLE);
  node->data.unary.child = throttle;
  return node;
}

ASTNode *ast_pid_attach(ASTNode *kp, ASTNode *ki, ASTNode *kd) {
  ASTNode *node = ast_create(NODE_PID_ATTACH);
  node->data.pid_attach.kp = kp;
  node->data.pid_attach.ki = ki;
  node->data.pid_attach.kd = kd;
  return node;
}

ASTNode *ast_pid_target(ASTNode *target) {
  ASTNode *node = ast_create(NODE_PID_TARGET);
  node->data.unary.child = target;
  return node;
}

ASTNode *ast_pid_compute(ASTNode *current) {
  ASTNode *node = ast_create(NODE_PID_COMPUTE);
  node->data.pid_compute.current_val = current;
  return node;
}

/* ============================================================
 * Wave 3: Communication & Networking
 * ============================================================ */

ASTNode *ast_ble_enable(ASTNode *name) {
  ASTNode *node = ast_create(NODE_BLE_ENABLE);
  node->data.ble_enable.name = name;
  return node;
}

ASTNode *ast_ble_advertise(ASTNode *data) {
  ASTNode *node = ast_create(NODE_BLE_ADVERTISE);
  node->data.ble_advertise.data = data;
  return node;
}

ASTNode *ast_ble_send(ASTNode *data) {
  ASTNode *node = ast_create(NODE_BLE_SEND);
  node->data.ble_send.data = data;
  return node;
}

ASTNode *ast_ble_receive(void) { return ast_create(NODE_BLE_RECEIVE); }

ASTNode *ast_wifi_connect(ASTNode *ssid, ASTNode *password) {
  ASTNode *node = ast_create(NODE_WIFI_CONNECT);
  node->data.wifi_connect.ssid = ssid;
  node->data.wifi_connect.password = password;
  return node;
}

ASTNode *ast_wifi_ip(void) { return ast_create(NODE_WIFI_IP); }

ASTNode *ast_mqtt_connect(ASTNode *broker, ASTNode *port) {
  ASTNode *node = ast_create(NODE_MQTT_CONNECT);
  node->data.mqtt_connect.broker = broker;
  node->data.mqtt_connect.port = port;
  return node;
}

ASTNode *ast_mqtt_subscribe(ASTNode *topic) {
  ASTNode *node = ast_create(NODE_MQTT_SUBSCRIBE);
  node->data.mqtt_subscribe.topic = topic;
  return node;
}

ASTNode *ast_mqtt_publish(ASTNode *topic, ASTNode *payload) {
  ASTNode *node = ast_create(NODE_MQTT_PUBLISH);
  node->data.mqtt_publish.topic = topic;
  node->data.mqtt_publish.payload = payload;
  return node;
}

ASTNode *ast_mqtt_read(void) { return ast_create(NODE_MQTT_READ); }

ASTNode *ast_http_get(ASTNode *url) {
  ASTNode *node = ast_create(NODE_HTTP_GET);
  node->data.http_get.url = url;
  return node;
}

ASTNode *ast_http_post(ASTNode *url, ASTNode *body) {
  ASTNode *node = ast_create(NODE_HTTP_POST);
  node->data.http_post.url = url;
  node->data.http_post.body = body;
  return node;
}

ASTNode *ast_ws_connect(ASTNode *url) {
  ASTNode *node = ast_create(NODE_WS_CONNECT);
  node->data.ws_connect.url = url;
  return node;
}

ASTNode *ast_ws_send(ASTNode *data) {
  ASTNode *node = ast_create(NODE_WS_SEND);
  node->data.ws_send.data = data;
  return node;
}

ASTNode *ast_ws_receive(void) { return ast_create(NODE_WS_RECEIVE); }

ASTNode *ast_ws_close(void) { return ast_create(NODE_WS_CLOSE); }

/* --- Library Wrapper APIs (Wave 4 - Navigation & Storage) --- */

ASTNode *ast_imu_attach(ASTNode *port) {
  ASTNode *node = ast_create(NODE_IMU_ATTACH);
  node->data.imu_attach.port = port;
  return node;
}

ASTNode *ast_imu_read_x(void) { return ast_create(NODE_IMU_READ_X); }
ASTNode *ast_imu_read_y(void) { return ast_create(NODE_IMU_READ_Y); }
ASTNode *ast_imu_read_z(void) { return ast_create(NODE_IMU_READ_Z); }
ASTNode *ast_imu_orient(void) { return ast_create(NODE_IMU_ORIENT); }

ASTNode *ast_gps_attach(ASTNode *port, ASTNode *baud) {
  ASTNode *node = ast_create(NODE_GPS_ATTACH);
  node->data.gps_attach.port = port;
  node->data.gps_attach.baud = baud;
  return node;
}

ASTNode *ast_gps_read_lat(void) { return ast_create(NODE_GPS_READ_LAT); }
ASTNode *ast_gps_read_lon(void) { return ast_create(NODE_GPS_READ_LON); }
ASTNode *ast_gps_read_alt(void) { return ast_create(NODE_GPS_READ_ALT); }
ASTNode *ast_gps_read_spd(void) { return ast_create(NODE_GPS_READ_SPD); }

ASTNode *ast_sd_mount(ASTNode *cs_pin) {
  ASTNode *node = ast_create(NODE_SD_MOUNT);
  node->data.sd_mount.cs_pin = cs_pin;
  return node;
}

ASTNode *ast_file_open(ASTNode *filename) {
  ASTNode *node = ast_create(NODE_FILE_OPEN);
  node->data.file_open.filename = filename;
  return node;
}

ASTNode *ast_file_write(ASTNode *data) {
  ASTNode *node = ast_create(NODE_FILE_WRITE);
  node->data.file_write.data = data;
  return node;
}

ASTNode *ast_file_read(void) { return ast_create(NODE_FILE_READ); }
ASTNode *ast_file_close(void) { return ast_create(NODE_FILE_CLOSE); }

ASTNode *ast_lidar_attach(ASTNode *port) {
  ASTNode *node = ast_create(NODE_LIDAR_ATTACH);
  node->data.lidar_attach.port = port;
  return node;
}

ASTNode *ast_lidar_read(void) { return ast_create(NODE_LIDAR_READ); }

/* --- Wave 5: Output Systems & Edge AI Vision --- */

ASTNode *ast_oled_attach(ASTNode *width, ASTNode *height) {
  ASTNode *node = ast_create(NODE_OLED_ATTACH);
  node->data.oled_attach.width = width;
  node->data.oled_attach.height = height;
  return node;
}

ASTNode *ast_oled_print(ASTNode *text, ASTNode *x, ASTNode *y) {
  ASTNode *node = ast_create(NODE_OLED_PRINT);
  node->data.oled_print.text = text;
  node->data.oled_print.x = x;
  node->data.oled_print.y = y;
  return node;
}

ASTNode *ast_oled_draw(int shape, ASTNode *x, ASTNode *y, ASTNode *p1, ASTNode *p2) {
  ASTNode *node = ast_create(NODE_OLED_DRAW);
  node->data.oled_draw.shape = shape;
  node->data.oled_draw.x = x;
  node->data.oled_draw.y = y;
  node->data.oled_draw.param1 = p1;
  node->data.oled_draw.param2 = p2;
  return node;
}

ASTNode *ast_oled_show(void) { return ast_create(NODE_OLED_SHOW); }
ASTNode *ast_oled_clear(void) { return ast_create(NODE_OLED_CLEAR); }

ASTNode *ast_audio_attach(ASTNode *pin) {
  ASTNode *node = ast_create(NODE_AUDIO_ATTACH);
  node->data.audio_attach.pin = pin;
  return node;
}

ASTNode *ast_play_freq(ASTNode *frequency, ASTNode *duration) {
  ASTNode *node = ast_create(NODE_PLAY_FREQ);
  node->data.play_freq.frequency = frequency;
  node->data.play_freq.duration = duration;
  return node;
}

ASTNode *ast_play_sound(ASTNode *name) {
  ASTNode *node = ast_create(NODE_PLAY_SOUND);
  node->data.play_sound.name = name;
  return node;
}

ASTNode *ast_set_volume(ASTNode *level) {
  ASTNode *node = ast_create(NODE_SET_VOLUME);
  node->data.set_volume.level = level;
  return node;
}

ASTNode *ast_cam_attach(ASTNode *protocol) {
  ASTNode *node = ast_create(NODE_CAM_ATTACH);
  node->data.cam_attach.protocol = protocol;
  return node;
}

ASTNode *ast_cam_detect(ASTNode *label) {
  ASTNode *node = ast_create(NODE_CAM_DETECT);
  node->data.cam_detect.label = label;
  return node;
}

ASTNode *ast_cam_obj_x(void) { return ast_create(NODE_CAM_OBJ_X); }
ASTNode *ast_cam_obj_y(void) { return ast_create(NODE_CAM_OBJ_Y); }

/* Wave 6: Advanced Locomotion, Sensor Fusion & Edge AI */
ASTNode *ast_mecanum_attach(ASTNode *fl, ASTNode *fr, ASTNode *bl,
                            ASTNode *br) {
  ASTNode *node = ast_create(NODE_MECANUM_ATTACH);
  node->data.mecanum_attach.fl_pin = fl;
  node->data.mecanum_attach.fr_pin = fr;
  node->data.mecanum_attach.bl_pin = bl;
  node->data.mecanum_attach.br_pin = br;
  return node;
}

ASTNode *ast_mecanum_move(ASTNode *x, ASTNode *y, ASTNode *turn) {
  ASTNode *node = ast_create(NODE_MECANUM_MOVE);
  node->data.mecanum_move.x = x;
  node->data.mecanum_move.y = y;
  node->data.mecanum_move.turn = turn;
  return node;
}

ASTNode *ast_mecanum_stop(void) { return ast_create(NODE_MECANUM_STOP); }

ASTNode *ast_kalman_attach(void) { return ast_create(NODE_KALMAN_ATTACH); }

ASTNode *ast_kalman_compute(ASTNode *raw_value) {
  ASTNode *node = ast_create(NODE_KALMAN_COMPUTE);
  node->data.kalman_compute.raw_value = raw_value;
  return node;
}

ASTNode *ast_ai_load(ASTNode *model_path) {
  ASTNode *node = ast_create(NODE_AI_LOAD);
  node->data.ai_load.model_path = model_path;
  return node;
}

ASTNode *ast_ai_compute(ASTNode *input_array) {
  ASTNode *node = ast_create(NODE_AI_COMPUTE);
  node->data.ai_compute.input_array = input_array;
  return node;
}

/* Wave 7: The Master Automaton */

ASTNode *ast_arm_attach(ASTNode *dof, ASTNode *len1, ASTNode *len2, ASTNode *len3) {
  ASTNode *node = ast_create(NODE_ARM_ATTACH);
  node->data.arm_attach.dof = dof;
  node->data.arm_attach.len1 = len1;
  node->data.arm_attach.len2 = len2;
  node->data.arm_attach.len3 = len3;
  return node;
}

ASTNode *ast_arm_move(ASTNode *x, ASTNode *y, ASTNode *z) {
  ASTNode *node = ast_create(NODE_ARM_MOVE);
  node->data.arm_move.x = x;
  node->data.arm_move.y = y;
  node->data.arm_move.z = z;
  return node;
}

ASTNode *ast_grid_create(const char *name, ASTNode *width, ASTNode *height) {
  ASTNode *node = ast_create(NODE_GRID_CREATE);
  node->data.grid_create.name = strdup(name);
  node->data.grid_create.width = width;
  node->data.grid_create.height = height;
  return node;
}

ASTNode *ast_grid_obstacle(const char *name, ASTNode *x, ASTNode *y) {
  ASTNode *node = ast_create(NODE_GRID_OBSTACLE);
  node->data.grid_obstacle.name = strdup(name);
  node->data.grid_obstacle.x = x;
  node->data.grid_obstacle.y = y;
  return node;
}

ASTNode *ast_path_compute(ASTNode *from_x, ASTNode *from_y, ASTNode *to_x, ASTNode *to_y) {
  ASTNode *node = ast_create(NODE_PATH_COMPUTE);
  node->data.path_compute.from_x = from_x;
  node->data.path_compute.from_y = from_y;
  node->data.path_compute.to_x = to_x;
  node->data.path_compute.to_y = to_y;
  return node;
}

ASTNode *ast_drone_attach(ASTNode *fl, ASTNode *fr, ASTNode *bl, ASTNode *br) {
  ASTNode *node = ast_create(NODE_DRONE_ATTACH);
  node->data.drone_attach.fl = fl;
  node->data.drone_attach.fr = fr;
  node->data.drone_attach.bl = bl;
  node->data.drone_attach.br = br;
  return node;
}

ASTNode *ast_drone_set(ASTNode *pitch, ASTNode *roll, ASTNode *yaw, ASTNode *throttle) {
  ASTNode *node = ast_create(NODE_DRONE_SET);
  node->data.drone_set.pitch = pitch;
  node->data.drone_set.roll = roll;
  node->data.drone_set.yaw = yaw;
  node->data.drone_set.throttle = throttle;
  return node;
}
