/* Kinetrix Code Generator Implementation - Multi-Target Dispatcher */

#include "codegen.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ── Target helpers ─────────────────────────────────────────────────────────

const char *target_name(Target t) {
  switch (t) {
  case TARGET_ARDUINO:
    return "Arduino";
  case TARGET_ESP32:
    return "ESP32";
  case TARGET_RPI:
    return "Raspberry Pi (Python)";
  case TARGET_PICO:
    return "Raspberry Pi Pico (MicroPython)";
  case TARGET_ROS2:
    return "ROS2";
  default:
    return "Unknown";
  }
}

const char *target_extension(Target t) {
  switch (t) {
  case TARGET_ARDUINO:
    return ".ino";
  case TARGET_ESP32:
    return ".cpp";
  case TARGET_RPI:
    return ".py";
  case TARGET_PICO:
    return ".py";
  case TARGET_ROS2:
    return ".cpp";
  default:
    return ".txt";
  }
}

// ── Constructor / Destructor ───────────────────────────────────────────────

CodeGen *codegen_create(FILE *output) {
  return codegen_create_for_target(output, TARGET_ARDUINO);
}

CodeGen *codegen_create_for_target(FILE *output, Target target) {
  CodeGen *gen = malloc(sizeof(CodeGen));
  gen->output = output;
  gen->indent_level = 0;
  gen->temp_var_counter = 0;
  gen->loop_counter = 0;
  gen->target = target;
  return gen;
}

void codegen_free(CodeGen *gen) { free(gen); }

// ── Shared emit helpers ────────────────────────────────────────────────────

void codegen_emit_indent(CodeGen *gen) {
  for (int i = 0; i < gen->indent_level; i++)
    fprintf(gen->output, "  ");
}

void codegen_emit(CodeGen *gen, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(gen->output, format, args);
  va_end(args);
}

void codegen_emit_line(CodeGen *gen, const char *format, ...) {
  codegen_emit_indent(gen);
  va_list args;
  va_start(args, format);
  vfprintf(gen->output, format, args);
  va_end(args);
  fprintf(gen->output, "\n");
}

// ── Main dispatcher ────────────────────────────────────────────────────────

void codegen_generate(CodeGen *gen, ASTNode *program) {
  switch (gen->target) {
  case TARGET_ESP32:
    codegen_generate_esp32(gen, program);
    break;
  case TARGET_RPI:
    codegen_generate_rpi(gen, program);
    break;
  case TARGET_PICO:
    codegen_generate_pico(gen, program);
    break;
  case TARGET_ROS2:
    codegen_generate_ros2(gen, program);
    break;
  case TARGET_ARDUINO:
  default:
    codegen_generate_arduino(gen, program);
    break;
  }
}

// Forward declarations
static void codegen_expression(CodeGen *gen, ASTNode *node);
static void codegen_statement(CodeGen *gen, ASTNode *node);

