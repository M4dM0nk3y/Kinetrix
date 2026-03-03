/* Kinetrix AST - Abstract Syntax Tree
 * V3.0: Arrays, Interrupts, Protocols, Types, Error Handling, Structs, Tasks, FFI
 */

#ifndef KINETRIX_AST_H
#define KINETRIX_AST_H

#include <stdlib.h>
#include <string.h>

/* Forward declarations */
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
    TYPE_BYTE,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_BUFFER,
    TYPE_STRUCT,
    TYPE_FUNCTION,
    TYPE_INFERRED,  /* For type inference */
    TYPE_ERROR      /* For error recovery */
} TypeKind;

struct Type {
    TypeKind kind;
    Type *element_type;  /* For arrays / buffers */
    Type *return_type;   /* For functions */
    Type **param_types;  /* For functions */
    int param_count;
    int array_size;      /* For fixed-size arrays (-1 for dynamic) */
    char *struct_name;   /* For struct types */
};

/* Type constructors */
Type* type_void();
Type* type_int();
Type* type_float();
Type* type_bool();
Type* type_byte();
Type* type_string();
Type* type_array(Type *element_type, int size);
Type* type_buffer(Type *element_type, int size);
Type* type_struct(const char *name);
Type* type_function(Type *return_type, Type **param_types, int param_count);
Type* type_inferred();
Type* type_error();

/* Type utilities */
int type_equals(Type *a, Type *b);
const char* type_to_string(Type *t);
const char* type_to_ctype(Type *t);   /* Returns C type name: "int", "float", etc. */
Type* type_clone(Type *t);
void type_free(Type *t);

// ============================================================================
// AST NODE TYPES
// ============================================================================

