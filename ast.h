/* Kinetrix AST - Abstract Syntax Tree
 * V3.0: Arrays, Interrupts, Protocols, Types, Error Handling, Structs, Tasks,
 * FFI
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
  TYPE_INFERRED, /* For type inference */
  TYPE_ERROR     /* For error recovery */
} TypeKind;

struct Type {
  TypeKind kind;
  Type *element_type; /* For arrays / buffers */
  Type *return_type;  /* For functions */
  Type **param_types; /* For functions */
  int param_count;
  int array_size;    /* For fixed-size arrays (-1 for dynamic) */
  char *struct_name; /* For struct types */
};

/* Type constructors */
Type *type_void();
Type *type_int();
Type *type_float();
Type *type_bool();
Type *type_byte();
Type *type_string();
Type *type_array(Type *element_type, int size);
Type *type_buffer(Type *element_type, int size);
Type *type_struct(const char *name);
Type *type_function(Type *return_type, Type **param_types, int param_count);
Type *type_inferred();
Type *type_error();

/* Type utilities */
int type_equals(Type *a, Type *b);
const char *type_to_string(Type *t);
const char *
type_to_ctype(Type *t); /* Returns C type name: "int", "float", etc. */
Type *type_clone(Type *t);
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
  NODE_CAST, /* cast float expr */

  /* Function calls */
  NODE_CALL,

  /* Array / buffer operations */
  NODE_ARRAY_ACCESS,
  NODE_ARRAY_LITERAL,
  NODE_ARRAY_DECL,  /* make array name[N] of T */
  NODE_BUFFER_DECL, /* make buffer name[N] of T */
  NODE_BUFFER_PUSH, /* push buf expr */

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
  NODE_SERIAL_OPEN, /* open serial at N baud */
  NODE_SERIAL_SEND, /* send serial expr */
  NODE_SERIAL_RECV, /* receive serial (expression) */

  /* I2C high-level */
  NODE_I2C_OPEN,              /* open i2c */
  NODE_I2C_DEVICE_READ,       /* read i2c device addr register reg */
  NODE_I2C_DEVICE_READ_ARRAY, /* read i2c device addr register reg count N into
                                 arr */
  NODE_I2C_DEVICE_WRITE,      /* write i2c device addr value val */

  /* SPI */
  NODE_SPI_OPEN,     /* open spi at N hz */
  NODE_SPI_TRANSFER, /* transfer spi expr (expression) */

  /* Named device abstraction */
  NODE_DEVICE_DEF,      /* define device name as uart/i2c/spi at addr */
  NODE_DEVICE_READ,     /* read devicename */
  NODE_DEVICE_READ_REG, /* read devicename register reg */
  NODE_DEVICE_WRITE,    /* write devicename value val */

  /* Wireless Radio */
  NODE_RADIO_SEND,      /* radio_send_peer(peer_id, data) */
  NODE_RADIO_AVAILABLE, /* radio_available() */
  NODE_RADIO_READ,      /* radio_read() */

  /* Error handling */
  NODE_TRY,             /* try { } on error { } */
  NODE_WATCHDOG_ENABLE, /* enable watchdog timeout Nms */
  NODE_WATCHDOG_FEED,   /* feed watchdog */
  NODE_OTA_ENABLE,      /* enable ota "hostname" password "pass" */
  NODE_ASSERT,          /* assert expr else action */

  /* Structs */
  NODE_STRUCT_DEF,      /* define type Name { fields... } */
  NODE_STRUCT_INSTANCE, /* make StructName varname */

  /* Concurrent tasks */
  NODE_TASK_DEF,    /* task name { body } */
  NODE_TASK_START,  /* start task name */
  NODE_SHARED_DECL, /* shared make T name = expr */

  /* Library wrappers - Wave 1 */
  NODE_SERVO_ATTACH,   /* attach servo pin N */
  NODE_SERVO_MOVE,     /* move servo to angle */
  NODE_SERVO_DETACH,   /* detach servo pin N */
  NODE_DISTANCE_READ,  /* read distance trigger T echo E */
  NODE_DHT_ATTACH,     /* attach dht11/dht22 pin N */
  NODE_DHT_READ_TEMP,  /* read temperature */
  NODE_DHT_READ_HUMID, /* read humidity */
  NODE_NEOPIXEL_INIT,  /* attach strip pin N count C */
  NODE_NEOPIXEL_SET,   /* set pixel I to R G B */
  NODE_NEOPIXEL_SHOW,  /* show pixels */
  NODE_NEOPIXEL_CLEAR, /* clear pixels */
  NODE_LCD_INIT,       /* attach lcd columns C rows R */
  NODE_LCD_PRINT,      /* lcd print "text" line N */
  NODE_LCD_CLEAR,      /* lcd clear */

  /* Library wrappers - Wave 2 (Motion & Motor) */
  NODE_STEPPER_ATTACH, /* attach stepper step S dir D */
  NODE_STEPPER_SPEED,  /* set stepper speed V */
  NODE_STEPPER_MOVE,   /* move stepper steps */
  NODE_MOTOR_ATTACH,   /* attach motor enable E forward F reverse R */
  NODE_MOTOR_MOVE,     /* move motor <dir> at speed */
  NODE_MOTOR_STOP,     /* stop motor */
  NODE_ENCODER_ATTACH, /* attach encoder pin_a A pin_b B */
  NODE_ENCODER_READ,   /* read encoder */
  NODE_ENCODER_RESET,  /* reset encoder */
  NODE_ESC_ATTACH,     /* attach esc pin N */
  NODE_ESC_THROTTLE,   /* set esc throttle N */
  NODE_PID_ATTACH,     /* attach pid kp P ki I kd D */
  NODE_PID_TARGET,     /* set pid target N */
  NODE_PID_COMPUTE,    /* compute pid current */

  /* Library wrappers - Wave 3 (Communication & Networking) */
  NODE_BLE_ENABLE,     /* enable ble "name" */
  NODE_BLE_ADVERTISE,  /* ble advertise "data" */
  NODE_BLE_SEND,       /* ble send expr */
  NODE_BLE_RECEIVE,    /* ble receive (expression) */
  NODE_WIFI_CONNECT,   /* connect wifi "ssid" password "pass" */
  NODE_WIFI_IP,        /* wifi ip (expression) */
  NODE_MQTT_CONNECT,   /* connect mqtt "broker" port N */
  NODE_MQTT_SUBSCRIBE, /* mqtt subscribe "topic" */
  NODE_MQTT_PUBLISH,   /* mqtt publish "topic" expr */
  NODE_MQTT_READ,      /* mqtt read (expression) */
  NODE_HTTP_GET,       /* http get "url" (expression) */
  NODE_HTTP_POST,      /* http post "url" body expr */
  NODE_WS_CONNECT,     /* connect websocket "url" */
  NODE_WS_SEND,        /* ws send expr */
  NODE_WS_RECEIVE,     /* ws receive (expression) */
  NODE_WS_CLOSE,       /* ws close */

  /* Wave 4: Advanced Robotics & Storage */
  NODE_IMU_ATTACH,   /* attach imu i2c */
  NODE_IMU_READ_X,   /* read accel x | read gyro x */
  NODE_IMU_READ_Y,   /* read accel y | read gyro y */
  NODE_IMU_READ_Z,   /* read accel z | read gyro z */
  NODE_IMU_ORIENT,   /* read orientation */
  NODE_GPS_ATTACH,   /* attach gps serial N */
  NODE_GPS_READ_LAT, /* read latitude */
  NODE_GPS_READ_LON, /* read longitude */
  NODE_GPS_READ_ALT, /* read altitude */
  NODE_GPS_READ_SPD, /* read speed */
  NODE_SD_MOUNT,     /* mount sd pin N */
  NODE_FILE_OPEN,    /* open file "name" */
  NODE_FILE_WRITE,   /* write file expr */
  NODE_FILE_READ,    /* read file */
  NODE_FILE_CLOSE,   /* close file */
  NODE_LIDAR_ATTACH, /* attach lidar i2c */
  NODE_LIDAR_READ,   /* read distance precise */

  /* Program root */
  NODE_PROGRAM
} NodeType;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_EQ,
  OP_NEQ,
  OP_LT,
  OP_GT,
  OP_LTE,
  OP_GTE,
  OP_AND,
  OP_OR,
  OP_NEG,
  OP_NOT
} Operator;