// Generate expression
static void codegen_expression(CodeGen *gen, ASTNode *node) {
  if (node == NULL)
    return;

  switch (node->type) {
  case NODE_NUMBER:
    codegen_emit(gen, "%g", node->data.number.value);
    break;

  case NODE_BOOL:
    codegen_emit(gen, "%s", node->data.boolean.value ? "true" : "false");
    break;

  case NODE_STRING:
    codegen_emit(gen, "\"%s\"", node->data.string.value);
    break;

  case NODE_IDENTIFIER:
    codegen_emit(gen, "%s", node->data.identifier.name);
    break;

  case NODE_BINARY_OP: {
    const char *op_str = "";
    switch (node->data.binary_op.op) {
    case OP_ADD:
      op_str = "+";
      break;
    case OP_SUB:
      op_str = "-";
      break;
    case OP_MUL:
      op_str = "*";
      break;
    case OP_DIV:
      op_str = "/";
      break;
    case OP_MOD:
      op_str = "%";
      break;
    case OP_EQ:
      op_str = "==";
      break;
    case OP_NEQ:
      op_str = "!=";
      break;
    case OP_LT:
      op_str = "<";
      break;
    case OP_GT:
      op_str = ">";
      break;
    case OP_LTE:
      op_str = "<=";
      break;
    case OP_GTE:
      op_str = ">=";
      break;
    case OP_AND:
      op_str = "&&";
      break;
    case OP_OR:
      op_str = "||";
      break;
    default:
      op_str = "?";
      break;
    }

    // Special handling for modulo - cast to int
    if (node->data.binary_op.op == OP_MOD) {
      codegen_emit(gen, "((int)(");
      codegen_expression(gen, node->data.binary_op.left);
      codegen_emit(gen, ") %s (int)(", op_str);
      codegen_expression(gen, node->data.binary_op.right);
      codegen_emit(gen, "))");
    } else if (node->data.binary_op.op == OP_ADD &&
               ((node->data.binary_op.left->value_type &&
                 node->data.binary_op.left->value_type->kind == TYPE_STRING) ||
                (node->data.binary_op.right->value_type &&
                 node->data.binary_op.right->value_type->kind ==
                     TYPE_STRING))) {
      codegen_emit(gen, "(String(");
      codegen_expression(gen, node->data.binary_op.left);
      codegen_emit(gen, ") + String(");
      codegen_expression(gen, node->data.binary_op.right);
      codegen_emit(gen, "))");
    } else if (node->data.binary_op.op == OP_DIV) {
      codegen_emit(gen, "((");
      codegen_expression(gen, node->data.binary_op.right);
      codegen_emit(gen, ") == 0 ? 0 : ((");
      codegen_expression(gen, node->data.binary_op.left);
      codegen_emit(gen, ") / (");
      codegen_expression(gen, node->data.binary_op.right);
      codegen_emit(gen, ")))");
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
      if (i > 0)
        codegen_emit(gen, ", ");
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
    if (node->data.gpio.value) {
      codegen_emit(gen, ", HIGH, ");
      codegen_expression(gen, node->data.gpio.value);
      codegen_emit(gen, ")");
    } else {
      codegen_emit(gen, ", HIGH, 23200)");
    }
    break;

  case NODE_I2C_READ:
    codegen_emit(gen, "(Wire.beginTransmission(");
    codegen_expression(gen, node->data.i2c.address);
    codegen_emit(gen, "), Wire.write(");
    codegen_expression(
        gen, node->data.i2c.data); // data field here holds the register address
    codegen_emit(gen, "), Wire.endTransmission(false), Wire.requestFrom(");
    codegen_expression(gen, node->data.i2c.address);
    codegen_emit(gen, ", 1), Wire.read())");
    break;

  case NODE_MATH_FUNC: {
    const char *func_name = "";
    switch (node->data.math_func.func) {
    case MATH_SIN:
      func_name = "sin";
      break;
    case MATH_COS:
      func_name = "cos";
      break;
    case MATH_TAN:
      func_name = "tan";
      break;
    case MATH_SQRT:
      func_name = "sqrt";
      break;
    case MATH_ASIN:
      func_name = "asin";
      break;
    case MATH_ACOS:
      func_name = "acos";
      break;
    case MATH_ATAN:
      func_name = "atan";
      break;
    case MATH_ATAN2:
      func_name = "atan2";
      break;
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

  case NODE_CAST:
    codegen_emit(gen, "(%s)(", type_to_ctype(node->data.cast_op.target_type));
    codegen_expression(gen, node->data.cast_op.operand);
    codegen_emit(gen, ")");
    break;

  case NODE_STRUCT_ACCESS:
    codegen_expression(gen, node->data.struct_access.object);
    codegen_emit(gen, ".%s", node->data.struct_access.member);
    break;

  case NODE_STRUCT_DEF:
  case NODE_DEVICE_DEF:
  case NODE_DEVICE_WRITE:
  case NODE_I2C_DEVICE_WRITE:
  case NODE_I2C_DEVICE_READ_ARRAY:
    /* Not an expression */
    break;

  case NODE_RADIO_AVAILABLE:
    codegen_emit(gen, "radio_available()");
    break;

  case NODE_RADIO_READ:
    codegen_emit(gen, "radio_read()");
    break;

  case NODE_SERIAL_RECV:
    codegen_emit(gen, "Serial.read()");
    break;

  case NODE_SPI_TRANSFER:
    codegen_emit(gen, "SPI.transfer(");
    codegen_expression(gen, node->data.spi_transfer.data);
    codegen_emit(gen, ")");
    break;

  case NODE_ENCODER_READ:
    codegen_emit(gen, "(_kx_encoder ? _kx_encoder->read() : 0)");
    break;

  /* Wave 3: Communication Expressions (Unsupported on Arduino Uno) */
  case NODE_BLE_RECEIVE:
  case NODE_WIFI_IP:
  case NODE_MQTT_READ:
  case NODE_HTTP_GET:
  case NODE_WS_RECEIVE:
    codegen_emit(gen, "(0.0 /* Unsupported on Arduino */)");
    break;

  case NODE_PID_COMPUTE:
    codegen_emit(gen, "(_kx_pid_input = (double)(");
    codegen_expression(gen, node->data.pid_compute.current_val);
    codegen_emit(gen,
                 "), (_kx_pid ? _kx_pid->Compute() : false), _kx_pid_output)");
    break;

  case NODE_I2C_DEVICE_READ:
    codegen_emit(gen, "(Wire.beginTransmission(");
    codegen_expression(gen, node->data.i2c_device_read.device_addr);
    codegen_emit(gen, "), Wire.write(");
    codegen_expression(gen, node->data.i2c_device_read.reg_addr);
    codegen_emit(gen, "), Wire.endTransmission(false), Wire.requestFrom(");
    codegen_expression(gen, node->data.i2c_device_read.device_addr);
    codegen_emit(gen, ", 1), Wire.read())");
    break;

  /* Wave 4: Advanced Robotics & Storage Expressions */
  case NODE_IMU_READ_X:
    codegen_emit(gen, "(float)(_kx_imu ? _kx_imu->readFloatAccelX() : 0.0)");
    break;
  case NODE_IMU_READ_Y:
    codegen_emit(gen, "(float)(_kx_imu ? _kx_imu->readFloatAccelY() : 0.0)");
    break;
  case NODE_IMU_READ_Z:
    codegen_emit(gen, "(float)(_kx_imu ? _kx_imu->readFloatAccelZ() : 0.0)");
    break;
  case NODE_IMU_ORIENT:
    codegen_emit(gen, "(float)(_kx_imu ? _kx_imu_get_heading() : 0.0)");
    break;
  case NODE_GPS_READ_LAT:
    codegen_emit(gen, "(float)(_kx_gps.location.lat())");
    break;
  case NODE_GPS_READ_LON:
    codegen_emit(gen, "(float)(_kx_gps.location.lng())");
    break;
  case NODE_GPS_READ_ALT:
    codegen_emit(gen, "(float)(_kx_gps.altitude.meters())");
    break;
  case NODE_GPS_READ_SPD:
    codegen_emit(gen, "(float)(_kx_gps.speed.kmph())");
    break;
  case NODE_LIDAR_READ:
    codegen_emit(gen, "(float)(_kx_lidar ? "
                      "_kx_lidar->readRangeContinuousMillimeters() : 0.0)");
    break;
  case NODE_FILE_READ:
    codegen_emit(gen, "_kx_file_read_string()");
    break;

  default:
    codegen_emit(gen, "/* unknown expression */");
    break;
  }
}

// Generate statement
static void codegen_statement(CodeGen *gen, ASTNode *node) {
  if (node == NULL)
    return;

  switch (node->type) {
  case NODE_VAR_DECL: {
    const char *ctype = "float";
    if (node->data.var_decl.declared_type)
      ctype = type_to_ctype(node->data.var_decl.declared_type);

    codegen_emit_indent(gen);
    if (node->data.var_decl.is_const)
      codegen_emit(gen, "const ");
    if (node->data.var_decl.is_shared)
      codegen_emit(gen, "volatile ");

    codegen_emit(gen, "%s %s", ctype, node->data.var_decl.name);
    if (node->data.var_decl.initializer) {
      codegen_emit(gen, " = ");
      codegen_expression(gen, node->data.var_decl.initializer);
    }
    codegen_emit(gen, ";\n");
    break;
  }

  case NODE_ARRAY_DECL: {
    const char *ctype = type_to_ctype(node->data.array_decl.elem_type);
    codegen_emit_line(gen, "%s %s[%d] = {0};\n", ctype,
                      node->data.array_decl.name, node->data.array_decl.size);
    break;
  }

  case NODE_BUFFER_DECL: {
    const char *ctype = type_to_ctype(node->data.array_decl.elem_type);
    /* Buffer is implemented as a fixed-size array with a head counter */
    codegen_emit_line(gen, "%s %s[%d] = {0};\n", ctype,
                      node->data.array_decl.name, node->data.array_decl.size);
    codegen_emit_line(gen, "int %s_head = 0;\n", node->data.array_decl.name);
    break;
  }

  case NODE_BUFFER_PUSH:
    codegen_emit_line(
        gen, "%s[%s_head %% (sizeof(%s)/sizeof(%s[0]))] = ",
        node->data.buffer_push.buffer_name, node->data.buffer_push.buffer_name,
        node->data.buffer_push.buffer_name, node->data.buffer_push.buffer_name);
    codegen_expression(gen, node->data.buffer_push.value);
    codegen_emit(gen, "; %s_head++;\n", node->data.buffer_push.buffer_name);
    break;

  /* ---- Interrupts ---- */
  case NODE_INTERRUPT_PIN: {
    const char *mode_str = "RISING";
    if (node->data.interrupt_pin.mode == INT_MODE_FALLING)
      mode_str = "FALLING";
    if (node->data.interrupt_pin.mode == INT_MODE_CHANGING)
      mode_str = "CHANGE";
    /* ISR function emitted at global scope; here emit attachInterrupt */
    codegen_emit_line(
        gen, "attachInterrupt(digitalPinToInterrupt(%d), _isr_pin%d, %s);\n",
        node->data.interrupt_pin.pin_number,
        node->data.interrupt_pin.pin_number, mode_str);
    break;
  }

  case NODE_INTERRUPT_TIMER:
    /* For Arduino/AVR we note the timer but can't add attachTimerInterrupt
       inline; a comment + MsTimer2 approach is emitted. */
    codegen_emit_line(
        gen, "/* timer interrupt every %d%s — use MsTimer2 or similar */\n",
        node->data.interrupt_timer.interval,
        node->data.interrupt_timer.is_us ? "us" : "ms");
    break;

  /* ---- UART ---- */
  case NODE_SERIAL_OPEN:
    codegen_emit_line(gen, "Serial.begin(%d);\n",
                      node->data.serial_open.baud_rate);
    break;

  case NODE_SERIAL_SEND:
    codegen_emit_indent(gen);
    codegen_emit(gen, "Serial.print(");
    codegen_expression(gen, node->data.serial_send.value);
    codegen_emit(gen, ");\n");
    break;

  /* ---- I2C high-level ---- */
  case NODE_I2C_OPEN:
    codegen_emit_line(gen, "Wire.begin();\n");
    break;

  case NODE_I2C_DEVICE_WRITE:
    codegen_emit_line(gen, "Wire.beginTransmission(");
    codegen_expression(gen, node->data.i2c_device_write.device_addr);
    codegen_emit(gen, "); Wire.write(");
    codegen_expression(gen, node->data.i2c_device_write.value);
    codegen_emit(gen, "); Wire.endTransmission();\n");
    break;

  /* ---- SPI ---- */
  case NODE_SPI_OPEN:
    codegen_emit_line(gen,
                      "SPI.begin(); SPI.setClockDivider(SPI_CLOCK_DIV%d);\n",
                      /* closest divider */
                      (16000000 / (node->data.spi_open.frequency > 0
                                       ? node->data.spi_open.frequency
                                       : 1000000) >
                               128
                           ? 128
                           : 16));
    break;

  case NODE_SPI_TRANSFER:
    codegen_emit_indent(gen);
    codegen_emit(gen, "SPI.transfer(");
    codegen_expression(gen, node->data.spi_transfer.data);
    codegen_emit(gen, ");\n");
    break;

  /* ---- Named devices ---- */
  case NODE_DEVICE_DEF:
    codegen_emit_indent(gen);
    codegen_emit(gen, "const int %s = ", node->data.device_def.device_name);
    codegen_expression(gen, node->data.device_def.address_or_baud);
    codegen_emit(gen, ";\n");
    break;

  case NODE_DEVICE_WRITE:
    if (node->data.device_write.protocol == PROTOCOL_I2C) {
      codegen_emit_indent(gen);
      codegen_emit(gen, "Wire.beginTransmission(%s);\n",
                   node->data.device_write.device_name);
      codegen_emit_indent(gen);
      codegen_emit(gen, "Wire.write(");
      codegen_expression(gen, node->data.device_write.value);
      codegen_emit(gen, ");\n");
      codegen_emit_indent(gen);
      codegen_emit(gen, "Wire.endTransmission();\n");
    } else if (node->data.device_write.protocol == PROTOCOL_SPI) {
      codegen_emit_indent(gen);
      codegen_emit(gen, "SPI.transfer(");
      codegen_expression(gen, node->data.device_write.value);
      codegen_emit(gen, ");\n");
    } else {
      codegen_emit_indent(gen);
      codegen_emit(gen, "Serial.println(");
      codegen_expression(gen, node->data.device_write.value);
      codegen_emit(gen, ");\n");
    }
    break;

  /* ---- Error handling ---- */
  case NODE_TRY:
    codegen_emit_line(gen, "try {\n");
    gen->indent_level++;
    codegen_statement(gen, node->data.try_stmt.try_block);
    gen->indent_level--;
    codegen_emit_line(gen, "} catch (...) {\n");
    if (node->data.try_stmt.error_block) {
      gen->indent_level++;
      codegen_statement(gen, node->data.try_stmt.error_block);
      gen->indent_level--;
    }
    codegen_emit_line(gen, "}\n");
    break;

  case NODE_WATCHDOG_ENABLE:
    codegen_emit_line(gen, "wdt_enable(WDTO_%dMS);\n",
                      node->data.watchdog_enable.timeout_ms < 15     ? 15
                      : node->data.watchdog_enable.timeout_ms < 30   ? 30
                      : node->data.watchdog_enable.timeout_ms < 60   ? 60
                      : node->data.watchdog_enable.timeout_ms < 120  ? 120
                      : node->data.watchdog_enable.timeout_ms < 250  ? 250
                      : node->data.watchdog_enable.timeout_ms < 500  ? 500
                      : node->data.watchdog_enable.timeout_ms < 1000 ? 1000
                      : node->data.watchdog_enable.timeout_ms < 2000 ? 2000
                                                                     : 4000);
    break;

  case NODE_OTA_ENABLE:
    codegen_emit_line(gen,
                      "/* OTA not available on Arduino Uno (no WiFi) */\n");
    codegen_emit_line(gen, "/* Use --target esp32 for OTA support */\n");
    break;

  /* ---- Library Wrappers ---- */
  case NODE_SERVO_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_servo.attach(");
    codegen_expression(gen, node->data.servo_attach.pin);
    codegen_emit(gen, ");\n");
    break;
  case NODE_SERVO_MOVE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_servo.write(");
    codegen_expression(gen, node->data.servo_write.angle);
    codegen_emit(gen, ");\n");
    break;
  case NODE_SERVO_DETACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_servo.detach();\n");
    break;
  case NODE_DISTANCE_READ:
    codegen_emit(gen, "_kx_read_distance(");
    codegen_expression(gen, node->data.distance_read.trigger_pin);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.distance_read.echo_pin);
    codegen_emit(gen, ")");
    break;
  case NODE_DHT_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_dht = new DHT(");
    codegen_expression(gen, node->data.dht_attach.pin);
    codegen_emit(gen, ", DHT%d);\n", node->data.dht_attach.dht_type);
    codegen_emit_line(gen, "_kx_dht->begin();\n");
    break;
  case NODE_DHT_READ_TEMP:
    codegen_emit(gen, "_kx_dht->readTemperature()");
    break;
  case NODE_DHT_READ_HUMID:
    codegen_emit(gen, "_kx_dht->readHumidity()");
    break;
  case NODE_NEOPIXEL_INIT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_strip = new Adafruit_NeoPixel(");
    codegen_expression(gen, node->data.neopixel_init.count);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.neopixel_init.pin);
    codegen_emit(gen, ", NEO_GRB + NEO_KHZ800);\n");
    codegen_emit_line(gen, "_kx_strip->begin();\n");
    break;
  case NODE_NEOPIXEL_SET:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_strip->setPixelColor(");
    codegen_expression(gen, node->data.neopixel_set.index);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.neopixel_set.r);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.neopixel_set.g);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.neopixel_set.b);
    codegen_emit(gen, ");\n");
    break;
  case NODE_NEOPIXEL_SHOW:
    codegen_emit_line(gen, "_kx_strip->show();\n");
    break;
  case NODE_NEOPIXEL_CLEAR:
    codegen_emit_line(gen, "_kx_strip->clear();\n");
    codegen_emit_line(gen, "_kx_strip->show();\n");
    break;
  case NODE_LCD_INIT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_lcd = new LiquidCrystal_I2C(0x27, ");
    codegen_expression(gen, node->data.lcd_init.cols);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.lcd_init.rows);
    codegen_emit(gen, ");\n");
    codegen_emit_line(gen, "_kx_lcd->init();\n");
    codegen_emit_line(gen, "_kx_lcd->backlight();\n");
    break;
  case NODE_LCD_PRINT:
    codegen_emit_indent(gen);
    if (node->data.lcd_print.line) {
      codegen_emit(gen, "_kx_lcd->setCursor(0, ");
      codegen_expression(gen, node->data.lcd_print.line);
      codegen_emit(gen, ");\n");
      codegen_emit_indent(gen);
    }
    codegen_emit(gen, "_kx_lcd->print(");
    codegen_expression(gen, node->data.lcd_print.text);
    codegen_emit(gen, ");\n");
    break;
  case NODE_LCD_CLEAR:
    codegen_emit_line(gen, "_kx_lcd->clear();\n");
    break;

  /* --- Wave 2 Wrappers --- */
  case NODE_STEPPER_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_stepper = new Stepper(200, ");
    codegen_expression(gen, node->data.stepper_attach.step_pin);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.stepper_attach.dir_pin);
    codegen_emit(gen, ");\n");
    break;
  case NODE_STEPPER_SPEED:
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (_kx_stepper) _kx_stepper->setSpeed(");
    codegen_expression(gen, node->data.unary.child);
    codegen_emit(gen, ");\n");
    break;
  case NODE_STEPPER_MOVE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (_kx_stepper) _kx_stepper->step(");
    codegen_expression(gen, node->data.stepper_move.steps);
    codegen_emit(gen, ");\n");
    break;
  case NODE_MOTOR_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_motor_en = ");
    codegen_expression(gen, node->data.motor_attach.en_pin);
    codegen_emit(gen, ";\n");
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_motor_fwd = ");
    codegen_expression(gen, node->data.motor_attach.fwd_pin);
    codegen_emit(gen, ";\n");
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_motor_rev = ");
    codegen_expression(gen, node->data.motor_attach.rev_pin);
    codegen_emit(gen, ";\n");
    codegen_emit_line(gen, "pinMode(_kx_motor_en, OUTPUT);\n");
    codegen_emit_line(gen, "pinMode(_kx_motor_fwd, OUTPUT);\n");
    codegen_emit_line(gen, "pinMode(_kx_motor_rev, OUTPUT);\n");
    break;
  case NODE_MOTOR_MOVE:
    codegen_emit_line(gen, "if (_kx_motor_en != -1) {\n");
    gen->indent_level++;
    if (node->data.motor_move.direction > 0) {
      codegen_emit_line(gen, "digitalWrite(_kx_motor_fwd, HIGH);\n");
      codegen_emit_line(gen, "digitalWrite(_kx_motor_rev, LOW);\n");
    } else {
      codegen_emit_line(gen, "digitalWrite(_kx_motor_fwd, LOW);\n");
      codegen_emit_line(gen, "digitalWrite(_kx_motor_rev, HIGH);\n");
    }
    codegen_emit_indent(gen);
    codegen_emit(gen, "analogWrite(_kx_motor_en, ");
    codegen_expression(gen, node->data.motor_move.speed);
    codegen_emit(gen, ");\n");
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  case NODE_MOTOR_STOP:
    codegen_emit_line(gen, "if (_kx_motor_en != -1) {\n");
    gen->indent_level++;
    codegen_emit_line(gen, "digitalWrite(_kx_motor_fwd, LOW);\n");
    codegen_emit_line(gen, "digitalWrite(_kx_motor_rev, LOW);\n");
    codegen_emit_line(gen, "analogWrite(_kx_motor_en, 0);\n");
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  case NODE_ENCODER_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_encoder = new Encoder(");
    codegen_expression(gen, node->data.encoder_attach.pin_a);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.encoder_attach.pin_b);
    codegen_emit(gen, ");\n");
    break;
  case NODE_ENCODER_READ:
    codegen_emit(gen, "(_kx_encoder ? _kx_encoder->read() : 0)");
    break;
  case NODE_ENCODER_RESET:
    codegen_emit_line(gen, "if (_kx_encoder) _kx_encoder->write(0);\n");
    break;
  case NODE_ESC_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_esc.attach(");
    codegen_expression(gen, node->data.esc_attach.pin);
    codegen_emit(gen, ", 1000, 2000);\n");
    break;
  case NODE_ESC_THROTTLE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_esc.write(");
    codegen_expression(gen, node->data.unary.child);
    codegen_emit(gen, ");\n");
    break;
  case NODE_PID_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (_kx_pid) delete _kx_pid;\n");
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_pid = new PID(&_kx_pid_input, &_kx_pid_output, "
                      "&_kx_pid_setpoint, ");
    codegen_expression(gen, node->data.pid_attach.kp);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.pid_attach.ki);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.pid_attach.kd);
    codegen_emit(gen, ", DIRECT);\n");
    codegen_emit_line(gen, "if (_kx_pid) _kx_pid->SetMode(AUTOMATIC);\n");
    break;
  case NODE_PID_TARGET:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_pid_setpoint = ");
    codegen_expression(gen, node->data.unary.child);
    codegen_emit(gen, ";\n");
    break;

  /* Wave 3: Communication Statements (Unsupported on Arduino Uno) */
  case NODE_BLE_ENABLE:
  case NODE_BLE_ADVERTISE:
  case NODE_BLE_SEND:
  case NODE_WIFI_CONNECT:
  case NODE_MQTT_CONNECT:
  case NODE_MQTT_SUBSCRIBE:
  case NODE_MQTT_PUBLISH:
  case NODE_HTTP_POST:
  case NODE_WS_CONNECT:
  case NODE_WS_SEND:
  case NODE_WS_CLOSE:
    codegen_emit_line(gen,
                      "/* Networking unsupported on standard Arduino AVR */\n");
    break;

  case NODE_PID_COMPUTE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "[&]() -> double {\n");
    gen->indent_level++;
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_pid_input = ");
    codegen_expression(gen, node->data.pid_compute.current_val);
    codegen_emit(gen, ";\n");
    codegen_emit_line(gen, "if (_kx_pid) _kx_pid->Compute();\n");
    codegen_emit_line(gen, "return _kx_pid_output;\n");
    gen->indent_level--;
    codegen_emit_line(gen, "}()");
    break;

  case NODE_WATCHDOG_FEED:
    codegen_emit_line(gen, "wdt_reset();\n");
    break;

  case NODE_RADIO_SEND:
    codegen_emit_indent(gen);
    codegen_emit(gen, "radio_send_peer(");
    codegen_expression(gen, node->data.radio_send.peer_id);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.radio_send.data);
    codegen_emit(gen, ");\n");
    break;

  case NODE_DISABLE_INTERRUPTS:
    codegen_emit_line(gen, "noInterrupts();\n");
    break;

  case NODE_ENABLE_INTERRUPTS:
    codegen_emit_line(gen, "interrupts();\n");
    break;

  case NODE_ASSERT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (!(");
    codegen_expression(gen, node->data.assert_stmt.condition);
    codegen_emit(gen, ")) { ");
    if (node->data.assert_stmt.action) {
      codegen_expression(gen, node->data.assert_stmt.action);
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
    codegen_emit_line(gen, "%s %s;\n", node->data.struct_instance.struct_type,
                      node->data.struct_instance.var_name);
    break;

  /* ---- Tasks (Arduino: cooperative via flags) ---- */
  case NODE_TASK_DEF:
    /* Tasks are hoisted as functions at global scope;
       here we just emit a comment showing it's been registered. */
    codegen_emit_line(gen, "/* task '%s' defined as function */\n",
                      node->data.task_def.name);
    break;

  case NODE_TASK_START:
    codegen_emit_line(gen, "%s_run = true; /* start task %s */\n",
                      node->data.task_start.task_name,
                      node->data.task_start.task_name);
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
    codegen_emit_line(gen, "for (int _i%d = 0; _i%d < (int)(", loop_id,
                      loop_id);
    codegen_expression(gen, node->data.repeat_loop.count);
    codegen_emit(gen, "); _i%d++) {\n", loop_id);
    gen->indent_level++;
    codegen_statement(gen, node->data.repeat_loop.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  }

  case NODE_FOR: {
    int loop_id = gen->loop_counter++;
    codegen_emit_indent(gen);
    codegen_emit(gen, "int _start_%d = (", loop_id);
    codegen_expression(gen, node->data.for_loop.start_expr);
    codegen_emit(gen, ");\n");

    codegen_emit_indent(gen);
    codegen_emit(gen, "int _end_%d = (", loop_id);
    codegen_expression(gen, node->data.for_loop.end_expr);
    codegen_emit(gen, ");\n");

    codegen_emit_indent(gen);
    if (node->data.for_loop.step_expr) {
      codegen_emit(gen, "int _step_%d = (", loop_id);
      codegen_expression(gen, node->data.for_loop.step_expr);
      codegen_emit(gen, ");\n");
    } else {
      codegen_emit(gen, "int _step_%d = (_start_%d <= _end_%d) ? 1 : -1;\n",
                   loop_id, loop_id, loop_id);
    }

    codegen_emit_line(gen,
                      "for (int %s = _start_%d; _step_%d > 0 ? %s <= _end_%d : "
                      "%s >= _end_%d; %s += _step_%d) {\n",
                      node->data.for_loop.var_name, loop_id, loop_id,
                      node->data.for_loop.var_name, loop_id,
                      node->data.for_loop.var_name, loop_id,
                      node->data.for_loop.var_name, loop_id);
    gen->indent_level++;
    codegen_statement(gen, node->data.for_loop.body);
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

  case NODE_CONTINUE:
    codegen_emit_line(gen, "continue;\n");
    break;

  case NODE_I2C_DEVICE_READ_ARRAY: {
    codegen_emit_indent(gen);
    codegen_emit(gen, "Wire.beginTransmission(");
    codegen_expression(gen, node->data.i2c_device_read_array.device_addr);
    codegen_emit(gen, "); Wire.write(");
    codegen_expression(gen, node->data.i2c_device_read_array.reg_addr);
    codegen_emit(gen, "); Wire.endTransmission(false); Wire.requestFrom(");
    codegen_expression(gen, node->data.i2c_device_read_array.device_addr);
    codegen_emit(gen, ", ");
    codegen_expression(gen, node->data.i2c_device_read_array.count);
    codegen_emit(gen, ");\n");
    codegen_emit_line(gen, "for (int _i2c_i = 0; _i2c_i < (int)(");
    codegen_expression(gen, node->data.i2c_device_read_array.count);
    codegen_emit(gen, "); _i2c_i++) {\n");
    gen->indent_level++;
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (Wire.available()) %s[_i2c_i] = Wire.read();\n",
                 node->data.i2c_device_read_array.array_name);
    codegen_emit_indent(gen);
    codegen_emit(gen, "else %s[_i2c_i] = 0;\n",
                 node->data.i2c_device_read_array.array_name);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  }

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
    if (gen->inside_task) {
      // Non-blocking wait using millis() state machine
      int state = ++gen->temp_var_counter;
      codegen_emit_indent(gen);
      codegen_emit(gen, "_timer_%d = millis();\n", state);
      codegen_emit_indent(gen);
      codegen_emit(gen, "_state_id = %d;\n", state);
      codegen_emit_indent(gen);
      codegen_emit(gen, "return;\n");
      codegen_emit_indent(gen);
      codegen_emit(gen, "case %d:\n", state);
      gen->indent_level++;
      codegen_emit_indent(gen);
      codegen_emit(gen, "if (millis() - _timer_%d < ", state);
      codegen_expression(gen, node->data.unary.child);
      codegen_emit(gen, ") return;\n");
      gen->indent_level--;
    } else {
      codegen_emit_indent(gen);
      codegen_emit(gen, "delay(");
      codegen_expression(gen, node->data.unary.child);
      codegen_emit(gen, ");\n");
    }
    break;

  case NODE_PRINT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "Serial.print(");
    codegen_expression(gen, node->data.unary.child);
    codegen_emit(gen, ");\n");
    break;

  case NODE_PRINTLN:
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
    // Emit function definition inline (for functions defined inside program
    // block)
    if (node->data.function_def.is_extern) {
      codegen_emit_line(gen, "/* Extern %s function: %s */",
                        node->data.function_def.extern_lang,
                        node->data.function_def.name);
      break;
    }
    codegen_emit_line(gen, "void %s(", node->data.function_def.name);
    for (int i = 0; i < node->data.function_def.param_count; i++) {
      if (i > 0)
        codegen_emit(gen, ", ");
      codegen_emit(gen, "float %s", node->data.function_def.param_names[i]);
    }
    codegen_emit(gen, ") {\n");
    gen->indent_level++;
    codegen_statement(gen, node->data.function_def.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;

  /* Wave 4: Advanced Robotics & Storage Statements */
  case NODE_IMU_ATTACH:
    codegen_emit_line(gen, "/* Setup IMU on I2C */");
    codegen_emit_line(gen, "if (!_kx_imu) {");
    gen->indent_level++;
    codegen_emit_line(gen, "_kx_imu = new SparkFun_BNO080();");
    codegen_emit_line(gen, "_kx_imu->begin();");
    codegen_emit_line(gen, "_kx_imu->enableAccelerometer(50);");
    codegen_emit_line(gen, "_kx_imu->enableGyro(50);");
    codegen_emit_line(gen, "_kx_imu->enableRotationVector(50);");
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;

  case NODE_GPS_ATTACH:
    codegen_emit_line(gen, "/* Setup GPS on Serial */");
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_gps_serial.begin(");
    codegen_expression(gen, node->data.gps_attach.baud);
    codegen_emit(gen, ");\n\n");
    break;

  case NODE_LIDAR_ATTACH:
    codegen_emit_line(gen, "/* Setup LIDAR on I2C */");
    codegen_emit_line(gen, "if (!_kx_lidar) {");
    gen->indent_level++;
    codegen_emit_line(gen, "_kx_lidar = new VL53L0X();");
    codegen_emit_line(gen, "_kx_lidar->init();");
    codegen_emit_line(gen, "_kx_lidar->setTimeout(500);");
    codegen_emit_line(gen, "_kx_lidar->startContinuous();");
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;

  case NODE_SD_MOUNT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (!SD.begin(");
    codegen_expression(gen, node->data.sd_mount.cs_pin);
    codegen_emit(gen, ")) {");
    codegen_emit_line(gen, "  Serial.println(\"SD Mount Failed\");");
    codegen_emit_line(gen, "}\n");
    break;

  case NODE_FILE_OPEN:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_file = SD.open(");
    codegen_expression(gen, node->data.file_open.filename);
    codegen_emit(gen, ", FILE_WRITE);\n");
    break;

  case NODE_FILE_WRITE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (_kx_file) _kx_file.println(");
    codegen_expression(gen, node->data.file_write.data);
    codegen_emit(gen, ");\n");
    break;

  case NODE_FILE_CLOSE:
    codegen_emit_line(gen, "if (_kx_file) _kx_file.close();\n");
    break;

  default:
    codegen_emit_line(gen, "/* unknown statement */\n");
    break;
  }
}

static void codegen_assign_timer_ids(ASTNode *node, int *counter) {
  if (!node)
    return;
  switch (node->type) {
  case NODE_PROGRAM:
    for (int i = 0; i < node->data.program.function_count; i++)
      codegen_assign_timer_ids(node->data.program.functions[i], counter);
    codegen_assign_timer_ids(node->data.program.main_block, counter);
    break;
  case NODE_BLOCK:
    for (int i = 0; i < node->data.block.statement_count; i++)
      codegen_assign_timer_ids(node->data.block.statements[i], counter);
    break;
  case NODE_FUNCTION_DEF:
    codegen_assign_timer_ids(node->data.function_def.body, counter);
    break;
  case NODE_TASK_DEF:
    codegen_assign_timer_ids(node->data.task_def.body, counter);
    break;
  case NODE_IF:
    codegen_assign_timer_ids(node->data.if_stmt.then_block, counter);
    codegen_assign_timer_ids(node->data.if_stmt.else_block, counter);
    break;
  case NODE_WHILE:
    codegen_assign_timer_ids(node->data.while_loop.body, counter);
    break;
  case NODE_FOR:
    codegen_assign_timer_ids(node->data.for_loop.body, counter);
    break;
  case NODE_REPEAT:
    codegen_assign_timer_ids(node->data.repeat_loop.body, counter);
    break;
  case NODE_INTERRUPT_PIN:
    codegen_assign_timer_ids(node->data.interrupt_pin.body, counter);
    break;
  case NODE_INTERRUPT_TIMER:
    node->data.interrupt_timer.timer_id = (*counter)++;
    codegen_assign_timer_ids(node->data.interrupt_timer.body, counter);
    break;
  default:
    break;
  }
}

static void codegen_hoist_isrs(CodeGen *gen, ASTNode *node) {
  if (!node)
    return;
  switch (node->type) {
  case NODE_PROGRAM:
    for (int i = 0; i < node->data.program.function_count; i++)
      codegen_hoist_isrs(gen, node->data.program.functions[i]);
    codegen_hoist_isrs(gen, node->data.program.main_block);
    break;
  case NODE_BLOCK:
    for (int i = 0; i < node->data.block.statement_count; i++)
      codegen_hoist_isrs(gen, node->data.block.statements[i]);
    break;
  case NODE_FUNCTION_DEF:
    codegen_hoist_isrs(gen, node->data.function_def.body);
    break;
  case NODE_TASK_DEF:
    codegen_hoist_isrs(gen, node->data.task_def.body);
    break;
  case NODE_IF:
    codegen_hoist_isrs(gen, node->data.if_stmt.then_block);
    codegen_hoist_isrs(gen, node->data.if_stmt.else_block);
    break;
  case NODE_WHILE:
    codegen_hoist_isrs(gen, node->data.while_loop.body);
    break;
  case NODE_FOR:
    codegen_hoist_isrs(gen, node->data.for_loop.body);
    break;
  case NODE_REPEAT:
    codegen_hoist_isrs(gen, node->data.repeat_loop.body);
    break;
  case NODE_INTERRUPT_PIN:
    codegen_emit_line(gen, "void _isr_pin%d() {\n",
                      node->data.interrupt_pin.pin_number);
    gen->indent_level++;
    codegen_statement(gen, node->data.interrupt_pin.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n\n");
    codegen_hoist_isrs(gen, node->data.interrupt_pin.body);
    break;
  case NODE_INTERRUPT_TIMER:
    codegen_emit_line(gen, "void _isr_timer%d() {\n",
                      node->data.interrupt_timer.timer_id);
    gen->indent_level++;
    codegen_statement(gen, node->data.interrupt_timer.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n\n");
    codegen_hoist_isrs(gen, node->data.interrupt_timer.body);
    break;
  default:
    break;
  }
}

// Generate complete Arduino program
void codegen_generate_arduino(CodeGen *gen, ASTNode *program) {
  if (program == NULL || program->type != NODE_PROGRAM) {
    fprintf(stderr, "Error: Invalid program node\n");
    return;
  }

  ASTNode *block = program->data.program.main_block;

  /* --- Includes --- */
  codegen_emit_line(gen, "#include <Wire.h>\n");
  codegen_emit_line(gen, "#include <SPI.h>\n");
  codegen_emit_line(gen, "#include <avr/wdt.h>\n");
  codegen_emit_line(gen, "#include <Servo.h>\n");
  codegen_emit_line(gen, "#include <DHT.h>\n");
  codegen_emit_line(gen, "#include <Adafruit_NeoPixel.h>\n");
  codegen_emit_line(gen, "#include <LiquidCrystal_I2C.h>\n");
  /* Wave 2 Includes */
  codegen_emit_line(gen, "#include <Stepper.h>\n");
  codegen_emit_line(gen, "#include <Encoder.h>\n");
  codegen_emit_line(gen, "#include <PID_v1.h>\n");
  /* Wave 4 Includes */
  codegen_emit_line(gen, "#include <SparkFun_BNO080_Arduino_Library.h>\n");
  codegen_emit_line(gen, "#include <TinyGPSPlus.h>\n");
  codegen_emit_line(gen, "#include <SoftwareSerial.h>\n");
  codegen_emit_line(gen, "#include <VL53L0X.h>\n");
  codegen_emit_line(gen, "#include <SD.h>\n");
  codegen_emit_line(gen, "\n");
  codegen_emit_line(gen, "Servo _kx_servo;\n");
  codegen_emit_line(gen, "DHT *_kx_dht = NULL;\n");
  codegen_emit_line(gen, "Adafruit_NeoPixel *_kx_strip = NULL;\n");
  codegen_emit_line(gen, "LiquidCrystal_I2C *_kx_lcd = NULL;\n");
  /* Wave 2 Globals */
  codegen_emit_line(gen, "Stepper *_kx_stepper = NULL;\n");
  /* DC Motor uses raw pins */
  codegen_emit_line(
      gen, "int _kx_motor_en = -1, _kx_motor_fwd = -1, _kx_motor_rev = -1;\n");
  codegen_emit_line(gen, "Encoder *_kx_encoder = NULL;\n");
  codegen_emit_line(gen, "Servo _kx_esc;\n"); // ESC uses Servo protocol
  /* PID globals */
  codegen_emit_line(
      gen,
      "double _kx_pid_setpoint = 0, _kx_pid_input = 0, _kx_pid_output = 0;\n");
  codegen_emit_line(gen, "PID *_kx_pid = NULL;\n");
  /* Wave 4 Globals */
  codegen_emit_line(gen, "SparkFun_BNO080 *_kx_imu = NULL;\n");
  codegen_emit_line(gen, "TinyGPSPlus _kx_gps;\n");
  codegen_emit_line(gen, "SoftwareSerial _kx_gps_serial(4, 3); // Default "
                         "RX/TX (Can be overridden)\n");
  codegen_emit_line(gen, "VL53L0X *_kx_lidar = NULL;\n");
  codegen_emit_line(gen, "File _kx_file;\n");

  codegen_emit_line(gen, "\n");

  /* Wave 4 Helpers */
  codegen_emit_line(gen, "float _kx_imu_get_heading() {");
  codegen_emit_line(gen, "  if(!_kx_imu) return 0.0;");
  codegen_emit_line(gen, "  float q0 = _kx_imu->getQuatI();");
  codegen_emit_line(gen, "  float q1 = _kx_imu->getQuatJ();");
  codegen_emit_line(gen, "  float q2 = _kx_imu->getQuatK();");
  codegen_emit_line(gen, "  float q3 = _kx_imu->getQuatReal();");
  codegen_emit_line(gen, "  return atan2(2.0*(q0*q1 + q2*q3), 1.0 - 2.0*(q1*q1 "
                         "+ q2*q2)) * 180.0/M_PI;");
  codegen_emit_line(gen, "}");
  codegen_emit_line(gen, "const char* _kx_file_read_string() {");
  codegen_emit_line(gen, "  if (!_kx_file) return \"\";");
  codegen_emit_line(gen, "  static char buf[256];");
  codegen_emit_line(gen, "  int i=0; while(_kx_file.available() && i<255) "
                         "buf[i++] = _kx_file.read();");
  codegen_emit_line(gen, "  buf[i] = 0;");
  codegen_emit_line(gen, "  return buf;");
  codegen_emit_line(gen, "}\n");

  /* --- Hoist struct definitions --- */
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt && stmt->type == NODE_STRUCT_DEF) {
        codegen_statement(gen, stmt);
        codegen_emit(gen, "\n");
      }
    }
  }

  /* --- Hoist global variables and arrays --- */
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt &&
          (stmt->type == NODE_VAR_DECL || stmt->type == NODE_ARRAY_DECL ||
           stmt->type == NODE_BUFFER_DECL)) {
        codegen_statement(gen, stmt);
      }
    }
  }
  codegen_emit(gen, "\n");
  // Recursively hoist timer/pin interrupt bodies globally
  int timer_id_counter = 0;
  codegen_assign_timer_ids(program, &timer_id_counter);
  codegen_hoist_isrs(gen, program);

  /* --- Hoist task functions with run-flags --- */
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt && stmt->type == NODE_TASK_DEF) {
        codegen_emit_line(gen, "volatile bool %s_run = false;\n",
                          stmt->data.task_def.name);
        codegen_emit_line(gen, "static int _task_state_%s = 0;\n",
                          stmt->data.task_def.name);
        for (int m = 0; m < 10; m++) {
          codegen_emit_line(gen, "static unsigned long _timer_%d = 0;\n",
                            m + 1); // Allocate timers for state machine waits
        }
        codegen_emit_line(gen, "void task_%s() {\n", stmt->data.task_def.name);
        gen->indent_level++;
        codegen_emit_line(gen, "int &_state_id = _task_state_%s;\n",
                          stmt->data.task_def.name);
        codegen_emit_line(gen, "switch(_state_id) {\n");
        codegen_emit_line(gen, "case 0:\n");
        gen->inside_task = 1;
        codegen_statement(gen, stmt->data.task_def.body);
        gen->inside_task = 0;
        codegen_emit_line(gen, "}\n");
        gen->indent_level--;
        codegen_emit_line(gen, "}\n\n");
      }
    }
  }

  /* --- Hoist user function definitions --- */
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt && stmt->type == NODE_FUNCTION_DEF) {
        if (stmt->data.function_def.is_extern) {
          codegen_emit_line(gen, "extern void %s(",
                            stmt->data.function_def.name);
          for (int j = 0; j < stmt->data.function_def.param_count; j++) {
            if (j > 0)
              codegen_emit(gen, ", ");
            const char *pty = "float";
            if (stmt->data.function_def.param_types &&
                stmt->data.function_def.param_types[j])
              pty = type_to_ctype(stmt->data.function_def.param_types[j]);
            codegen_emit(gen, "%s %s", pty,
                         stmt->data.function_def.param_names[j]);
          }
          codegen_emit(gen, ");\n\n");
          continue;
        }
        const char *ret = "void";
        if (stmt->data.function_def.return_type)
          ret = type_to_ctype(stmt->data.function_def.return_type);
        codegen_emit_line(gen, "%s %s(", ret, stmt->data.function_def.name);
        for (int j = 0; j < stmt->data.function_def.param_count; j++) {
          if (j > 0)
            codegen_emit(gen, ", ");
          const char *pty = "float";
          if (stmt->data.function_def.param_types &&
              stmt->data.function_def.param_types[j])
            pty = type_to_ctype(stmt->data.function_def.param_types[j]);
          codegen_emit(gen, "%s %s", pty,
                       stmt->data.function_def.param_names[j]);
        }
        codegen_emit(gen, ") {\n");
        gen->indent_level++;
        codegen_statement(gen, stmt->data.function_def.body);
        gen->indent_level--;
        codegen_emit_line(gen, "}\n\n");
      }
    }
  }

  /* --- setup() --- */
  codegen_emit_line(gen, "void setup() {\n");
  gen->indent_level++;
  codegen_emit_line(gen, "Serial.begin(9600);\n");
  codegen_emit_line(gen, "Serial.setTimeout(100);\n");
  if (program->data.program.pins_used) {
    for (int i = 0; i < program->data.program.pin_count; i++) {
      codegen_emit_line(gen, "pinMode(%d, OUTPUT);\n",
                        program->data.program.pins_used[i]);
    }
  }

  // Auto-configure INPUT pins
  if (program->data.program.in_pins_used) {
    for (int i = 0; i < program->data.program.in_pin_count; i++) {
      codegen_emit_line(gen, "pinMode(%d, INPUT);\n",
                        program->data.program.in_pins_used[i]);
    }
  }

  /* Emit interrupt attachments from the main block */
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt && stmt->type == NODE_INTERRUPT_PIN) {
        codegen_statement(gen, stmt);
      }
    }
  }
  gen->indent_level--;
  codegen_emit_line(gen, "}\n\n");

  /* --- loop() --- */
  codegen_emit_line(gen, "void loop() {\n");
  gen->indent_level++;

  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (!stmt)
        continue;
      /* Skip hoisted stmts */
      if (stmt->type == NODE_FUNCTION_DEF)
        continue;
      if (stmt->type == NODE_STRUCT_DEF)
        continue;
      if (stmt->type == NODE_INTERRUPT_PIN)
        continue;
      if (stmt->type == NODE_TASK_DEF)
        continue;
      if (stmt->type == NODE_VAR_DECL || stmt->type == NODE_ARRAY_DECL ||
          stmt->type == NODE_BUFFER_DECL)
        continue;
      codegen_statement(gen, stmt);
    }
  } else if (block) {
    codegen_statement(gen, block);
  }

  /* Emit task runner calls */
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt && stmt->type == NODE_TASK_DEF) {
        codegen_emit_line(gen, "if (%s_run) task_%s();\n",
                          stmt->data.task_def.name, stmt->data.task_def.name);
      }
    }
  }

  gen->indent_level--;
  codegen_emit_line(gen, "}\n");
}