typedef enum {
    /* Literals */
    NODE_NUMBER,
    NODE_STRING,
    NODE_BOOL,
    NODE_IDENTIFIER,

    /* Binary / unary operations */
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_CAST,          /* cast float expr */

    /* Function calls */
    NODE_CALL,

    /* Array / buffer operations */
    NODE_ARRAY_ACCESS,
    NODE_ARRAY_LITERAL,
    NODE_ARRAY_DECL,    /* make array name[N] of T */
    NODE_BUFFER_DECL,   /* make buffer name[N] of T */
    NODE_BUFFER_PUSH,   /* push buf expr */

    /* Struct access */
    NODE_STRUCT_ACCESS, /* r.value */

    /* Statements */
    NODE_VAR_DECL,
    NODE_ASSIGNMENT,
    NODE_IF,
    NODE_WHILE,
    NODE_REPEAT,
    NODE_FOREVER,
    NODE_FOR,
    NODE_BLOCK,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,

    /* Robotics-specific (original) */
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
    NODE_PRINTLN,

    /* Math functions */
    NODE_MATH_FUNC,

    /* Function definition */
    NODE_FUNCTION_DEF,

    /* --- V3.0 new nodes --- */

    /* Hardware interrupts */
    NODE_INTERRUPT_PIN,   /* on pin N rising/falling/changing { } */
    NODE_INTERRUPT_TIMER, /* on timer every N us/ms { } */
    NODE_DISABLE_INTERRUPTS,
    NODE_ENABLE_INTERRUPTS,

    /* UART (Serial) */
    NODE_SERIAL_OPEN,    /* open serial at N baud */
    NODE_SERIAL_SEND,    /* send serial expr */
    NODE_SERIAL_RECV,    /* receive serial (expression) */

    /* I2C high-level */
    NODE_I2C_OPEN,           /* open i2c */
    NODE_I2C_DEVICE_READ,    /* read i2c device addr register reg */
    NODE_I2C_DEVICE_READ_ARRAY, /* read i2c device addr register reg count N into arr */
    NODE_I2C_DEVICE_WRITE,   /* write i2c device addr value val */

    /* SPI */
    NODE_SPI_OPEN,       /* open spi at N hz */
    NODE_SPI_TRANSFER,   /* transfer spi expr (expression) */

    /* Named device abstraction */
    NODE_DEVICE_DEF,     /* define device name as uart/i2c/spi at addr */
    NODE_DEVICE_READ,    /* read devicename */
    NODE_DEVICE_READ_REG,/* read devicename register reg */
    NODE_DEVICE_WRITE,   /* write devicename value val */

    /* Wireless Radio */
    NODE_RADIO_SEND,       /* radio_send_peer(peer_id, data) */
    NODE_RADIO_AVAILABLE,  /* radio_available() */
    NODE_RADIO_READ,       /* radio_read() */

    /* Error handling */
    NODE_TRY,              /* try { } on error { } */
    NODE_WATCHDOG_ENABLE,  /* enable watchdog timeout Nms */
    NODE_WATCHDOG_FEED,    /* feed watchdog */
    NODE_OTA_ENABLE,       /* enable ota "hostname" password "pass" */
    NODE_ASSERT,           /* assert expr else action */

    /* Structs */
    NODE_STRUCT_DEF,       /* define type Name { fields... } */
    NODE_STRUCT_INSTANCE,  /* make StructName varname */

    /* Concurrent tasks */
    NODE_TASK_DEF,         /* task name { body } */
    NODE_TASK_START,       /* start task name */
    NODE_SHARED_DECL,      /* shared make T name = expr */

    /* Program root */
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

typedef enum {
    INT_MODE_RISING,
    INT_MODE_FALLING,
    INT_MODE_CHANGING
} InterruptMode;

typedef enum {
    PROTOCOL_UART,
    PROTOCOL_I2C,
    PROTOCOL_SPI
} ProtocolType;

// ============================================================================
// STRUCT FIELD
// ============================================================================

typedef struct StructField {
    char *name;
    Type *type;
} StructField;

// ============================================================================
// AST NODE STRUCTURE
// ============================================================================

struct ASTNode {
    NodeType type;
    Type *value_type;  /* Type of this expression/statement */
    int line;          /* Source line number for error reporting */

    union {
        /* Literals */
        struct { double value; } number;
        struct { char *value; } string;
        struct { int value; } boolean;
        struct { char *name; } identifier;

        /* Binary operation */
        struct {
            Operator op;
            ASTNode *left;
            ASTNode *right;
        } binary_op;

        /* Unary operation */
        struct {
            Operator op;
            ASTNode *operand;
        } unary_op;

        /* Cast: cast <type> <expr> */
        struct {
            Type *target_type;
            ASTNode *operand;
        } cast_op;

        /* Function call */
        struct {
            char *name;
            ASTNode **args;
            int arg_count;
        } call;

        /* Array access */
        struct {
            ASTNode *array;
            ASTNode *index;
        } array_access;

        /* Array literal */
        struct {
            ASTNode **elements;
            int element_count;
        } array_literal;

        /* Array/buffer declaration */
        struct {
            char *name;
            Type *elem_type;
            int size;
        } array_decl;

        /* Buffer push */
        struct {
            char *buffer_name;
            ASTNode *value;
        } buffer_push;

        /* Struct member access: obj.field */
        struct {
            ASTNode *object;
            char *member;
        } struct_access;

        /* Variable declaration */
        struct {
            char *name;
            Type *declared_type;  /* NULL for inferred */
            ASTNode *initializer; /* NULL if no initializer */
            int is_array;
            int array_size;
            int is_shared;        /* 1 if 'shared' keyword used */
            int is_const;         /* 1 if 'const' keyword used */
        } var_decl;

        /* Assignment */
        struct {
            ASTNode *target;  /* Identifier, array access, or struct access */
            ASTNode *value;
        } assignment;

        /* If statement */
        struct {
            ASTNode *condition;
            ASTNode *then_block;
            ASTNode *else_block;
        } if_stmt;

        /* "while" loop */
        struct {
            ASTNode *condition;
            ASTNode *body;
        } while_loop;

        /* "for" loop: for var from start to end { body } */
        struct {
            char *var_name;
            ASTNode *start_expr;
            ASTNode *end_expr;
            ASTNode *step_expr;
            ASTNode *body;
        } for_loop;

        /* Repeat loop */
        struct {
            ASTNode *count;
            ASTNode *body;
        } repeat_loop;

        /* Forever loop */
        struct { ASTNode *body; } forever_loop;

        /* Block */
        struct {
            ASTNode **statements;
            int statement_count;
        } block;

        /* Return */
        struct { ASTNode *value; } return_stmt;

        /* GPIO operations */
        struct {
            ASTNode *pin;
            ASTNode *value;
        } gpio;

        /* I2C low-level */
        struct {
            ASTNode *address;
            ASTNode *data;
        } i2c;

        /* Math function */
        struct {
            MathFunc func;
            ASTNode *arg1;
            ASTNode *arg2;
        } math_func;

        /* Function definition */
        struct {
            char *name;
            char **param_names;
            Type **param_types;
            int param_count;
            Type *return_type;
            ASTNode *body;
            int is_extern;
            char *extern_lang;
        } function_def;

        /* Hardware interrupt — pin */
        struct {
            int pin_number;        /* literal pin number */
            InterruptMode mode;    /* rising/falling/changing */
            ASTNode *body;
        } interrupt_pin;

        /* Hardware interrupt — timer */
        struct {
            int interval;          /* numeric value */
            int is_us;             /* 1=microseconds, 0=milliseconds */
            ASTNode *body;
            int timer_id;
        } interrupt_timer;

        /* UART open */
        struct {
            int baud_rate;
        } serial_open;

        /* UART send */
        struct {
            ASTNode *value;
        } serial_send;

        /* I2C high-level read: read i2c device addr register reg */
        struct {
            ASTNode *device_addr;
            ASTNode *reg_addr;
        } i2c_device_read;

        /* I2C high-level array read */
        struct {
            ASTNode *device_addr;
            ASTNode *reg_addr;
            ASTNode *count;
            char *array_name;
        } i2c_device_read_array;

        /* I2C high-level write: write i2c device addr value val */
        struct {
            ASTNode *device_addr;
            ASTNode *value;
        } i2c_device_write;

        /* SPI open */
        struct {
            int frequency;
        } spi_open;

        /* SPI transfer */
        struct {
            ASTNode *data;
        } spi_transfer;

        /* Named device definition */
        struct {
            char *device_name;
            ProtocolType protocol;
            ASTNode *address_or_baud;  /* baud for UART, I2C addr for i2c, freq for SPI */
        } device_def;

        /* Named device read */
        struct {
            char *device_name;
            ASTNode *reg;   /* NULL if no register */
            ProtocolType protocol;
        } device_read;

        /* Named device write */
        struct {
            char *device_name;
            ASTNode *value;
            ProtocolType protocol;
        } device_write;

        /* Watchdog */
        struct {
            int timeout_ms;
        } watchdog_enable;

        /* OTA (Over-The-Air) */
        struct {
            char *hostname;
            char *password;
        } ota_enable;

        /* Radio */
        struct {
            ASTNode *peer_id;
            ASTNode *data;
        } radio_send;

        /* Try / on error */
        struct {
            ASTNode *try_block;
            ASTNode *error_block;
        } try_stmt;

        /* Assert */
        struct {
            ASTNode *condition;
            ASTNode *action;    /* expression/call to run on failure */
        } assert_stmt;

        /* Struct definition */
        struct {
            char *name;
            StructField *fields;
            int field_count;
        } struct_def;

        /* Struct instantiation */
        struct {
            char *struct_type;   /* type name */
            char *var_name;      /* instance variable name */
        } struct_instance;

        /* Task definition */
        struct {
            char *name;
            ASTNode *body;
        } task_def;

        /* Start task */
        struct {
            char *task_name;
        } task_start;

        /* Generic single-child nodes (wait, print, etc.) */
        struct { ASTNode *child; } unary;

        /* Program */
        struct {
            ASTNode **functions;
            int function_count;
            ASTNode *main_block;
            int *pins_used;        /* OUTPUT / generic pins */
            int pin_count;
            int *in_pins_used;     /* INPUT pins */
            int in_pin_count;
        } program;
    } data;
};

// ============================================================================
// AST NODE CONSTRUCTORS
// ============================================================================

/* Literals */
ASTNode* ast_number(double value);
ASTNode* ast_string(const char *value);
ASTNode* ast_bool(int value);
ASTNode* ast_identifier(const char *name);

/* Operations */
ASTNode* ast_binary_op(Operator op, ASTNode *left, ASTNode *right);
ASTNode* ast_unary_op(Operator op, ASTNode *operand);
ASTNode* ast_cast(Type *target_type, ASTNode *operand);

/* Function call */
ASTNode* ast_call(const char *name, ASTNode **args, int arg_count);

/* Array / buffer operations */
ASTNode* ast_array_access(ASTNode *array, ASTNode *index);
ASTNode* ast_array_literal(ASTNode **elements, int element_count);
ASTNode* ast_array_decl(const char *name, Type *elem_type, int size);
ASTNode* ast_buffer_decl(const char *name, Type *elem_type, int size);
ASTNode* ast_buffer_push(const char *buffer_name, ASTNode *value);

/* Struct access */
ASTNode* ast_struct_access(ASTNode *object, const char *member);

/* Statements */
ASTNode* ast_var_decl(const char *name, Type *type, ASTNode *initializer);
ASTNode* ast_var_decl_ex(const char *name, Type *type, ASTNode *initializer, int is_shared);
ASTNode* ast_var_decl_const(const char *name, Type *type, ASTNode *initializer);
ASTNode* ast_array_decl_old(const char *name, Type *element_type, int size);
ASTNode* ast_assignment(ASTNode *target, ASTNode *value);
ASTNode* ast_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block);
ASTNode* ast_while(ASTNode *condition, ASTNode *body);
ASTNode* ast_repeat(ASTNode *count, ASTNode *body);
ASTNode* ast_forever(ASTNode *body);
ASTNode* ast_for(const char *var_name, ASTNode *start_expr, ASTNode *end_expr, ASTNode *step_expr, ASTNode *body);
ASTNode* ast_block(ASTNode **statements, int statement_count);
ASTNode* ast_return(ASTNode *value);
ASTNode* ast_break();
ASTNode* ast_continue();