typedef enum {
  MATH_SIN,
  MATH_COS,
  MATH_TAN,
  MATH_SQRT,
  MATH_ASIN,
  MATH_ACOS,
  MATH_ATAN,
  MATH_ATAN2
} MathFunc;

typedef enum {
  INT_MODE_RISING,
  INT_MODE_FALLING,
  INT_MODE_CHANGING
} InterruptMode;

typedef enum { PROTOCOL_UART, PROTOCOL_I2C, PROTOCOL_SPI } ProtocolType;

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
  Type *value_type; /* Type of this expression/statement */
  int line;         /* Source line number for error reporting */

  union {
    /* Literals */
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
      int is_shared; /* 1 if 'shared' keyword used */
      int is_const;  /* 1 if 'const' keyword used */
    } var_decl;

    /* Assignment */
    struct {
      ASTNode *target; /* Identifier, array access, or struct access */
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
    struct {
      ASTNode *body;
    } forever_loop;

    /* Block */
    struct {
      ASTNode **statements;
      int statement_count;
    } block;

    /* Return */
    struct {
      ASTNode *value;
    } return_stmt;

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
      int pin_number;     /* literal pin number */
      InterruptMode mode; /* rising/falling/changing */
      ASTNode *body;
    } interrupt_pin;

    /* Hardware interrupt — timer */
    struct {
      int interval; /* numeric value */
      int is_us;    /* 1=microseconds, 0=milliseconds */
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
      ASTNode
          *address_or_baud; /* baud for UART, I2C addr for i2c, freq for SPI */
    } device_def;

    /* Named device read */
    struct {
      char *device_name;
      ASTNode *reg; /* NULL if no register */
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

    /* Servo */
    struct {
      ASTNode *pin;
    } servo_attach;
    struct {
      ASTNode *angle;
    } servo_write;
    struct {
      ASTNode *pin;
    } servo_detach;

    /* Ultrasonic Distance */
    struct {
      ASTNode *trigger_pin;
      ASTNode *echo_pin;
    } distance_read;

    /* DHT Sensor */
    struct {
      ASTNode *pin;
      int dht_type; /* 11 or 22 */
    } dht_attach;

    /* NeoPixel */
    struct {
      ASTNode *pin;
      ASTNode *count;
    } neopixel_init;
    struct {
      ASTNode *index;
      ASTNode *r;
      ASTNode *g;
      ASTNode *b;
    } neopixel_set;

    /* LCD */
    struct {
      ASTNode *cols;
      ASTNode *rows;
    } lcd_init;
    struct {
      ASTNode *text;
      ASTNode *line;
    } lcd_print;

    /* Stepper Motor */
    struct {
      ASTNode *step_pin;
      ASTNode *dir_pin;
    } stepper_attach;
    struct {
      ASTNode *steps;
    } stepper_move;

    /* DC Motor */
    struct {
      ASTNode *en_pin;
      ASTNode *fwd_pin;
      ASTNode *rev_pin;
    } motor_attach;
    struct {
      int direction; /* 1=forward, -1=reverse */
      ASTNode *speed;
    } motor_move;

    /* Encoder */
    struct {
      ASTNode *pin_a;
      ASTNode *pin_b;
    } encoder_attach;

    /* BLDC ESC */
    struct {
      ASTNode *pin;
    } esc_attach;

    /* PID Controller */
    struct {
      ASTNode *kp;
      ASTNode *ki;
      ASTNode *kd;
    } pid_attach;
    struct {
      ASTNode *current_val;
    } pid_compute;

    /* Wave 3: BLE */
    struct {
      ASTNode *name;
    } ble_enable;
    struct {
      ASTNode *data;
    } ble_advertise;
    struct {
      ASTNode *data;
    } ble_send;
    /* ble_receive has no data fields */

    /* Wave 3: WiFi */
    struct {
      ASTNode *ssid;
      ASTNode *password;
    } wifi_connect;
    /* wifi_ip has no data fields */

    /* Wave 3: MQTT */
    struct {
      ASTNode *broker;
      ASTNode *port;
    } mqtt_connect;
    struct {
      ASTNode *topic;
    } mqtt_subscribe;
    struct {
      ASTNode *topic;
      ASTNode *payload;
    } mqtt_publish;
    /* mqtt_read has no data fields */

    /* Wave 3: HTTP */
    struct {
      ASTNode *url;
    } http_get;
    struct {
      ASTNode *url;
      ASTNode *body;
    } http_post;

    /* Wave 3: WebSocket */
    struct {
      ASTNode *url;
    } ws_connect;
    struct {
      ASTNode *data;
    } ws_send;
    /* ws_receive and ws_close have no data fields */

    /* Wave 4: IMU */
    struct {
      ASTNode *port; /* e.g., i2c or spi */
    } imu_attach;

    /* Wave 4: GPS */
    struct {
      ASTNode *port; /* serial */
      ASTNode *baud; /* 9600 */
    } gps_attach;

    /* Wave 4: Lidar */
    struct {
      ASTNode *port; /* e.g. i2c */
    } lidar_attach;

    /* Wave 4: SD Card / Filesystem */
    struct {
      ASTNode *cs_pin;
    } sd_mount;
    struct {
      ASTNode *filename;
    } file_open;
    struct {
      ASTNode *data;
    } file_write;
    /* file_read and file_close are 0-arity or implicit, no fields */

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
      ASTNode *action; /* expression/call to run on failure */
    } assert_stmt;

    /* Struct definition */
    struct {
      char *name;
      StructField *fields;
      int field_count;
    } struct_def;

    /* Struct instantiation */
    struct {
      char *struct_type; /* type name */
      char *var_name;    /* instance variable name */
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
    struct {
      ASTNode *child;
    } unary;

    /* Program */
    struct {
      ASTNode **functions;
      int function_count;
      ASTNode *main_block;
      int *pins_used; /* OUTPUT / generic pins */
      int pin_count;
      int *in_pins_used; /* INPUT pins */
      int in_pin_count;
    } program;
  } data;
};

