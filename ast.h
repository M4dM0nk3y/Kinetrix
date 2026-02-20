/* Kinetrix AST - Abstract Syntax Tree
 * Defines the node structures for the AST representation
 */

#ifndef KINETRIX_AST_H
#define KINETRIX_AST_H

#include <stdlib.h>
#include <string.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Type Type;

// ============================================================================
// TYPE SYSTEM
// ============================================================================

typedef enum {
    TYPE_VOID,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_INFERRED,  // For type inference
    TYPE_ERROR      // For error recovery
} TypeKind;

struct Type {
    TypeKind kind;
    Type *element_type;  // For arrays
    Type *return_type;   // For functions
    Type **param_types;  // For functions
    int param_count;
    int array_size;      // For fixed-size arrays (-1 for dynamic)
};

// Type constructors
Type* type_void();
Type* type_int();
Type* type_float();
Type* type_bool();
Type* type_string();
Type* type_array(Type *element_type, int size);
Type* type_function(Type *return_type, Type **param_types, int param_count);
Type* type_inferred();
Type* type_error();

// Type utilities
int type_equals(Type *a, Type *b);
const char* type_to_string(Type *t);
Type* type_clone(Type *t);
void type_free(Type *t);

// ============================================================================
// AST NODE TYPES
// ============================================================================

typedef enum {
    // Literals
    NODE_NUMBER,
    NODE_STRING,
    NODE_BOOL,
    NODE_IDENTIFIER,
    
    // Binary operations
    NODE_BINARY_OP,
    
    // Unary operations
    NODE_UNARY_OP,
    
    // Function calls
    NODE_CALL,
    
    // Array operations
    NODE_ARRAY_ACCESS,
    NODE_ARRAY_LITERAL,
    
    // Statements
    NODE_VAR_DECL,
    NODE_ASSIGNMENT,
    NODE_IF,
    NODE_WHILE,
    NODE_REPEAT,
    NODE_FOREVER,
    NODE_BLOCK,
    NODE_RETURN,
    NODE_BREAK,
    
    // Robotics-specific
    NODE_GPIO_WRITE,
    NODE_GPIO_READ,
    NODE_ANALOG_READ,
    NODE_ANALOG_WRITE,
    NODE_PULSE_READ,
    NODE_SERVO_WRITE,
    NODE_I2C_BEGIN,
    NODE_I2C_START,
    NODE_I2C_SEND,
    NODE_I2C_STOP,
    NODE_I2C_READ,
    NODE_TONE,
    NODE_NOTONE,
    NODE_WAIT,
    NODE_PRINT,
    
    // Math functions
    NODE_MATH_FUNC,
    
    // Function definition
    NODE_FUNCTION_DEF,
    
    // Program root
    NODE_PROGRAM
} NodeType;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LTE, OP_GTE,
    OP_AND, OP_OR,
    OP_NEG, OP_NOT
} Operator;

typedef enum {
    MATH_SIN, MATH_COS, MATH_TAN, MATH_SQRT,
    MATH_ASIN, MATH_ACOS, MATH_ATAN, MATH_ATAN2
} MathFunc;

// ============================================================================
// AST NODE STRUCTURE
// ============================================================================

struct ASTNode {
    NodeType type;
    Type *value_type;  // Type of this expression/statement
    int line;          // Source line number for error reporting
    
    union {
        // Literals
        struct {
            double value;
        } number;
        
        struct {
            char *value;
        } string;
        
        struct {
            int value;
        } boolean;
        
        struct {
            char *name;
        } identifier;
        
        // Binary operation
        struct {
            Operator op;
            ASTNode *left;
            ASTNode *right;
        } binary_op;
        
        // Unary operation
        struct {
            Operator op;
            ASTNode *operand;
        } unary_op;
        
        // Function call
        struct {
            char *name;
            ASTNode **args;
            int arg_count;
        } call;
        
        // Array access
        struct {
            ASTNode *array;
            ASTNode *index;
        } array_access;
        
        // Array literal
        struct {
            ASTNode **elements;
            int element_count;
        } array_literal;
        
        // Variable declaration
        struct {
            char *name;
            Type *declared_type;  // NULL for inferred
            ASTNode *initializer; // NULL if no initializer
            int is_array;
            int array_size;       // For array declarations
        } var_decl;
        
        // Assignment
        struct {
            ASTNode *target;  // Identifier or array access
            ASTNode *value;
        } assignment;
        
        // If statement
        struct {
            ASTNode *condition;
            ASTNode *then_block;
            ASTNode *else_block;  // NULL if no else
        } if_stmt;
        