/* Robotics-specific (original) */
ASTNode* ast_gpio_write(ASTNode *pin, ASTNode *value);
ASTNode* ast_gpio_read(ASTNode *pin);
ASTNode* ast_analog_read(ASTNode *pin);
ASTNode* ast_analog_write(ASTNode *pin, ASTNode *value);
ASTNode* ast_pulse_read(ASTNode *pin, ASTNode *timeout);
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
ASTNode* ast_println_stmt(ASTNode *value);

/* Math functions */
ASTNode* ast_math_func(MathFunc func, ASTNode *arg1, ASTNode *arg2);

/* Function definition */
ASTNode* ast_function_def(const char *name, char **param_names, Type **param_types,
                          int param_count, Type *return_type, ASTNode *body);
ASTNode* ast_extern_function_def(const char *name, char **param_names, Type **param_types,
                                 int param_count, Type *return_type, const char *extern_lang);

/* --- V3.0 constructors --- */

/* Interrupts */
ASTNode* ast_interrupt_pin(int pin_number, InterruptMode mode, ASTNode *body);
ASTNode* ast_interrupt_timer(int interval, int is_us, ASTNode *body);

/* UART */
ASTNode* ast_serial_open(int baud_rate);
ASTNode* ast_serial_send(ASTNode *value);
ASTNode* ast_serial_recv();