// ============================================================================
// AST NODE CONSTRUCTORS
// ============================================================================

/* Literals */
ASTNode *ast_number(double value);
ASTNode *ast_string(const char *value);
ASTNode *ast_bool(int value);
ASTNode *ast_identifier(const char *name);

/* Operations */
ASTNode *ast_binary_op(Operator op, ASTNode *left, ASTNode *right);
ASTNode *ast_unary_op(Operator op, ASTNode *operand);
ASTNode *ast_cast(Type *target_type, ASTNode *operand);

/* Function call */
ASTNode *ast_call(const char *name, ASTNode **args, int arg_count);

/* Array / buffer operations */
ASTNode *ast_array_access(ASTNode *array, ASTNode *index);
ASTNode *ast_array_literal(ASTNode **elements, int element_count);
ASTNode *ast_array_decl(const char *name, Type *elem_type, int size);
ASTNode *ast_buffer_decl(const char *name, Type *elem_type, int size);
ASTNode *ast_buffer_push(const char *buffer_name, ASTNode *value);

/* Struct access */
ASTNode *ast_struct_access(ASTNode *object, const char *member);

/* Statements */
ASTNode *ast_var_decl(const char *name, Type *type, ASTNode *initializer);
ASTNode *ast_var_decl_ex(const char *name, Type *type, ASTNode *initializer,
                         int is_shared);
