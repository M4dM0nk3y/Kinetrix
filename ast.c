/* Kinetrix AST Implementation */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TYPE SYSTEM IMPLEMENTATION
// ============================================================================

Type* type_void() {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_VOID;
    t->element_type = NULL;
    t->return_type = NULL;
    t->param_types = NULL;
    t->param_count = 0;
    t->array_size = 0;
    return t;
}

Type* type_int() {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_INT;
    t->element_type = NULL;
    t->return_type = NULL;
    t->param_types = NULL;
    t->param_count = 0;
    t->array_size = 0;
    return t;
}

Type* type_float() {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_FLOAT;
    t->element_type = NULL;
    t->return_type = NULL;
    t->param_types = NULL;
    t->param_count = 0;
    t->array_size = 0;
    return t;
}

Type* type_bool() {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_BOOL;
    t->element_type = NULL;
    t->return_type = NULL;
    t->param_types = NULL;
    t->param_count = 0;
    t->array_size = 0;
    return t;
}

Type* type_string() {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_STRING;
    t->element_type = NULL;
    t->return_type = NULL;
    t->param_types = NULL;
    t->param_count = 0;
    t->array_size = 0;
    return t;
}

Type* type_array(Type *element_type, int size) {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_ARRAY;
    t->element_type = element_type;
    t->return_type = NULL;
    t->param_types = NULL;
    t->param_count = 0;
    t->array_size = size;
    return t;
}

Type* type_function(Type *return_type, Type **param_types, int param_count) {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_FUNCTION;
    t->element_type = NULL;
    t->return_type = return_type;
    t->param_types = param_types;
    t->param_count = param_count;
    t->array_size = 0;
    return t;
}

Type* type_inferred() {
    Type *t = malloc(sizeof(Type));
    t->kind = TYPE_INFERRED;
    t->element_type = NULL;
    t->return_type = NULL;
    t->param_types = NULL;
    t->param_count = 0;
    t->array_size = 0;
    return t;
}

Type* type_error() {
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
    if (a == NULL || b == NULL) return 0;
    if (a->kind != b->kind) return 0;
    
    switch (a->kind) {
        case TYPE_ARRAY:
            return type_equals(a->element_type, b->element_type);
        case TYPE_FUNCTION:
            if (!type_equals(a->return_type, b->return_type)) return 0;
            if (a->param_count != b->param_count) return 0;
            for (int i = 0; i < a->param_count; i++) {
                if (!type_equals(a->param_types[i], b->param_types[i])) return 0;
            }
            return 1;
        default:
            return 1;
    }
}

const char* type_to_string(Type *t) {
    if (t == NULL) return "null";
    
    static char buffer[256];
    switch (t->kind) {
        case TYPE_VOID: return "void";
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_ARRAY:
            snprintf(buffer, sizeof(buffer), "%s[]", type_to_string(t->element_type));
            return buffer;
        case TYPE_FUNCTION:
            snprintf(buffer, sizeof(buffer), "function");
            return buffer;
        case TYPE_INFERRED: return "inferred";
        case TYPE_ERROR: return "error";
        default: return "unknown";
    }
}

Type* type_clone(Type *t) {
    if (t == NULL) return NULL;
    
    Type *clone = malloc(sizeof(Type));
    clone->kind = t->kind;
    clone->element_type = type_clone(t->element_type);
    clone->return_type = type_clone(t->return_type);
    clone->array_size = t->array_size;
    clone->param_count = t->param_count;
    
    if (t->param_types != NULL) {
        clone->param_types = malloc(sizeof(Type*) * t->param_count);
        for (int i = 0; i < t->param_count; i++) {
            clone->param_types[i] = type_clone(t->param_types[i]);
        }
    } else {
        clone->param_types = NULL;
    }
    
    return clone;
}

void type_free(Type *t) {
    if (t == NULL) return;
    
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

static ASTNode* ast_create(NodeType type) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->value_type = type_inferred();
    node->line = 0;
    return node;
}