/* I2C high-level */
ASTNode* ast_i2c_open();
ASTNode* ast_i2c_device_read(ASTNode *device_addr, ASTNode *reg_addr);
ASTNode* ast_i2c_device_read_array(ASTNode *device_addr, ASTNode *reg_addr, ASTNode *count, const char *array_name);
ASTNode* ast_i2c_device_write(ASTNode *device_addr, ASTNode *value);

/* SPI */
ASTNode* ast_spi_open(int frequency);
ASTNode* ast_spi_transfer(ASTNode *data);

/* Named devices */
ASTNode* ast_device_def(const char *name, ProtocolType protocol, ASTNode *addr);
ASTNode* ast_device_read(const char *device_name, ProtocolType protocol, ASTNode *reg);
ASTNode* ast_device_write(const char *device_name, ProtocolType protocol, ASTNode *value);

/* Error handling */
ASTNode* ast_try(ASTNode *try_block, ASTNode *error_block);
ASTNode* ast_watchdog_enable(int timeout_ms);
ASTNode* ast_watchdog_feed();
ASTNode* ast_ota_enable(const char *hostname, const char *password);
ASTNode* ast_disable_interrupts();
ASTNode* ast_enable_interrupts();
ASTNode* ast_assert(ASTNode *condition, ASTNode *action);

/* Structs */
ASTNode* ast_struct_def(const char *name, StructField *fields, int field_count);
ASTNode* ast_struct_instance(const char *struct_type, const char *var_name);

/* Concurrency */
ASTNode* ast_task_def(const char *name, ASTNode *body);
ASTNode* ast_task_start(const char *task_name);

/* Program */
ASTNode* ast_program(ASTNode **functions, int function_count, ASTNode *main_block);

// ============================================================================
// AST UTILITIES
// ============================================================================

void ast_free(ASTNode *node);
void ast_print(ASTNode *node, int indent);
void ast_track_pins(ASTNode *program);

/* --- Radio APIs --- */
ASTNode* ast_radio_send(ASTNode *peer_id, ASTNode *data);
ASTNode* ast_radio_available();
ASTNode* ast_radio_read();

#endif /* KINETRIX_AST_H */