ASTNode *ast_var_decl_const(const char *name, Type *type, ASTNode *initializer);
ASTNode *ast_array_decl_old(const char *name, Type *element_type, int size);
ASTNode *ast_assignment(ASTNode *target, ASTNode *value);
ASTNode *ast_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block);
ASTNode *ast_while(ASTNode *condition, ASTNode *body);
ASTNode *ast_repeat(ASTNode *count, ASTNode *body);
ASTNode *ast_forever(ASTNode *body);
ASTNode *ast_for(const char *var_name, ASTNode *start_expr, ASTNode *end_expr,
                 ASTNode *step_expr, ASTNode *body);
ASTNode *ast_block(ASTNode **statements, int statement_count);
ASTNode *ast_return(ASTNode *value);
ASTNode *ast_break();
ASTNode *ast_continue();

/* Robotics-specific (original) */
ASTNode *ast_gpio_write(ASTNode *pin, ASTNode *value);
ASTNode *ast_gpio_read(ASTNode *pin);
ASTNode *ast_analog_read(ASTNode *pin);
ASTNode *ast_analog_write(ASTNode *pin, ASTNode *value);
ASTNode *ast_pulse_read(ASTNode *pin, ASTNode *timeout);
ASTNode *ast_servo_write(ASTNode *pin, ASTNode *angle);
ASTNode *ast_i2c_begin();
ASTNode *ast_i2c_start(ASTNode *address);
ASTNode *ast_i2c_send(ASTNode *data);
ASTNode *ast_i2c_stop();
ASTNode *ast_i2c_read(ASTNode *address);
ASTNode *ast_tone(ASTNode *pin, ASTNode *frequency);
ASTNode *ast_notone(ASTNode *pin);
ASTNode *ast_wait(ASTNode *duration);
ASTNode *ast_print_stmt(ASTNode *value);
ASTNode *ast_println_stmt(ASTNode *value);