        // While loop
        struct {
            ASTNode *condition;
            ASTNode *body;
        } while_loop;
        
        // Repeat loop
        struct {
            ASTNode *count;
            ASTNode *body;
        } repeat_loop;
        
        // Forever loop
        struct {
            ASTNode *body;
        } forever_loop;
        
        // Block
        struct {
            ASTNode **statements;
            int statement_count;
        } block;
        
        // Return
        struct {
            ASTNode *value;  // NULL for void return
        } return_stmt;
        
        // GPIO operations
        struct {
            ASTNode *pin;
            ASTNode *value;  // For write operations
        } gpio;
        
        // I2C operations
        struct {
            ASTNode *address;  // For start/read
            ASTNode *data;     // For send
        } i2c;
        
        // Math function
        struct {
            MathFunc func;
            ASTNode *arg1;
            ASTNode *arg2;  // For atan2
        } math_func;
        
        // Function definition
        struct {
            char *name;
            char **param_names;
            Type **param_types;
            int param_count;
            Type *return_type;
            ASTNode *body;        // NULL if extern
            int is_extern;        // 1 if extern, 0 otherwise
            char *extern_lang;    // e.g. "C++", NULL if not extern
        } function_def;
        
        // Program
        struct {
            ASTNode **functions;
            int function_count;
            ASTNode *main_block;
            int *pins_used;      // Array of GPIO pins used
            int pin_count;       // Number of pins
        } program;
        
        // Generic single-child nodes (wait, print, etc.)
        struct {
            ASTNode *child;
        } unary;
    } data;
};

// ============================================================================
// AST NODE CONSTRUCTORS
// ============================================================================

// Literals
ASTNode* ast_number(double value);
ASTNode* ast_string(const char *value);
ASTNode* ast_bool(int value);
ASTNode* ast_identifier(const char *name);

// Operations
ASTNode* ast_binary_op(Operator op, ASTNode *left, ASTNode *right);
ASTNode* ast_unary_op(Operator op, ASTNode *operand);

// Function call
ASTNode* ast_call(const char *name, ASTNode **args, int arg_count);

// Array operations
ASTNode* ast_array_access(ASTNode *array, ASTNode *index);
ASTNode* ast_array_literal(ASTNode **elements, int element_count);

// Statements
ASTNode* ast_var_decl(const char *name, Type *type, ASTNode *initializer);
ASTNode* ast_array_decl(const char *name, Type *element_type, int size);
ASTNode* ast_assignment(ASTNode *target, ASTNode *value);
ASTNode* ast_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block);
ASTNode* ast_while(ASTNode *condition, ASTNode *body);
ASTNode* ast_repeat(ASTNode *count, ASTNode *body);
ASTNode* ast_forever(ASTNode *body);
ASTNode* ast_block(ASTNode **statements, int statement_count);
ASTNode* ast_return(ASTNode *value);
ASTNode* ast_break();

// Robotics-specific
ASTNode* ast_gpio_write(ASTNode *pin, ASTNode *value);
ASTNode* ast_gpio_read(ASTNode *pin);
ASTNode* ast_analog_read(ASTNode *pin);
ASTNode* ast_analog_write(ASTNode *pin, ASTNode *value);
ASTNode* ast_pulse_read(ASTNode *pin);
ASTNode* ast_servo_write(ASTNode *pin, ASTNode *angle);
ASTNode* ast_i2c_begin();
ASTNode* ast_i2c_start(ASTNode *address);
ASTNode* ast_i2c_send(ASTNode *data);
ASTNode* ast_i2c_stop();
ASTNode* ast_i2c_read(ASTNode *address);
ASTNode* ast_tone(ASTNode *pin, ASTNode *frequency);
ASTNode* ast_notone(ASTNode *pin);
ASTNode* ast_wait(ASTNode *duration);
ASTNode* ast_print_stmt(ASTNode *value);

// Math functions
ASTNode* ast_math_func(MathFunc func, ASTNode *arg1, ASTNode *arg2);

// Function definition
ASTNode* ast_function_def(const char *name, char **param_names, Type **param_types, 
                          int param_count, Type *return_type, ASTNode *body);
ASTNode* ast_extern_function_def(const char *name, char **param_names, Type **param_types, 
                                 int param_count, Type *return_type, const char *extern_lang);

// Program
ASTNode* ast_program(ASTNode **functions, int function_count, ASTNode *main_block);

// ============================================================================
// AST UTILITIES
// ============================================================================

void ast_free(ASTNode *node);
void ast_print(ASTNode *node, int indent);  // For debugging

#endif // KINETRIX_AST_H