ASTNode* ast_number(double value) {
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

ASTNode* ast_string(const char *value) {
    ASTNode *node = ast_create(NODE_STRING);
    node->data.string.value = strdup(value);
    type_free(node->value_type);
    node->value_type = type_string();
    return node;
}

ASTNode* ast_bool(int value) {
    ASTNode *node = ast_create(NODE_BOOL);
    node->data.boolean.value = value;
    type_free(node->value_type);
    node->value_type = type_bool();
    return node;
}

ASTNode* ast_identifier(const char *name) {
    ASTNode *node = ast_create(NODE_IDENTIFIER);
    node->data.identifier.name = strdup(name);
    return node;
}

ASTNode* ast_binary_op(Operator op, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_create(NODE_BINARY_OP);
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

ASTNode* ast_unary_op(Operator op, ASTNode *operand) {
    ASTNode *node = ast_create(NODE_UNARY_OP);
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

ASTNode* ast_call(const char *name, ASTNode **args, int arg_count) {
    ASTNode *node = ast_create(NODE_CALL);
    node->data.call.name = strdup(name);
    node->data.call.args = args;
    node->data.call.arg_count = arg_count;
    return node;
}

ASTNode* ast_array_access(ASTNode *array, ASTNode *index) {
    ASTNode *node = ast_create(NODE_ARRAY_ACCESS);
    node->data.array_access.array = array;
    node->data.array_access.index = index;
    return node;
}

ASTNode* ast_array_literal(ASTNode **elements, int element_count) {
    ASTNode *node = ast_create(NODE_ARRAY_LITERAL);
    node->data.array_literal.elements = elements;
    node->data.array_literal.element_count = element_count;
    return node;
}

ASTNode* ast_var_decl(const char *name, Type *type, ASTNode *initializer) {
    ASTNode *node = ast_create(NODE_VAR_DECL);
    node->data.var_decl.name = strdup(name);
    node->data.var_decl.declared_type = type;
    node->data.var_decl.initializer = initializer;
    node->data.var_decl.is_array = 0;
    node->data.var_decl.array_size = 0;
    return node;
}

ASTNode* ast_array_decl(const char *name, Type *element_type, int size) {
    ASTNode *node = ast_create(NODE_VAR_DECL);
    node->data.var_decl.name = strdup(name);
    node->data.var_decl.declared_type = type_array(element_type, size);
    node->data.var_decl.initializer = NULL;
    node->data.var_decl.is_array = 1;
    node->data.var_decl.array_size = size;
    return node;
}

ASTNode* ast_assignment(ASTNode *target, ASTNode *value) {
    ASTNode *node = ast_create(NODE_ASSIGNMENT);
    node->data.assignment.target = target;
    node->data.assignment.value = value;
    return node;
}

ASTNode* ast_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block) {
    ASTNode *node = ast_create(NODE_IF);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_block = then_block;
    node->data.if_stmt.else_block = else_block;
    return node;
}

ASTNode* ast_while(ASTNode *condition, ASTNode *body) {
    ASTNode *node = ast_create(NODE_WHILE);
    node->data.while_loop.condition = condition;
    node->data.while_loop.body = body;
    return node;
}

ASTNode* ast_repeat(ASTNode *count, ASTNode *body) {
    ASTNode *node = ast_create(NODE_REPEAT);
    node->data.repeat_loop.count = count;
    node->data.repeat_loop.body = body;
    return node;
}

ASTNode* ast_forever(ASTNode *body) {
    ASTNode *node = ast_create(NODE_FOREVER);
    node->data.forever_loop.body = body;
    return node;
}

ASTNode* ast_block(ASTNode **statements, int statement_count) {
    ASTNode *node = ast_create(NODE_BLOCK);
    node->data.block.statements = statements;
    node->data.block.statement_count = statement_count;
    return node;
}

ASTNode* ast_return(ASTNode *value) {
    ASTNode *node = ast_create(NODE_RETURN);
    node->data.return_stmt.value = value;
    return node;
}

ASTNode* ast_break() {
    return ast_create(NODE_BREAK);
}

// Robotics-specific constructors
ASTNode* ast_gpio_write(ASTNode *pin, ASTNode *value) {
    ASTNode *node = ast_create(NODE_GPIO_WRITE);
    node->data.gpio.pin = pin;
    node->data.gpio.value = value;
    return node;
}

ASTNode* ast_gpio_read(ASTNode *pin) {
    ASTNode *node = ast_create(NODE_GPIO_READ);
    node->data.gpio.pin = pin;
    node->data.gpio.value = NULL;
    return node;
}

ASTNode* ast_analog_read(ASTNode *pin) {
    ASTNode *node = ast_create(NODE_ANALOG_READ);
    node->data.gpio.pin = pin;
    node->data.gpio.value = NULL;
    return node;
}

ASTNode* ast_analog_write(ASTNode *pin, ASTNode *value) {
    ASTNode *node = ast_create(NODE_ANALOG_WRITE);
    node->data.gpio.pin = pin;
    node->data.gpio.value = value;
    return node;
}

ASTNode* ast_pulse_read(ASTNode *pin) {
    ASTNode *node = ast_create(NODE_PULSE_READ);
    node->data.gpio.pin = pin;
    node->data.gpio.value = NULL;
    return node;
}

ASTNode* ast_servo_write(ASTNode *pin, ASTNode *angle) {
    ASTNode *node = ast_create(NODE_SERVO_WRITE);
    node->data.gpio.pin = pin;
    node->data.gpio.value = angle;
    return node;
}

ASTNode* ast_i2c_begin() {
    return ast_create(NODE_I2C_BEGIN);
}

ASTNode* ast_i2c_start(ASTNode *address) {
    ASTNode *node = ast_create(NODE_I2C_START);
    node->data.i2c.address = address;
    node->data.i2c.data = NULL;
    return node;
}

ASTNode* ast_i2c_send(ASTNode *data) {
    ASTNode *node = ast_create(NODE_I2C_SEND);
    node->data.i2c.address = NULL;
    node->data.i2c.data = data;
    return node;
}

ASTNode* ast_i2c_stop() {
    return ast_create(NODE_I2C_STOP);
}

ASTNode* ast_i2c_read(ASTNode *address) {
    ASTNode *node = ast_create(NODE_I2C_READ);
    node->data.i2c.address = address;
    node->data.i2c.data = NULL;
    return node;
}

ASTNode* ast_tone(ASTNode *pin, ASTNode *frequency) {
    ASTNode *node = ast_create(NODE_TONE);
    node->data.gpio.pin = pin;
    node->data.gpio.value = frequency;
    return node;
}

ASTNode* ast_notone(ASTNode *pin) {
    ASTNode *node = ast_create(NODE_NOTONE);
    node->data.gpio.pin = pin;
    node->data.gpio.value = NULL;
    return node;
}

ASTNode* ast_wait(ASTNode *duration) {
    ASTNode *node = ast_create(NODE_WAIT);
    node->data.unary.child = duration;
    return node;
}

ASTNode* ast_print_stmt(ASTNode *value) {
    ASTNode *node = ast_create(NODE_PRINT);
    node->data.unary.child = value;
    return node;
}

ASTNode* ast_math_func(MathFunc func, ASTNode *arg1, ASTNode *arg2) {
    ASTNode *node = ast_create(NODE_MATH_FUNC);
    node->data.math_func.func = func;
    node->data.math_func.arg1 = arg1;
    node->data.math_func.arg2 = arg2;
    type_free(node->value_type);
    node->value_type = type_float();
    return node;
}

ASTNode* ast_function_def(const char *name, char **param_names, Type **param_types, 
                          int param_count, Type *return_type, ASTNode *body) {
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

ASTNode* ast_extern_function_def(const char *name, char **param_names, Type **param_types, 
                                 int param_count, Type *return_type, const char *extern_lang) {
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

ASTNode* ast_program(ASTNode **functions, int function_count, ASTNode *main_block) {
    ASTNode *node = ast_create(NODE_PROGRAM);
    node->data.program.functions = functions;
    node->data.program.function_count = function_count;
    node->data.program.main_block = main_block;
    node->data.program.pins_used = NULL;
    node->data.program.pin_count = 0;
    return node;
}

// ============================================================================
// AST UTILITIES
// ============================================================================

void ast_free(ASTNode *node) {
    if (node == NULL) return;
    
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
    if (node == NULL) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
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