/* Math functions */
ASTNode *ast_math_func(MathFunc func, ASTNode *arg1, ASTNode *arg2);

/* Function definition */
ASTNode *ast_function_def(const char *name, char **param_names,
                          Type **param_types, int param_count,
                          Type *return_type, ASTNode *body);
ASTNode *ast_extern_function_def(const char *name, char **param_names,
                                 Type **param_types, int param_count,
                                 Type *return_type, const char *extern_lang);

/* --- V3.0 constructors --- */

/* Interrupts */
ASTNode *ast_interrupt_pin(int pin_number, InterruptMode mode, ASTNode *body);
ASTNode *ast_interrupt_timer(int interval, int is_us, ASTNode *body);

/* UART */
ASTNode *ast_serial_open(int baud_rate);
ASTNode *ast_serial_send(ASTNode *value);
ASTNode *ast_serial_recv();

/* I2C high-level */
ASTNode *ast_i2c_open();
ASTNode *ast_i2c_device_read(ASTNode *device_addr, ASTNode *reg_addr);
ASTNode *ast_i2c_device_read_array(ASTNode *device_addr, ASTNode *reg_addr,
                                   ASTNode *count, const char *array_name);
ASTNode *ast_i2c_device_write(ASTNode *device_addr, ASTNode *value);

/* SPI */
ASTNode *ast_spi_open(int frequency);
ASTNode *ast_spi_transfer(ASTNode *data);

/* Named devices */
ASTNode *ast_device_def(const char *name, ProtocolType protocol, ASTNode *addr);
ASTNode *ast_device_read(const char *device_name, ProtocolType protocol,
                         ASTNode *reg);
ASTNode *ast_device_write(const char *device_name, ProtocolType protocol,
                          ASTNode *value);

/* Error handling */
ASTNode *ast_try(ASTNode *try_block, ASTNode *error_block);
ASTNode *ast_watchdog_enable(int timeout_ms);
ASTNode *ast_watchdog_feed();
ASTNode *ast_ota_enable(const char *hostname, const char *password);
ASTNode *ast_disable_interrupts();
ASTNode *ast_enable_interrupts();
ASTNode *ast_assert(ASTNode *condition, ASTNode *action);

/* Structs */
ASTNode *ast_struct_def(const char *name, StructField *fields, int field_count);
ASTNode *ast_struct_instance(const char *struct_type, const char *var_name);

/* Concurrency */
ASTNode *ast_task_def(const char *name, ASTNode *body);
ASTNode *ast_task_start(const char *task_name);

/* Program */
ASTNode *ast_program(ASTNode **functions, int function_count,
                     ASTNode *main_block);

// ============================================================================
// AST UTILITIES
// ============================================================================

void ast_free(ASTNode *node);
void ast_print(ASTNode *node, int indent);
void ast_track_pins(ASTNode *program);

/* --- Radio APIs --- */
ASTNode *ast_radio_send(ASTNode *peer_id, ASTNode *data);
ASTNode *ast_radio_available();
ASTNode *ast_radio_read();

/* --- Library Wrapper APIs (Wave 1) --- */
ASTNode *ast_servo_attach(ASTNode *pin);
ASTNode *ast_servo_move(ASTNode *angle);
ASTNode *ast_servo_detach(ASTNode *pin);
ASTNode *ast_distance_read(ASTNode *trigger_pin, ASTNode *echo_pin);
ASTNode *ast_dht_attach(ASTNode *pin, int dht_type);
ASTNode *ast_dht_read_temp();
ASTNode *ast_dht_read_humid();
ASTNode *ast_neopixel_init(ASTNode *pin, ASTNode *count);
ASTNode *ast_neopixel_set(ASTNode *index, ASTNode *r, ASTNode *g, ASTNode *b);
ASTNode *ast_neopixel_show();
ASTNode *ast_neopixel_clear();
ASTNode *ast_lcd_init(ASTNode *cols, ASTNode *rows);
ASTNode *ast_lcd_print(ASTNode *text, ASTNode *line);
ASTNode *ast_lcd_clear();

/* --- Library Wrapper APIs (Wave 2 - Motion & Motor) --- */
ASTNode *ast_stepper_attach(ASTNode *step_pin, ASTNode *dir_pin);
ASTNode *ast_stepper_speed(ASTNode *speed);
ASTNode *ast_stepper_move(ASTNode *steps);

ASTNode *ast_motor_attach(ASTNode *en_pin, ASTNode *fwd_pin, ASTNode *rev_pin);
ASTNode *ast_motor_move(int direction, ASTNode *speed);
ASTNode *ast_motor_stop();

ASTNode *ast_encoder_attach(ASTNode *pin_a, ASTNode *pin_b);
ASTNode *ast_encoder_read();
ASTNode *ast_encoder_reset();

ASTNode *ast_esc_attach(ASTNode *pin);
ASTNode *ast_esc_throttle(ASTNode *throttle);

ASTNode *ast_pid_attach(ASTNode *kp, ASTNode *ki, ASTNode *kd);
ASTNode *ast_pid_target(ASTNode *target);
ASTNode *ast_pid_compute(ASTNode *current);

/* Wave 3: Communication & Networking */
ASTNode *ast_ble_enable(ASTNode *name);
ASTNode *ast_ble_advertise(ASTNode *data);
ASTNode *ast_ble_send(ASTNode *data);
ASTNode *ast_ble_receive(void);
ASTNode *ast_wifi_connect(ASTNode *ssid, ASTNode *password);
ASTNode *ast_wifi_ip(void);
ASTNode *ast_mqtt_connect(ASTNode *broker, ASTNode *port);
ASTNode *ast_mqtt_subscribe(ASTNode *topic);
ASTNode *ast_mqtt_publish(ASTNode *topic, ASTNode *payload);
ASTNode *ast_mqtt_read(void);
ASTNode *ast_http_get(ASTNode *url);
ASTNode *ast_http_post(ASTNode *url, ASTNode *body);
ASTNode *ast_ws_connect(ASTNode *url);
ASTNode *ast_ws_send(ASTNode *data);
ASTNode *ast_ws_receive(void);
ASTNode *ast_ws_close(void);

/* --- Library Wrapper APIs (Wave 4 - Navigation & Storage) --- */
ASTNode *ast_imu_attach(ASTNode *port);
ASTNode *ast_imu_read_x(void);
ASTNode *ast_imu_read_y(void);
ASTNode *ast_imu_read_z(void);
ASTNode *ast_imu_orient(void);

ASTNode *ast_gps_attach(ASTNode *port, ASTNode *baud);
ASTNode *ast_gps_read_lat(void);
ASTNode *ast_gps_read_lon(void);
ASTNode *ast_gps_read_alt(void);
ASTNode *ast_gps_read_spd(void);

ASTNode *ast_sd_mount(ASTNode *cs_pin);
ASTNode *ast_file_open(ASTNode *filename);
ASTNode *ast_file_write(ASTNode *data);
ASTNode *ast_file_read(void);
ASTNode *ast_file_close(void);

ASTNode *ast_lidar_attach(ASTNode *port);
ASTNode *ast_lidar_read(void);

#endif /* KINETRIX_AST_H */
