/* Kinetrix Raspberry Pi Code Generator
 * Target: Raspberry Pi (Python 3 + RPi.GPIO)
 * Output: Python 3 script
 * Run with: python3 robot.py
 * Requirements: pip install RPi.GPIO Adafruit-MCP3008
 */

#include "ast.h"
#include "codegen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Python uses spaces-only indentation (4 spaces)
static void rpi_indent(CodeGen *gen) {
  for (int i = 0; i < gen->indent_level; i++)
    fprintf(gen->output, "    ");
}

static void rpi_emit(CodeGen *gen, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(gen->output, fmt, args);
  va_end(args);
}

static void rpi_emit_line(CodeGen *gen, const char *fmt, ...) {
  rpi_indent(gen);
  va_list args;
  va_start(args, fmt);
  vfprintf(gen->output, fmt, args);
  va_end(args);
  fprintf(gen->output, "\n");
}

// Forward declarations
static void rpi_expression(CodeGen *gen, ASTNode *node);
static void rpi_statement(CodeGen *gen, ASTNode *node);

// ============================================================
// EXPRESSION GENERATION
// ============================================================

static void rpi_expression(CodeGen *gen, ASTNode *node) {
  if (!node)
    return;

  switch (node->type) {
  case NODE_NUMBER:
    rpi_emit(gen, "%g", node->data.number.value);
    break;
  case NODE_STRING:
    rpi_emit(gen, "\"%s\"", node->data.string.value);
    break;
  case NODE_BOOL:
    rpi_emit(gen, "%s", node->data.boolean.value ? "True" : "False");
    break;
  case NODE_IDENTIFIER:
    rpi_emit(gen, "%s", node->data.identifier.name);
    break;

  case NODE_BINARY_OP: {
    const char *op_str = "?";
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
      op_str = "and";
      break;
    case OP_OR:
      op_str = "or";
      break;
    default:
      break;
    }
    if (node->data.binary_op.op == OP_ADD &&
        ((node->data.binary_op.left->value_type &&
          node->data.binary_op.left->value_type->kind == TYPE_STRING) ||
         (node->data.binary_op.right->value_type &&
          node->data.binary_op.right->value_type->kind == TYPE_STRING))) {
      rpi_emit(gen, "(str(");
      rpi_expression(gen, node->data.binary_op.left);
      rpi_emit(gen, ") + str(");
      rpi_expression(gen, node->data.binary_op.right);
      rpi_emit(gen, "))");
    } else if (node->data.binary_op.op == OP_DIV) {
      rpi_emit(gen, "(0 if (");
      rpi_expression(gen, node->data.binary_op.right);
      rpi_emit(gen, ") == 0 else (");
      rpi_expression(gen, node->data.binary_op.left);
      rpi_emit(gen, ") / (");
      rpi_expression(gen, node->data.binary_op.right);
      rpi_emit(gen, "))");
    } else {
      rpi_emit(gen, "(");
      rpi_expression(gen, node->data.binary_op.left);
      rpi_emit(gen, " %s ", op_str);
      rpi_expression(gen, node->data.binary_op.right);
      rpi_emit(gen, ")");
    }
    break;
  }

  case NODE_UNARY_OP:
    if (node->data.unary_op.op == OP_NOT) {
      rpi_emit(gen, "not (");
      rpi_expression(gen, node->data.unary_op.operand);
      rpi_emit(gen, ")");
    } else {
      rpi_emit(gen, "-(");
      rpi_expression(gen, node->data.unary_op.operand);
      rpi_emit(gen, ")");
    }
    break;

  case NODE_CALL: {
    // Remap built-in functions to Python equivalents
    const char *name = node->data.call.name;
    if (strcmp(name, "map") == 0 && node->data.call.arg_count == 5) {
      // map(v, fl, fh, tl, th) → int((v - fl) * (th - tl) / (fh - fl) + tl)
      rpi_emit(gen, "int((");
      rpi_expression(gen, node->data.call.args[0]);
      rpi_emit(gen, " - ");
      rpi_expression(gen, node->data.call.args[1]);
      rpi_emit(gen, ") * (");
      rpi_expression(gen, node->data.call.args[4]);
      rpi_emit(gen, " - ");
      rpi_expression(gen, node->data.call.args[3]);
      rpi_emit(gen, ") / (");
      rpi_expression(gen, node->data.call.args[2]);
      rpi_emit(gen, " - ");
      rpi_expression(gen, node->data.call.args[1]);
      rpi_emit(gen, ") + ");
      rpi_expression(gen, node->data.call.args[3]);
      rpi_emit(gen, ")");
    } else if (strcmp(name, "constrain") == 0) {
      rpi_emit(gen, "max(");
      rpi_expression(gen, node->data.call.args[1]);
      rpi_emit(gen, ", min(");
      rpi_expression(gen, node->data.call.args[2]);
      rpi_emit(gen, ", ");
      rpi_expression(gen, node->data.call.args[0]);
      rpi_emit(gen, "))");
    } else if (strcmp(name, "random") == 0) {
      rpi_emit(gen, "random.randint(");
      rpi_expression(gen, node->data.call.args[0]);
      rpi_emit(gen, ", ");
      rpi_expression(gen, node->data.call.args[1]);
      rpi_emit(gen, ")");
    } else if (strcmp(name, "delayMicroseconds") == 0) {
      rpi_emit(gen, "time.sleep(");
      rpi_expression(gen, node->data.call.args[0]);
      rpi_emit(gen, " / 1000000.0)");
    } else {
      // User-defined function call
      rpi_emit(gen, "%s(", name);
      for (int i = 0; i < node->data.call.arg_count; i++) {
        if (i > 0)
          rpi_emit(gen, ", ");
        rpi_expression(gen, node->data.call.args[i]);
      }
      rpi_emit(gen, ")");
    }
    break;
  }

  case NODE_ANALOG_READ:
    // Reads via MCP3008 ADC chip (SPI-connected to RPi)
    rpi_emit(gen, "mcp.read_adc(");
    rpi_expression(gen, node->data.gpio.pin);
    rpi_emit(gen, ")");
    break;

  case NODE_GPIO_READ:
    rpi_emit(gen, "GPIO.input(");
    rpi_expression(gen, node->data.gpio.pin);
    rpi_emit(gen, ")");
    break;
  case NODE_ARRAY_ACCESS:
    rpi_expression(gen, node->data.array_access.array);
    rpi_emit(gen, "[int(");
    rpi_expression(gen, node->data.array_access.index);
    rpi_emit(gen, ")]");
    break;
  case NODE_CAST:
    rpi_emit(gen, "%s(",
             strcmp(type_to_ctype(node->data.cast_op.target_type), "float") == 0
                 ? "float"
                 : "int");
    rpi_expression(gen, node->data.cast_op.operand);
    rpi_emit(gen, ")");
    break;
  case NODE_STRUCT_ACCESS:
    rpi_expression(gen, node->data.struct_access.object);
    rpi_emit(gen, "['%s']", node->data.struct_access.member);
    break;
  case NODE_I2C_READ:
    rpi_emit(gen, "(_i2c_read(");
    rpi_expression(gen, node->data.i2c.address);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.i2c.data);
    rpi_emit(gen, "))");
    break;
  case NODE_I2C_DEVICE_READ:
    rpi_emit(gen, "(_i2c_read(");
    rpi_expression(gen, node->data.i2c_device_read.device_addr);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.i2c_device_read.reg_addr);
    rpi_emit(gen, "))");
    break;
  case NODE_SERIAL_RECV:
    rpi_emit(gen, "(_uart.read(1) if '_uart' in globals() else 0)");
    break;
  case NODE_SPI_TRANSFER:
    rpi_emit(gen, "_kx_spi.xfer2([");
    rpi_expression(gen, node->data.spi_transfer.data);
    rpi_emit(gen, "])[0]");
    break;

  case NODE_ENCODER_READ:
    rpi_emit(gen, "(_kx_encoder_pos if '_kx_encoder_pos' in globals() else 0)");
    break;

  /* Wave 3: Communication Expressions */
  case NODE_BLE_RECEIVE:
    rpi_emit(gen, "_kx_ble_msg");
    break;
  case NODE_WIFI_IP:
    rpi_emit(gen, "_kx_wifi_ip");
    break;
  case NODE_MQTT_READ:
    rpi_emit(gen, "_kx_mqtt_msg");
    break;
  case NODE_HTTP_GET:
    rpi_emit(gen, "requests.get(");
    rpi_expression(gen, node->data.unary.child);
    rpi_emit(gen, ").text");
    break;
  case NODE_WS_RECEIVE:
    rpi_emit(gen, "_kx_ws_msg");
    break;

  case NODE_PID_COMPUTE:
    rpi_emit(gen, "_kx_compute_pid(");
    rpi_expression(gen, node->data.pid_compute.current_val);
    rpi_emit(gen, ")");
    break;
  case NODE_KALMAN_COMPUTE:
    rpi_emit(gen, "_kx_kalman_update(");
    rpi_expression(gen, node->data.kalman_compute.raw_value);
    rpi_emit(gen, ")");
    break;
  case NODE_AI_COMPUTE:
    rpi_emit(gen, "_kx_ai_invoke(");
    rpi_expression(gen, node->data.ai_compute.input_array);
    rpi_emit(gen, ")");
    break;
  case NODE_PULSE_READ:
    rpi_emit(gen, "time_pulse_us(");
    rpi_expression(gen, node->data.gpio.pin);
    rpi_emit(gen, ", 30000)");
    break;

  case NODE_DEVICE_WRITE:
  case NODE_STRUCT_DEF:
  case NODE_I2C_DEVICE_WRITE:
  case NODE_I2C_DEVICE_READ_ARRAY:
    break;
  case NODE_RADIO_AVAILABLE:
    rpi_emit(gen, "_radio_available()");
    break;

  case NODE_RADIO_READ:
    rpi_emit(gen, "_radio_read()");
    break;

  case NODE_MATH_FUNC: {
    const char *fn = "";
    switch (node->data.math_func.func) {
    case MATH_SIN:
      fn = "math.sin";
      break;
    case MATH_COS:
      fn = "math.cos";
      break;
    case MATH_TAN:
      fn = "math.tan";
      break;
    case MATH_SQRT:
      fn = "math.sqrt";
      break;
    case MATH_ASIN:
      fn = "math.asin";
      break;
    case MATH_ACOS:
      fn = "math.acos";
      break;
    case MATH_ATAN:
      fn = "math.atan";
      break;
    case MATH_ATAN2:
      fn = "math.atan2";
      break;
    }
    rpi_emit(gen, "%s(", fn);
    rpi_expression(gen, node->data.math_func.arg1);
    if (node->data.math_func.arg2) {
      rpi_emit(gen, ", ");
      rpi_expression(gen, node->data.math_func.arg2);
    }
    rpi_emit(gen, ")");
    break;
  }

  /* Wave 4: Advanced Robotics & Storage Expressions */
  case NODE_IMU_READ_X:
    rpi_emit(gen, "(float(_kx_imu.acceleration[0]) if _kx_imu else 0.0)");
    break;
  case NODE_IMU_READ_Y:
    rpi_emit(gen, "(float(_kx_imu.acceleration[1]) if _kx_imu else 0.0)");
    break;
  case NODE_IMU_READ_Z:
    rpi_emit(gen, "(float(_kx_imu.acceleration[2]) if _kx_imu else 0.0)");
    break;
  case NODE_IMU_ORIENT:
    rpi_emit(gen, "(float(_kx_imu.euler[0]) if _kx_imu else 0.0)");
    break;
  case NODE_GPS_READ_LAT:
    rpi_emit(gen, "(float(_kx_gps.latitude) if _kx_gps else 0.0)");
    break;
  case NODE_GPS_READ_LON:
    rpi_emit(gen, "(float(_kx_gps.longitude) if _kx_gps else 0.0)");
    break;
  case NODE_GPS_READ_ALT:
    rpi_emit(gen, "(float(_kx_gps.altitude) if _kx_gps else 0.0)");
    break;
  case NODE_GPS_READ_SPD:
    rpi_emit(gen, "(float(_kx_gps.spd_over_grnd) if _kx_gps else 0.0)");
    break;
  case NODE_LIDAR_READ:
    rpi_emit(gen, "(float(_kx_lidar.range) if _kx_lidar else 0.0)");
    break;
  case NODE_FILE_READ:
    rpi_emit(gen, "(str(_kx_file.read()) if _kx_file else \"\")");
    break;

  /* Wave 5 Expressions */
  case NODE_CAM_DETECT: rpi_emit(gen, "(1.0 if _kx_husky and _kx_husky.command_request() else 0.0)"); break;
  case NODE_CAM_OBJ_X: rpi_emit(gen, "(float(_kx_husky_blocks[0].x) if _kx_husky_blocks else 0.0)"); break;
  case NODE_CAM_OBJ_Y: rpi_emit(gen, "(float(_kx_husky_blocks[0].y) if _kx_husky_blocks else 0.0)"); break;

  default:
    rpi_emit(gen, "0  # unsupported expression");
    break;
  }
}

// ============================================================
// STATEMENT GENERATION
// ============================================================

static void rpi_statement(CodeGen *gen, ASTNode *node) {
  if (!node)
    return;

  switch (node->type) {
  case NODE_VAR_DECL:
    rpi_indent(gen);
    rpi_emit(gen, "%s", node->data.var_decl.name);
    if (node->data.var_decl.initializer) {
      rpi_emit(gen, " = ");
      rpi_expression(gen, node->data.var_decl.initializer);
    } else {
      rpi_emit(gen, " = 0");
    }
    rpi_emit(gen, "\n");
    break;
  case NODE_ARRAY_DECL:
    rpi_indent(gen);
    rpi_emit(gen, "%s = [0] * %d\n", node->data.array_decl.name,
             node->data.array_decl.size);
    break;
  case NODE_BUFFER_DECL:
    rpi_indent(gen);
    rpi_emit(gen, "%s = []\n", node->data.array_decl.name);
    rpi_indent(gen);
    rpi_emit(gen, "%s_size = %d\n", node->data.array_decl.name,
             node->data.array_decl.size);
    break;
  case NODE_SHARED_DECL:
    rpi_indent(gen);
    rpi_emit(gen, "%s = ", node->data.var_decl.name);
    if (node->data.var_decl.initializer)
      rpi_expression(gen, node->data.var_decl.initializer);
    else
      rpi_emit(gen, "0");
    rpi_emit(gen, "\n");
    break;

  case NODE_ASSIGNMENT:
    rpi_indent(gen);
    rpi_expression(gen, node->data.assignment.target);
    rpi_emit(gen, " = ");
    rpi_expression(gen, node->data.assignment.value);
    rpi_emit(gen, "\n");
    break;

  case NODE_IF:
    rpi_indent(gen);
    rpi_emit(gen, "if ");
    rpi_expression(gen, node->data.if_stmt.condition);
    rpi_emit(gen, ":\n");
    gen->indent_level++;
    rpi_statement(gen, node->data.if_stmt.then_block);
    gen->indent_level--;
    if (node->data.if_stmt.else_block) {
      // Check if else block is a single if (else-if chain → elif)
      ASTNode *else_b = node->data.if_stmt.else_block;
      if (else_b->type == NODE_BLOCK &&
          else_b->data.block.statement_count == 1 &&
          else_b->data.block.statements[0]->type == NODE_IF) {
        // elif
        ASTNode *elif = else_b->data.block.statements[0];
        rpi_indent(gen);
        rpi_emit(gen, "elif ");
        rpi_expression(gen, elif->data.if_stmt.condition);
        rpi_emit(gen, ":\n");
        gen->indent_level++;
        rpi_statement(gen, elif->data.if_stmt.then_block);
        gen->indent_level--;
        if (elif->data.if_stmt.else_block) {
          rpi_emit_line(gen, "else:");
          gen->indent_level++;
          rpi_statement(gen, elif->data.if_stmt.else_block);
          gen->indent_level--;
        }
      } else {
        rpi_emit_line(gen, "else:");
        gen->indent_level++;
        rpi_statement(gen, else_b);
        gen->indent_level--;
      }
    }
    rpi_emit(gen, "\n");
    break;

  case NODE_WHILE:
    rpi_indent(gen);
    rpi_emit(gen, "while ");
    rpi_expression(gen, node->data.while_loop.condition);
    rpi_emit(gen, ":\n");
    gen->indent_level++;
    rpi_statement(gen, node->data.while_loop.body);
    gen->indent_level--;
    rpi_emit(gen, "\n");
    break;

  case NODE_REPEAT: {
    int id = gen->loop_counter++;
    rpi_emit_line(gen, "for _i%d in range(int(", id);
    rpi_expression(gen, node->data.repeat_loop.count);
    rpi_emit(gen, ")):\n");
    gen->indent_level++;
    rpi_statement(gen, node->data.repeat_loop.body);
    gen->indent_level--;
    rpi_emit(gen, "\n");
    break;
  }

  case NODE_FOR: {
    int loop_id = gen->loop_counter++;
    rpi_indent(gen);
    rpi_emit(gen, "_start_%d = int(", loop_id);
    rpi_expression(gen, node->data.for_loop.start_expr);
    rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_end_%d = int(", loop_id);
    rpi_expression(gen, node->data.for_loop.end_expr);
    rpi_emit(gen, ")\n");
    if (node->data.for_loop.step_expr) {
      rpi_indent(gen);
      rpi_emit(gen, "_step_%d = int(", loop_id);
      rpi_expression(gen, node->data.for_loop.step_expr);
      rpi_emit(gen, ")\n");
    } else {
      rpi_indent(gen);
      rpi_emit(gen, "_step_%d = 1 if _start_%d <= _end_%d else -1\n", loop_id,
               loop_id, loop_id);
    }
    rpi_indent(gen);
    rpi_emit(gen,
             "for %s in range(_start_%d, _end_%d + (1 if _step_%d > 0 else "
             "-1), _step_%d):\n",
             node->data.for_loop.var_name, loop_id, loop_id, loop_id, loop_id);
    gen->indent_level++;
    rpi_statement(gen, node->data.for_loop.body);
    gen->indent_level--;
    rpi_emit(gen, "\n");
    break;
  }

  case NODE_FOREVER:
    rpi_emit_line(gen, "while True:\n");
    gen->indent_level++;
    rpi_statement(gen, node->data.forever_loop.body);
    gen->indent_level--;
    rpi_emit(gen, "\n");
    break;

  case NODE_BLOCK:
    for (int i = 0; i < node->data.block.statement_count; i++)
      rpi_statement(gen, node->data.block.statements[i]);
    break;

  case NODE_RETURN:
    rpi_indent(gen);
    rpi_emit(gen, "return");
    if (node->data.return_stmt.value) {
      rpi_emit(gen, " ");
      rpi_expression(gen, node->data.return_stmt.value);
    }
    rpi_emit(gen, "\n");
    break;

  case NODE_BREAK:
    rpi_emit_line(gen, "break\n");
    break;

  case NODE_CONTINUE:
    rpi_emit_line(gen, "continue\n");
    break;

  case NODE_GPIO_WRITE:
    rpi_emit_line(gen, "GPIO.output(");
    rpi_expression(gen, node->data.gpio.pin);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.gpio.value);
    rpi_emit(gen, ")\n");
    break;

  case NODE_ANALOG_WRITE:
    // RPi uses software PWM
    rpi_emit_line(gen, "_pwm_");
    rpi_expression(gen, node->data.gpio.pin);
    rpi_emit(gen, ".ChangeDutyCycle(");
    rpi_expression(gen, node->data.gpio.value);
    rpi_emit(gen, " * 100.0 / 255.0)\n");
    break;

  case NODE_WAIT:
    rpi_emit_line(gen, "time.sleep(");
    rpi_expression(gen, node->data.unary.child);
    rpi_emit(gen, " / 1000.0)\n");
    break;

  case NODE_PRINT:
    rpi_indent(gen);
    rpi_emit(gen, "print(");
    rpi_expression(gen, node->data.unary.child);
    rpi_emit(gen, ", end='')\n");
    break;

  case NODE_PRINTLN:
    rpi_indent(gen);
    rpi_emit(gen, "print(");
    rpi_expression(gen, node->data.unary.child);
    rpi_emit(gen, ")\n");
    break;

  case NODE_CALL:
    rpi_indent(gen);
    rpi_expression(gen, node);
    rpi_emit(gen, "\n");
    break;
  case NODE_BUFFER_PUSH:
    rpi_indent(gen);
    rpi_emit(gen, "%s.append(", node->data.buffer_push.buffer_name);
    rpi_expression(gen, node->data.buffer_push.value);
    rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "if len(%s) > %s_size: %s.pop(0)\n",
             node->data.buffer_push.buffer_name,
             node->data.buffer_push.buffer_name,
             node->data.buffer_push.buffer_name);
    break;
  case NODE_STRUCT_DEF:
    break;
  case NODE_STRUCT_INSTANCE:
    rpi_indent(gen);
    rpi_emit(gen, "%s = {}\n", node->data.struct_instance.var_name);
    break;
  case NODE_INTERRUPT_PIN:
    rpi_indent(gen);
    rpi_emit(gen, "GPIO.add_event_detect(%d, GPIO.",
             node->data.interrupt_pin.pin_number);
    if (node->data.interrupt_pin.mode == INT_MODE_RISING)
      rpi_emit(gen, "RISING");
    else if (node->data.interrupt_pin.mode == INT_MODE_FALLING)
      rpi_emit(gen, "FALLING");
    else
      rpi_emit(gen, "BOTH");
    rpi_emit(gen, ", callback=lambda p: _isr_pin%d())\n",
             node->data.interrupt_pin.pin_number);
    break;
  case NODE_INTERRUPT_TIMER:
    rpi_indent(gen);
    rpi_emit(gen, "import threading\n");
    rpi_indent(gen);
    rpi_emit(gen, "def _timer_loop():\n");
    gen->indent_level++;
    rpi_indent(gen);
    rpi_emit(gen, "while True:\n");
    gen->indent_level++;
    rpi_indent(gen);
    rpi_emit(gen, "time.sleep(%g)\n",
             node->data.interrupt_timer.interval /
                 (node->data.interrupt_timer.is_us ? 1000000.0 : 1000.0));
    rpi_indent(gen);
    rpi_emit(gen, "_isr_timer()\n");
    gen->indent_level--;
    gen->indent_level--;
    rpi_indent(gen);
    rpi_emit(gen,
             "threading.Thread(target=_timer_loop, daemon=True).start()\n");
    break;
  case NODE_DISABLE_INTERRUPTS:
    rpi_emit_line(
        gen, "pass # Disable IRQ not directly supported in user-space python");
    break;
  case NODE_ENABLE_INTERRUPTS:
    rpi_emit_line(gen, "pass # Enable IRQ");
    break;
  case NODE_I2C_OPEN:
    rpi_emit_line(gen, "_i2c = busio.I2C(board.SCL, board.SDA)");
    break;
  case NODE_I2C_DEVICE_READ_ARRAY:
    rpi_indent(gen);
    rpi_emit(gen, "_i2c_data = bus.read_i2c_block_data(");
    rpi_expression(gen, node->data.i2c_device_read_array.device_addr);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.i2c_device_read_array.reg_addr);
    rpi_emit(gen, ", int(");
    rpi_expression(gen, node->data.i2c_device_read_array.count);
    rpi_emit(gen, "))\n");

    rpi_indent(gen);
    rpi_emit(gen, "for _i2c_i, _i2c_b in enumerate(_i2c_data):\n");
    gen->indent_level++;
    rpi_indent(gen);
    rpi_emit(gen, "%s[_i2c_i] = _i2c_b\n",
             node->data.i2c_device_read_array.array_name);
    gen->indent_level--;
    break;

  case NODE_I2C_DEVICE_WRITE:
    /* Note: this is actually writing to an address not a register directly but
     * let's emulate */
    rpi_indent(gen);
    rpi_emit(gen, "bus.write_byte(");
    rpi_expression(gen, node->data.i2c_device_write.device_addr);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.i2c_device_write.value);
    rpi_emit(gen, ")\n");
    break;
  case NODE_SERIAL_OPEN:
    rpi_emit_line(gen,
                  "import serial; _uart = serial.Serial('/dev/serial0', %d)",
                  node->data.serial_open.baud_rate);
    break;
  case NODE_SERIAL_SEND:
    rpi_indent(gen);
    rpi_emit(gen, "if '_uart' in globals(): _uart.write(str(");
    rpi_expression(gen, node->data.serial_send.value);
    rpi_emit(gen, ").encode())\n");
    break;
  case NODE_SPI_OPEN:
    rpi_emit_line(
        gen, "if '_spi' not in globals(): _spi = busio.SPI(clock=board.SCK, "
             "MISO=board.MISO, MOSI=board.MOSI)");
    break;
  case NODE_DEVICE_DEF:
    rpi_indent(gen);
    rpi_emit(gen, "# Device %s configured\n",
             node->data.device_def.device_name);
    break;
  case NODE_DEVICE_WRITE:
    rpi_indent(gen);
    rpi_emit(gen, "# Write %s: ", node->data.device_write.device_name);
    rpi_expression(gen, node->data.device_write.value);
    rpi_emit(gen, "\n");
    break;
  case NODE_TASK_DEF:
    break;
  case NODE_TASK_START:
    rpi_emit_line(gen, "import threading");
    rpi_emit_line(gen,
                  "threading.Thread(target=_task_func_%s, daemon=True).start()",
                  node->data.task_start.task_name);
    break;
  case NODE_TRY:
    rpi_emit_line(gen, "try:");
    gen->indent_level++;
    rpi_statement(gen, node->data.try_stmt.try_block);
    gen->indent_level--;
    if (node->data.try_stmt.error_block) {
      rpi_emit_line(gen, "except Exception:");
      gen->indent_level++;
      rpi_statement(gen, node->data.try_stmt.error_block);
      gen->indent_level--;
    }
    break;
  case NODE_ASSERT:
    rpi_indent(gen);
    rpi_emit(gen, "if not (");
    rpi_expression(gen, node->data.assert_stmt.condition);
    rpi_emit(gen, "):\n");
    gen->indent_level++;
    if (node->data.assert_stmt.action)
      rpi_statement(gen, node->data.assert_stmt.action);
    else {
      rpi_indent(gen);
      rpi_emit(gen, "raise AssertionError()\n");
    }
    gen->indent_level--;
    break;

  case NODE_RADIO_SEND:
    rpi_indent(gen);
    rpi_emit(gen, "_radio_send(");
    rpi_expression(gen, node->data.radio_send.peer_id);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.radio_send.data);
    rpi_emit(gen, ")\n");
    break;
  case NODE_WATCHDOG_ENABLE:
    rpi_emit_line(gen, "pass # WDT requires OS setup");
    break;
  case NODE_WATCHDOG_FEED:
    rpi_emit_line(gen, "pass # WDT feed");
    break;
  case NODE_OTA_ENABLE:
    rpi_emit_line(gen, "# OTA: Register on local network for fleet discovery");
    rpi_emit_line(gen, "import subprocess, socket");
    rpi_emit_line(gen, "_ota_hostname = \"%s\"",
                  node->data.ota_enable.hostname);
    rpi_emit_line(gen, "try:");
    rpi_emit_line(
        gen,
        "    subprocess.Popen(['avahi-publish-service', _ota_hostname, "
        "'_kinetrix._tcp', '5050', 'target=rpi'], stdout=subprocess.DEVNULL)");
    rpi_emit_line(gen, "    print(f\"OTA discoverable as {_ota_hostname} on "
                       "{socket.gethostbyname(socket.gethostname())}\")");
    rpi_emit_line(gen, "except: print(\"Warning: avahi not available, OTA "
                       "discovery disabled\")");
    break;

  /* ---- Library Wrappers ---- */
  case NODE_SERVO_ATTACH:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_servo = GPIO.PWM(");
    rpi_expression(gen, node->data.servo_attach.pin);
    rpi_emit(gen, ", 50)\n");
    rpi_emit_line(gen, "_kx_servo.start(0)");
    break;
  case NODE_SERVO_MOVE:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_servo.ChangeDutyCycle(2.5 + ");
    rpi_expression(gen, node->data.servo_write.angle);
    rpi_emit(gen, " / 18.0)\n");
    break;
  case NODE_SERVO_DETACH:
    rpi_emit_line(gen, "_kx_servo.stop()");
    break;
  case NODE_DISTANCE_READ:
    rpi_emit(gen, "_kx_read_distance(");
    rpi_expression(gen, node->data.distance_read.trigger_pin);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.distance_read.echo_pin);
    rpi_emit(gen, ")");
    break;
  case NODE_DHT_ATTACH:
    rpi_emit_line(gen, "import Adafruit_DHT");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_dht_type = Adafruit_DHT.DHT%d\n",
             node->data.dht_attach.dht_type);
    rpi_indent(gen);
    rpi_emit(gen, "_kx_dht_pin = ");
    rpi_expression(gen, node->data.dht_attach.pin);
    rpi_emit(gen, "\n");
    break;
  case NODE_DHT_READ_TEMP:
    rpi_emit(gen, "Adafruit_DHT.read_retry(_kx_dht_type, _kx_dht_pin)[1]");
    break;
  case NODE_DHT_READ_HUMID:
    rpi_emit(gen, "Adafruit_DHT.read_retry(_kx_dht_type, _kx_dht_pin)[0]");
    break;
  case NODE_NEOPIXEL_INIT:
    rpi_emit_line(gen, "from rpi_ws281x import PixelStrip, Color");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_strip = PixelStrip(int(");
    rpi_expression(gen, node->data.neopixel_init.count);
    rpi_emit(gen, "), int(");
    rpi_expression(gen, node->data.neopixel_init.pin);
    rpi_emit(gen, "))\n");
    rpi_emit_line(gen, "_kx_strip.begin()");
    break;
  case NODE_NEOPIXEL_SET:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_strip.setPixelColor(int(");
    rpi_expression(gen, node->data.neopixel_set.index);
    rpi_emit(gen, "), Color(int(");
    rpi_expression(gen, node->data.neopixel_set.r);
    rpi_emit(gen, "), int(");
    rpi_expression(gen, node->data.neopixel_set.g);
    rpi_emit(gen, "), int(");
    rpi_expression(gen, node->data.neopixel_set.b);
    rpi_emit(gen, ")))\n");
    break;
  case NODE_NEOPIXEL_SHOW:
    rpi_emit_line(gen, "_kx_strip.show()");
    break;
  case NODE_NEOPIXEL_CLEAR:
    rpi_emit_line(gen, "for _i in range(_kx_strip.numPixels()): "
                       "_kx_strip.setPixelColor(_i, Color(0,0,0))");
    rpi_emit_line(gen, "_kx_strip.show()");
    break;
  case NODE_LCD_INIT:
    rpi_emit_line(gen, "from RPLCD.i2c import CharLCD");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_lcd = CharLCD('PCF8574', 0x27, cols=int(");
    rpi_expression(gen, node->data.lcd_init.cols);
    rpi_emit(gen, "), rows=int(");
    rpi_expression(gen, node->data.lcd_init.rows);
    rpi_emit(gen, "))\n");
    break;
  case NODE_LCD_PRINT:
    if (node->data.lcd_print.line) {
      rpi_indent(gen);
      rpi_emit(gen, "_kx_lcd.cursor_pos = (int(");
      rpi_expression(gen, node->data.lcd_print.line);
      rpi_emit(gen, "), 0)\n");
    }
    rpi_indent(gen);
    rpi_emit(gen, "_kx_lcd.write_string(str(");
    rpi_expression(gen, node->data.lcd_print.text);
    rpi_emit(gen, "))\n");
    break;
  case NODE_LCD_CLEAR:
    rpi_emit_line(gen, "_kx_lcd.clear()");
    break;

  /* --- Wave 2 Wrappers --- */
  case NODE_STEPPER_ATTACH:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_stepper_step = int(");
    rpi_expression(gen, node->data.stepper_attach.step_pin);
    rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_stepper_dir = int(");
    rpi_expression(gen, node->data.stepper_attach.dir_pin);
    rpi_emit(gen, ")\n");
    rpi_emit_line(gen, "GPIO.setup(_kx_stepper_step, GPIO.OUT)");
    rpi_emit_line(gen, "GPIO.setup(_kx_stepper_dir, GPIO.OUT)");
    rpi_emit_line(gen, "_kx_stepper_speed = 100");
    break;
  case NODE_STEPPER_SPEED:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_stepper_speed = max(1, ");
    rpi_expression(gen, node->data.unary.child);
    rpi_emit(gen, ")\n");
    break;
  case NODE_STEPPER_MOVE:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_steps = int(");
    rpi_expression(gen, node->data.stepper_move.steps);
    rpi_emit(gen, ")\n");
    rpi_emit_line(gen, "GPIO.output(_kx_stepper_dir, GPIO.HIGH if _kx_steps > "
                       "0 else GPIO.LOW)");
    rpi_emit_line(gen, "_kx_delay = 60.0 / (200.0 * _kx_stepper_speed)");
    rpi_emit_line(gen, "for _ in range(abs(_kx_steps)):");
    gen->indent_level++;
    rpi_emit_line(gen, "GPIO.output(_kx_stepper_step, GPIO.HIGH)");
    rpi_emit_line(gen, "time.sleep(_kx_delay / 2)");
    rpi_emit_line(gen, "GPIO.output(_kx_stepper_step, GPIO.LOW)");
    rpi_emit_line(gen, "time.sleep(_kx_delay / 2)");
    gen->indent_level--;
    break;
  case NODE_MOTOR_ATTACH:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_motor_en = int(");
    rpi_expression(gen, node->data.motor_attach.en_pin);
    rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_motor_fwd = int(");
    rpi_expression(gen, node->data.motor_attach.fwd_pin);
    rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_motor_rev = int(");
    rpi_expression(gen, node->data.motor_attach.rev_pin);
    rpi_emit(gen, ")\n");
    rpi_emit_line(gen, "GPIO.setup(_kx_motor_fwd, GPIO.OUT)");
    rpi_emit_line(gen, "GPIO.setup(_kx_motor_rev, GPIO.OUT)");
    rpi_emit_line(gen, "GPIO.setup(_kx_motor_en, GPIO.OUT)");
    rpi_emit_line(gen, "_kx_motor_pwm = GPIO.PWM(_kx_motor_en, 1000)");
    rpi_emit_line(gen, "_kx_motor_pwm.start(0)");
    break;
  case NODE_MOTOR_MOVE:
    if (node->data.motor_move.direction > 0) {
      rpi_emit_line(gen, "GPIO.output(_kx_motor_fwd, GPIO.HIGH)");
      rpi_emit_line(gen, "GPIO.output(_kx_motor_rev, GPIO.LOW)");
    } else {
      rpi_emit_line(gen, "GPIO.output(_kx_motor_fwd, GPIO.LOW)");
      rpi_emit_line(gen, "GPIO.output(_kx_motor_rev, GPIO.HIGH)");
    }
    rpi_indent(gen);
    rpi_emit(gen, "_kx_motor_pwm.ChangeDutyCycle((");
    rpi_expression(gen, node->data.motor_move.speed);
    rpi_emit(gen, ") * 100.0 / 255.0)\n");
    break;
  case NODE_MOTOR_STOP:
    rpi_emit_line(gen, "GPIO.output(_kx_motor_fwd, GPIO.LOW)");
    rpi_emit_line(gen, "GPIO.output(_kx_motor_rev, GPIO.LOW)");
    rpi_emit_line(gen, "_kx_motor_pwm.ChangeDutyCycle(0)");
    break;

  case NODE_MECANUM_ATTACH:
    rpi_indent(gen);
    rpi_emit(gen, "global _kx_mec_fl, _kx_mec_fr, _kx_mec_bl, _kx_mec_br, ");
    rpi_emit(gen, "_kx_mec_pwm_fl, _kx_mec_pwm_fr, _kx_mec_pwm_bl, _kx_mec_pwm_br\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mec_fl = int("); rpi_expression(gen, node->data.mecanum_attach.fl_pin); rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mec_fr = int("); rpi_expression(gen, node->data.mecanum_attach.fr_pin); rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mec_bl = int("); rpi_expression(gen, node->data.mecanum_attach.bl_pin); rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mec_br = int("); rpi_expression(gen, node->data.mecanum_attach.br_pin); rpi_emit(gen, ")\n");
    rpi_emit_line(gen, "GPIO.setup(_kx_mec_fl, GPIO.OUT)");
    rpi_emit_line(gen, "GPIO.setup(_kx_mec_fr, GPIO.OUT)");
    rpi_emit_line(gen, "GPIO.setup(_kx_mec_bl, GPIO.OUT)");
    rpi_emit_line(gen, "GPIO.setup(_kx_mec_br, GPIO.OUT)");
    rpi_emit_line(gen, "_kx_mec_pwm_fl = GPIO.PWM(_kx_mec_fl, 1000)");
    rpi_emit_line(gen, "_kx_mec_pwm_fr = GPIO.PWM(_kx_mec_fr, 1000)");
    rpi_emit_line(gen, "_kx_mec_pwm_bl = GPIO.PWM(_kx_mec_bl, 1000)");
    rpi_emit_line(gen, "_kx_mec_pwm_br = GPIO.PWM(_kx_mec_br, 1000)");
    rpi_emit_line(gen, "_kx_mec_pwm_fl.start(0)");
    rpi_emit_line(gen, "_kx_mec_pwm_fr.start(0)");
    rpi_emit_line(gen, "_kx_mec_pwm_bl.start(0)");
    rpi_emit_line(gen, "_kx_mec_pwm_br.start(0)");
    break;

  case NODE_MECANUM_MOVE:
    rpi_emit_line(gen, "if _kx_mec_fl is not None:");
    gen->indent_level++;
    rpi_indent(gen);
    rpi_emit(gen, "_kf_y = "); rpi_expression(gen, node->data.mecanum_move.y); rpi_emit(gen, "\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kf_x = "); rpi_expression(gen, node->data.mecanum_move.x); rpi_emit(gen, "\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kf_t = "); rpi_expression(gen, node->data.mecanum_move.turn); rpi_emit(gen, "\n");
    rpi_emit_line(gen, "_kx_fl = _kf_y + _kf_x + _kf_t");
    rpi_emit_line(gen, "_kx_fr = _kf_y - _kf_x - _kf_t");
    rpi_emit_line(gen, "_kx_bl = _kf_y - _kf_x + _kf_t");
    rpi_emit_line(gen, "_kx_br = _kf_y + _kf_x - _kf_t");
    rpi_emit_line(gen, "_kx_mec_pwm_fl.ChangeDutyCycle(max(0, min(100, abs(_kx_fl) * 100.0 / 255.0)))");
    rpi_emit_line(gen, "_kx_mec_pwm_fr.ChangeDutyCycle(max(0, min(100, abs(_kx_fr) * 100.0 / 255.0)))");
    rpi_emit_line(gen, "_kx_mec_pwm_bl.ChangeDutyCycle(max(0, min(100, abs(_kx_bl) * 100.0 / 255.0)))");
    rpi_emit_line(gen, "_kx_mec_pwm_br.ChangeDutyCycle(max(0, min(100, abs(_kx_br) * 100.0 / 255.0)))");
    gen->indent_level--;
    break;

  case NODE_MECANUM_STOP:
    rpi_emit_line(gen, "if _kx_mec_fl is not None:");
    gen->indent_level++;
    rpi_emit_line(gen, "_kx_mec_pwm_fl.ChangeDutyCycle(0)");
    rpi_emit_line(gen, "_kx_mec_pwm_fr.ChangeDutyCycle(0)");
    rpi_emit_line(gen, "_kx_mec_pwm_bl.ChangeDutyCycle(0)");
    rpi_emit_line(gen, "_kx_mec_pwm_br.ChangeDutyCycle(0)");
    gen->indent_level--;
    break;
  case NODE_ENCODER_ATTACH:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_enc_a = int(");
    rpi_expression(gen, node->data.encoder_attach.pin_a);
    rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_enc_b = int(");
    rpi_expression(gen, node->data.encoder_attach.pin_b);
    rpi_emit(gen, ")\n");
    rpi_emit_line(gen, "_kx_enc_count = 0");
    rpi_emit_line(gen,
                  "GPIO.setup(_kx_enc_a, GPIO.IN, pull_up_down=GPIO.PUD_UP)");
    rpi_emit_line(gen,
                  "GPIO.setup(_kx_enc_b, GPIO.IN, pull_up_down=GPIO.PUD_UP)");
    rpi_emit_line(gen, "def _kx_enc_cb(channel):");
    gen->indent_level++;
    rpi_emit_line(gen, "global _kx_enc_count");
    rpi_emit_line(gen, "if GPIO.input(_kx_enc_b): _kx_enc_count += 1");
    rpi_emit_line(gen, "else: _kx_enc_count -= 1");
    gen->indent_level--;
    rpi_emit_line(
        gen,
        "GPIO.add_event_detect(_kx_enc_a, GPIO.RISING, callback=_kx_enc_cb)");
    break;
  case NODE_ENCODER_READ:
    rpi_emit(gen, "_kx_enc_count");
    break;
  case NODE_ENCODER_RESET:
    rpi_emit_line(gen, "global _kx_enc_count; _kx_enc_count = 0");
    break;
  case NODE_ESC_ATTACH:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_esc_pin = int(");
    rpi_expression(gen, node->data.esc_attach.pin);
    rpi_emit(gen, ")\n");
    rpi_emit_line(gen, "GPIO.setup(_kx_esc_pin, GPIO.OUT)");
    rpi_emit_line(gen, "_kx_esc_pwm = GPIO.PWM(_kx_esc_pin, 50)");
    rpi_emit_line(gen, "_kx_esc_pwm.start(5) # 1ms pulse (stopped)");
    break;
  case NODE_ESC_THROTTLE:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_throttle_val = ");
    rpi_expression(gen, node->data.unary.child);
    rpi_emit(gen, "\n");
    rpi_emit_line(gen, "_kx_esc_dc = 5.0 + (_kx_throttle_val / 180.0) * 5.0 # "
                       "Map 0-180 to 5-10% DC (1ms-2ms pulse)");
    rpi_emit_line(gen, "_kx_esc_pwm.ChangeDutyCycle(_kx_esc_dc)");
    break;
  case NODE_PID_ATTACH:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_pid_kp = ");
    rpi_expression(gen, node->data.pid_attach.kp);
    rpi_emit(gen, "\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_pid_ki = ");
    rpi_expression(gen, node->data.pid_attach.ki);
    rpi_emit(gen, "\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_pid_kd = ");
    rpi_expression(gen, node->data.pid_attach.kd);
    rpi_emit(gen, "\n");
    rpi_emit_line(gen, "_kx_pid_setpoint = 0.0");
    rpi_emit_line(gen, "_kx_pid_last_err = 0.0");
    rpi_emit_line(gen, "_kx_pid_integral = 0.0");
    rpi_emit_line(gen, "_kx_pid_last_time = time.time()");
    break;
  case NODE_PID_TARGET:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_pid_setpoint = ");
    rpi_expression(gen, node->data.unary.child);
    rpi_emit(gen, "\n");
    break;

  /* Wave 3: Communication Statements */
  case NODE_BLE_ENABLE:
    rpi_indent(gen);
    rpi_emit(gen, "global _kx_ble_msg\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_ble_msg = ''\n");
    rpi_indent(gen);
    rpi_emit(gen, "print('BLE enabled: ', ");
    rpi_expression(gen, node->data.ble_enable.name);
    rpi_emit(gen, ")\n");
    break;
  case NODE_BLE_ADVERTISE:
    rpi_indent(gen);
    rpi_emit(gen, "print('BLE advertise: ', ");
    rpi_expression(gen, node->data.ble_advertise.data);
    rpi_emit(gen, ")\n");
    break;
  case NODE_BLE_SEND:
    rpi_indent(gen);
    rpi_emit(gen, "print('BLE send: ', ");
    rpi_expression(gen, node->data.ble_send.data);
    rpi_emit(gen, ")\n");
    break;
  case NODE_WIFI_CONNECT:
    rpi_indent(gen);
    rpi_emit(gen, "global _kx_wifi_ip\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_wifi_ip = socket.gethostbyname(socket.gethostname())\n");
    break;
  case NODE_MQTT_CONNECT:
    rpi_indent(gen);
    rpi_emit(gen, "global _kx_mqtt_client\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mqtt_client = mqtt.Client()\n");
    rpi_indent(gen);
    rpi_emit(gen, "def _on_message(client, userdata, message):\n");
    rpi_indent(gen);
    rpi_emit(gen, "    global _kx_mqtt_msg\n");
    rpi_indent(gen);
    rpi_emit(gen, "    _kx_mqtt_msg = str(message.payload.decode('utf-8'))\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mqtt_client.on_message = _on_message\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mqtt_client.connect(str(");
    rpi_expression(gen, node->data.mqtt_connect.broker);
    rpi_emit(gen, "), int(");
    rpi_expression(gen, node->data.mqtt_connect.port);
    rpi_emit(gen, "))\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_mqtt_client.loop_start()\n");
    break;
  case NODE_MQTT_SUBSCRIBE:
    rpi_indent(gen);
    rpi_emit(gen, "if _kx_mqtt_client: _kx_mqtt_client.subscribe(str(");
    rpi_expression(gen, node->data.mqtt_subscribe.topic);
    rpi_emit(gen, "))\n");
    break;
  case NODE_MQTT_PUBLISH:
    rpi_indent(gen);
    rpi_emit(gen, "if _kx_mqtt_client: _kx_mqtt_client.publish(str(");
    rpi_expression(gen, node->data.mqtt_publish.topic);
    rpi_emit(gen, "), str(");
    rpi_expression(gen, node->data.mqtt_publish.payload);
    rpi_emit(gen, "))\n");
    break;
  case NODE_HTTP_POST:
    rpi_indent(gen);
    rpi_emit(gen, "requests.post(");
    rpi_expression(gen, node->data.http_post.url);
    rpi_emit(gen, ", data=");
    rpi_expression(gen, node->data.http_post.body);
    rpi_emit(gen, ")\n");
    break;
  case NODE_WS_CONNECT:
    rpi_indent(gen);
    rpi_emit(gen,
             "global _kx_ws_client; _kx_ws_client = websocket.WebSocket()\n");
    rpi_indent(gen);
    rpi_emit(gen, "_kx_ws_client.connect(");
    rpi_expression(gen, node->data.ws_connect.url);
    rpi_emit(gen, ")\n");
    break;
  case NODE_WS_SEND:
    rpi_indent(gen);
    rpi_emit(gen, "if _kx_ws_client: _kx_ws_client.send(str(");
    rpi_expression(gen, node->data.ws_send.data);
    rpi_emit(gen, "))\n");
    break;
  case NODE_WS_CLOSE:
    rpi_indent(gen);
    rpi_emit(gen,
             "if _kx_ws_client: _kx_ws_client.close(); _kx_ws_client = None\n");
    break;

  case NODE_PID_COMPUTE:
    rpi_emit(gen, "_kx_compute_pid(");
    rpi_expression(gen, node->data.pid_compute.current_val);
    rpi_emit(gen, ")\n");
    break;

  case NODE_KALMAN_ATTACH:
    rpi_indent(gen);
    rpi_emit_line(gen, "global _kx_kalman_p, _kx_kalman_x, _kx_kalman_k");
    rpi_indent(gen);
    rpi_emit_line(gen, "_kx_kalman_p = 1.0; _kx_kalman_x = 0.0; _kx_kalman_k = 0.0");
    break;

  case NODE_KALMAN_COMPUTE:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_kalman_update(");
    rpi_expression(gen, node->data.kalman_compute.raw_value);
    rpi_emit(gen, ")\n");
    break;
  case NODE_AI_LOAD:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_ai_interpreter = tflite.Interpreter(model_path=");
    rpi_expression(gen, node->data.ai_load.model_path);
    rpi_emit(gen, ")\n");
    rpi_indent(gen);
    rpi_emit_line(gen, "_kx_ai_interpreter.allocate_tensors()");
    rpi_indent(gen);
    rpi_emit_line(gen, "_kx_ai_input_details = _kx_ai_interpreter.get_input_details()");
    rpi_indent(gen);
    rpi_emit_line(gen, "_kx_ai_output_details = _kx_ai_interpreter.get_output_details()");
    break;

  case NODE_AI_COMPUTE:
    rpi_indent(gen);
    rpi_emit(gen, "_kx_ai_invoke(");
    rpi_expression(gen, node->data.ai_compute.input_array);
    rpi_emit(gen, ")\n");
    break;
  case NODE_TONE:
    // RPi PWM starts by verifying object doesn't exist yet, if not it registers
    // it globally, and changes its frequency using GPIO.ChangeFrequency. Wait,
    // we can simplify this.
    rpi_indent(gen);
    rpi_emit(gen, "if '_pwm_%d' not in globals():\n", node->data.gpio.pin);
    gen->indent_level++;
    rpi_indent(gen);
    rpi_emit(gen, "GPIO.setup(%d, GPIO.OUT)\n", node->data.gpio.pin);
    rpi_indent(gen);
    rpi_emit(gen, "globals()['_pwm_%d'] = GPIO.PWM(%d, 1)\n",
             node->data.gpio.pin, node->data.gpio.pin);
    rpi_indent(gen);
    rpi_emit(gen, "globals()['_pwm_%d'].start(50)\n", node->data.gpio.pin);
    gen->indent_level--;
    rpi_indent(gen);
    rpi_emit(gen, "globals()['_pwm_%d'].ChangeFrequency(",
             node->data.gpio.value);
    rpi_expression(gen, node->data.gpio.value);
    rpi_emit(gen, ")\n");
    break;
  case NODE_NOTONE:
    rpi_indent(gen);
    rpi_emit(gen, "if '_pwm_%d' in globals():\n", node->data.gpio.pin);
    gen->indent_level++;
    rpi_indent(gen);
    rpi_emit(gen, "globals()['_pwm_%d'].stop()\n", node->data.gpio.pin);
    rpi_indent(gen);
    rpi_emit(gen, "del globals()['_pwm_%d']\n", node->data.gpio.pin);
    gen->indent_level--;
    break;

  case NODE_FUNCTION_DEF:
    if (node->data.function_def.is_extern) {
      rpi_emit_line(gen, "# Extern %s function: %s",
                    node->data.function_def.extern_lang,
                    node->data.function_def.name);
      break;
    }
    rpi_indent(gen);
    rpi_emit(gen, "def %s(", node->data.function_def.name);
    for (int i = 0; i < node->data.function_def.param_count; i++) {
      if (i > 0)
        rpi_emit(gen, ", ");
      rpi_emit(gen, "%s", node->data.function_def.param_names[i]);
    }
    rpi_emit(gen, "):\n");
    gen->indent_level++;
    rpi_statement(gen, node->data.function_def.body);
    gen->indent_level--;
    rpi_emit(gen, "\n");
    break;

  /* Wave 4: Advanced Robotics & Storage Statements */
  case NODE_IMU_ATTACH:
    rpi_indent(gen);
    rpi_emit_line(gen, "global _kx_imu");
    rpi_indent(gen);
    rpi_emit_line(
        gen, "try: _kx_imu = adafruit_bno08x.BNO08X_I2C(busio.I2C(board.SCL, "
             "board.SDA))");
    rpi_indent(gen);
    rpi_emit_line(gen, "except: _kx_imu = None");
    break;

  case NODE_GPS_ATTACH:
    rpi_indent(gen);
    rpi_emit_line(gen, "global _kx_gps_serial, _kx_gps");
    rpi_indent(gen);
    rpi_emit(
        gen,
        "try: _kx_gps_serial = serial.Serial('/dev/serial0', baudrate=int(");
    rpi_expression(gen, node->data.gps_attach.baud);
    rpi_emit(gen, "), timeout=1)\n");
    rpi_indent(gen);
    rpi_emit_line(gen, "except: _kx_gps_serial = None");
    break;

  case NODE_LIDAR_ATTACH:
    rpi_indent(gen);
    rpi_emit_line(gen, "global _kx_lidar");
    rpi_indent(gen);
    rpi_emit_line(gen,
                  "try: _kx_lidar = "
                  "adafruit_vl53l0x.VL53L0X(busio.I2C(board.SCL, board.SDA))");
    rpi_indent(gen);
    rpi_emit_line(gen, "except: _kx_lidar = None");
    break;

  case NODE_SD_MOUNT:
    rpi_indent(gen);
    rpi_emit_line(
        gen, "print('RPi runs OS file system natively. SD Mount bypassed.')");
    break;

  case NODE_FILE_OPEN:
    rpi_indent(gen);
    rpi_emit(gen, "global _kx_file; _kx_file = open(");
    rpi_expression(gen, node->data.file_open.filename);
    rpi_emit(gen, ", 'a+')\n");
    break;

  case NODE_FILE_WRITE:
    rpi_indent(gen);
    rpi_emit(gen, "if _kx_file: _kx_file.write(str(");
    rpi_expression(gen, node->data.file_write.data);
    rpi_emit(gen, ") + '\\n')\n");
    break;

  case NODE_FILE_CLOSE:
    rpi_indent(gen);
    rpi_emit_line(gen, "if _kx_file: _kx_file.close(); _kx_file = None");
    break;

  /* Wave 5 Statements */
  case NODE_OLED_ATTACH:
    rpi_indent(gen);
    rpi_emit_line(gen, "global _kx_oled");
    rpi_indent(gen);
    rpi_emit_line(gen, "try: from luma.oled.device import ssd1306; from luma.core.interface.serial import i2c as luma_i2c; from luma.core.render import canvas");
    rpi_indent(gen);
    rpi_emit_line(gen, "except: pass");
    rpi_indent(gen);
    rpi_emit_line(gen, "try: _kx_oled = ssd1306(luma_i2c(port=1, address=0x3C))");
    rpi_indent(gen);
    rpi_emit_line(gen, "except: _kx_oled = None");
    break;
  case NODE_OLED_PRINT:
    rpi_indent(gen);
    rpi_emit(gen, "if _kx_oled:\n");
    rpi_indent(gen);
    rpi_emit(gen, "    with canvas(_kx_oled) as _draw: _draw.text((");
    rpi_expression(gen, node->data.oled_print.x);
    rpi_emit(gen, ", ");
    rpi_expression(gen, node->data.oled_print.y);
    rpi_emit(gen, "), str(");
    rpi_expression(gen, node->data.oled_print.text);
    rpi_emit(gen, "), fill='white')\n");
    break;
  case NODE_OLED_DRAW:
    rpi_indent(gen);
    rpi_emit_line(gen, "# OLED Draw (shape rendering via luma.oled)");
    break;
  case NODE_OLED_SHOW:
    rpi_indent(gen);
    rpi_emit_line(gen, "# OLED show (auto-flushed in luma.oled)");
    break;
  case NODE_OLED_CLEAR:
    rpi_indent(gen);
    rpi_emit_line(gen, "if _kx_oled: _kx_oled.clear()");
    break;
  case NODE_AUDIO_ATTACH:
    rpi_indent(gen);
    rpi_emit_line(gen, "# Audio attached");
    break;
  case NODE_PLAY_FREQ:
    rpi_indent(gen);
    rpi_emit(gen, "import subprocess; subprocess.Popen(['speaker-test', '-t', 'sine', '-f', str(int(");
    rpi_expression(gen, node->data.play_freq.frequency);
    rpi_emit(gen, ")), '-l', '1'])\n");
    break;
  case NODE_PLAY_SOUND:
    rpi_indent(gen);
    rpi_emit(gen, "import subprocess; subprocess.Popen(['aplay', str(");
    rpi_expression(gen, node->data.play_sound.name);
    rpi_emit(gen, ")])\n");
    break;
  case NODE_SET_VOLUME:
    rpi_indent(gen);
    rpi_emit(gen, "import subprocess; subprocess.call(['amixer', 'set', 'Master', str(int(");
    rpi_expression(gen, node->data.set_volume.level);
    rpi_emit(gen, ")) + '%%'])\n");
    break;
  case NODE_CAM_ATTACH:
    rpi_indent(gen);
    rpi_emit_line(gen, "global _kx_husky, _kx_husky_blocks");
    rpi_indent(gen);
    rpi_emit_line(gen, "try: from huskylensPythonLibrary import HuskyLensLibrary; _kx_husky = HuskyLensLibrary('I2C')");
    rpi_indent(gen);
    rpi_emit_line(gen, "except: _kx_husky = None");
    break;

  default:
    break;
  }
}

// ============================================================
// PROGRAM ENTRY POINT
// ============================================================

void codegen_generate_rpi(CodeGen *gen, ASTNode *program) {
  if (!program || program->type != NODE_PROGRAM)
    return;

  // Python header and imports
  rpi_emit_line(gen, "#!/usr/bin/env python3");
  rpi_emit_line(gen, "# Generated by Kinetrix Compiler (Target: Raspberry Pi)");
  rpi_emit_line(gen, "# Run with: python3 robot.py");
  rpi_emit_line(gen, "# Requirements: pip install RPi.GPIO Adafruit-MCP3008\n");
  rpi_emit_line(gen, "import RPi.GPIO as GPIO");
  rpi_emit_line(gen, "import time");
  rpi_emit_line(gen, "import random");
  rpi_emit_line(gen, "import math");
  rpi_emit_line(gen, "import busio");
  rpi_emit_line(gen, "import digitalio");
  rpi_emit_line(gen, "import board");
  rpi_emit_line(gen, "import adafruit_mcp3xxx.mcp3008 as MCP");
  rpi_emit_line(gen, "from adafruit_mcp3xxx.analog_in import AnalogIn");

  /* Wave 3 Imports */
  rpi_emit_line(gen, "import socket");
  rpi_emit_line(gen, "try:");
  rpi_emit_line(gen, "    import requests");
  rpi_emit_line(gen, "    import paho.mqtt.client as mqtt");
  rpi_emit_line(gen, "    import websocket");
  rpi_emit_line(gen, "except ImportError: pass\n");
  rpi_emit_line(gen, "# GPIO setup");
  rpi_emit_line(gen, "GPIO.setmode(GPIO.BCM)");
  rpi_emit_line(gen, "GPIO.setwarnings(False)");
  rpi_emit_line(gen, "for _pin in range(2, 28): GPIO.setup(_pin, GPIO.OUT)\n");
  rpi_emit_line(gen, "# ADC setup (MCP3008 via SPI)");
  rpi_emit_line(gen, "try:");
  rpi_emit_line(gen, "    _spi = busio.SPI(clock=board.SCK, MISO=board.MISO, "
                     "MOSI=board.MOSI)");
  rpi_emit_line(gen, "    _cs  = digitalio.DigitalInOut(board.CE0)");
  rpi_emit_line(gen, "    mcp  = MCP.MCP3008(_spi, _cs)");
  rpi_emit_line(gen, "except: mcp = None  # No ADC connected\n");

  rpi_emit_line(gen, "# Wave 2 Globals");
  rpi_emit_line(gen, "_kx_stepper_step = -1");
  rpi_emit_line(gen, "_kx_stepper_dir = -1");
  rpi_emit_line(gen, "_kx_stepper_speed = 100");
  rpi_emit_line(gen, "_kx_motor_en = -1");
  rpi_emit_line(gen, "_kx_motor_fwd = -1");
  rpi_emit_line(gen, "_kx_motor_rev = -1");
  rpi_emit_line(gen, "_kx_enc_a = -1");
  rpi_emit_line(gen, "_kx_enc_b = -1");
  rpi_emit_line(gen, "_kx_enc_count = 0");
  rpi_emit_line(gen, "_kx_esc_pin = -1");
  rpi_emit_line(gen, "_kx_pid_kp = 0");
  rpi_emit_line(gen, "_kx_pid_ki = 0");
  rpi_emit_line(gen, "_kx_pid_kd = 0");
  rpi_emit_line(gen, "_kx_pid_setpoint = 0");
  rpi_emit_line(gen, "_kx_pid_last_err = 0");
  rpi_emit_line(gen, "_kx_pid_integral = 0");
  rpi_emit_line(gen, "_kx_pid_last_time = time.time()");

  /* Wave 6 AI Include */
  rpi_emit_line(gen, "try:");
  rpi_emit_line(gen, "    import tflite_runtime.interpreter as tflite");
  rpi_emit_line(gen, "except ImportError:");
  rpi_emit_line(gen, "    import tensorflow.lite as tflite\n");
  rpi_emit_line(gen, "import numpy as np\n");

  /* Wave 3 Globals */
  rpi_emit_line(gen, "_kx_mqtt_client = None");
  rpi_emit_line(gen, "_kx_mqtt_msg = ''");
  rpi_emit_line(gen, "_kx_ws_client = None");
  rpi_emit_line(gen, "_kx_ws_msg = ''");
  rpi_emit_line(gen, "_kx_ble_msg = ''");
  rpi_emit_line(gen, "_kx_wifi_ip = ''\n");

  /* Wave 4 Modules & Globals */
  rpi_emit_line(gen, "try:");
  rpi_emit_line(gen, "    import adafruit_bno08x");
  rpi_emit_line(gen, "    from adafruit_bno08x.i2c import BNO08X_I2C");
  rpi_emit_line(gen, "    import serial");
  rpi_emit_line(gen, "    import pynmea2");
  rpi_emit_line(gen, "    import adafruit_vl53l0x");
  rpi_emit_line(gen, "except ImportError: pass");
  rpi_emit_line(gen, "_kx_imu = None");
  rpi_emit_line(gen, "_kx_gps_serial = None");
  rpi_emit_line(gen, "_kx_gps = None");
  rpi_emit_line(gen, "_kx_lidar = None");
  rpi_emit_line(gen, "_kx_file = None\n");

  /* Wave 5 Globals */
  rpi_emit_line(gen, "_kx_oled = None");
  rpi_emit_line(gen, "_kx_husky = None");
  rpi_emit_line(gen, "_kx_husky_blocks = []\n");

  /* Wave 6 Globals */
  rpi_emit_line(gen, "_kx_mec_fl = None; _kx_mec_fr = None; _kx_mec_bl = None; _kx_mec_br = None");
  rpi_emit_line(gen, "_kx_mec_pwm_fl = None; _kx_mec_pwm_fr = None; _kx_mec_pwm_bl = None; _kx_mec_pwm_br = None\n");
  rpi_emit_line(gen, "_kx_kalman_q = 0.01; _kx_kalman_r = 0.1");
  rpi_emit_line(gen, "_kx_kalman_x = 0.0; _kx_kalman_p = 1.0; _kx_kalman_k = 0.0\n");

  rpi_emit_line(gen, "def _kx_kalman_update(mea):");
  rpi_emit_line(gen, "    global _kx_kalman_q, _kx_kalman_r, _kx_kalman_x, _kx_kalman_p, _kx_kalman_k");
  rpi_emit_line(gen, "    _kx_kalman_p = _kx_kalman_p + _kx_kalman_q");
  rpi_emit_line(gen, "    _kx_kalman_k = _kx_kalman_p / (_kx_kalman_p + _kx_kalman_r)");
  rpi_emit_line(gen, "    _kx_kalman_x = _kx_kalman_x + _kx_kalman_k * (mea - _kx_kalman_x)");
  rpi_emit_line(gen, "    _kx_kalman_p = (1.0 - _kx_kalman_k) * _kx_kalman_p");
  rpi_emit_line(gen, "    return _kx_kalman_x\n");

  rpi_emit_line(gen, "_kx_ai_interpreter = None; _kx_ai_input_details = None; _kx_ai_output_details = None");
  rpi_emit_line(gen, "def _kx_ai_invoke(input_data):");
  rpi_emit_line(gen, "    global _kx_ai_interpreter, _kx_ai_input_details, _kx_ai_output_details");
  rpi_emit_line(gen, "    if not _kx_ai_interpreter: return 0.0");
  rpi_emit_line(gen, "    input_data = np.array([input_data], dtype=np.float32) if isinstance(input_data, (int, float)) else np.array(input_data, dtype=np.float32)");
  rpi_emit_line(gen, "    _kx_ai_interpreter.set_tensor(_kx_ai_input_details[0]['index'], input_data)");
  rpi_emit_line(gen, "    _kx_ai_interpreter.invoke()");
  rpi_emit_line(gen, "    return _kx_ai_interpreter.get_tensor(_kx_ai_output_details[0]['index'])[0][0]\n");

  rpi_emit_line(gen, "def _kx_compute_pid(current_val):");
  rpi_emit_line(
      gen,
      "    global _kx_pid_kp, _kx_pid_ki, _kx_pid_kd, _kx_pid_setpoint, \\");
  rpi_emit_line(
      gen, "           _kx_pid_last_err, _kx_pid_integral, _kx_pid_last_time");
  rpi_emit_line(gen, "    now = time.time()");
  rpi_emit_line(gen, "    dt = now - _kx_pid_last_time");
  rpi_emit_line(gen, "    if dt <= 0.0: dt = 0.001");
  rpi_emit_line(gen, "    err = _kx_pid_setpoint - current_val");
  rpi_emit_line(gen, "    _kx_pid_integral += err * dt");
  rpi_emit_line(gen, "    deriv = (err - _kx_pid_last_err) / dt");
  rpi_emit_line(gen, "    out = (_kx_pid_kp * err) + (_kx_pid_ki * "
                     "_kx_pid_integral) + (_kx_pid_kd * deriv)");
  rpi_emit_line(gen, "    _kx_pid_last_err = err");
  rpi_emit_line(gen, "    _kx_pid_last_time = now");
  rpi_emit_line(gen, "    return out\n");

  rpi_emit_line(gen, "def _i2c_read(addr, reg):");
  rpi_emit_line(gen, "    if '_i2c' in globals():");
  rpi_emit_line(gen, "        while not _i2c.try_lock(): pass");
  rpi_emit_line(gen, "        res = bytearray(1)");
  rpi_emit_line(
      gen, "        try: _i2c.writeto_then_readfrom(addr, bytes([reg]), res)");
  rpi_emit_line(gen, "        except: pass");
  rpi_emit_line(gen, "        _i2c.unlock()");
  rpi_emit_line(gen, "        return res[0]");
  rpi_emit_line(gen, "    return 0\n");
  rpi_emit_line(gen, "def _i2c_write(addr, val):");
  rpi_emit_line(gen, "    if '_i2c' in globals():");
  rpi_emit_line(gen, "        while not _i2c.try_lock(): pass");
  rpi_emit_line(gen, "        try: _i2c.writeto(addr, bytes([val]))");
  rpi_emit_line(gen, "        except: pass");
  rpi_emit_line(gen, "        _i2c.unlock()\n");
  rpi_emit_line(gen, "def _spi_transfer(val):");
  rpi_emit_line(gen, "    if '_spi' in globals() and 'mcp' not in globals():");
  rpi_emit_line(gen, "        while not _spi.try_lock(): pass");
  rpi_emit_line(gen, "        res = bytearray(1)");
  rpi_emit_line(gen, "        try: _spi.write_readinto(bytes([val]), res)");
  rpi_emit_line(gen, "        except: pass");
  rpi_emit_line(gen, "        _spi.unlock()");
  rpi_emit_line(gen, "        return res[0]");
  rpi_emit_line(gen, "    return 0");
  rpi_emit_line(gen, "");
  rpi_emit_line(gen, "def time_pulse_us(pin_num, timeout_us=30000):");
  rpi_emit_line(gen,
                "    return 0  # Not strictly supported on RPi user-space");
  rpi_emit_line(gen, "");
  rpi_emit_line(gen, "def _radio_send(peer, data):");
  rpi_emit_line(gen,
                "    pass # Placeholder for RPi NRF24 or LoRa transmission");
  rpi_emit_line(gen, "");
  rpi_emit_line(gen, "def _radio_available():");
  rpi_emit_line(gen, "    return 0");
  rpi_emit_line(gen, "");
  rpi_emit_line(gen, "def _radio_read():");
  rpi_emit_line(gen, "    return 0\n");

  // Function definitions
  if (program->data.program.main_block &&
      program->data.program.main_block->type == NODE_BLOCK) {
    ASTNode *block = program->data.program.main_block;
    // Hoist global vars, arrays, buffers
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type == NODE_VAR_DECL || stmt->type == NODE_ARRAY_DECL ||
          stmt->type == NODE_BUFFER_DECL || stmt->type == NODE_SHARED_DECL) {
        rpi_statement(gen, stmt);
      }
    }
    // Hoist functions
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *s = block->data.block.statements[i];
      if (s && s->type == NODE_FUNCTION_DEF) {
        rpi_statement(gen, s);
      }
    }
    // Hoist ISRs
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type == NODE_INTERRUPT_PIN) {
        rpi_emit_line(gen,
                      "def _isr_pin%d():", stmt->data.interrupt_pin.pin_number);
        gen->indent_level++;
        rpi_statement(gen, stmt->data.interrupt_pin.body);
        gen->indent_level--;
        rpi_emit_line(gen, "");
      } else if (stmt->type == NODE_INTERRUPT_TIMER) {
        rpi_emit_line(gen, "def _isr_timer():");
        gen->indent_level++;
        rpi_statement(gen, stmt->data.interrupt_timer.body);
        gen->indent_level--;
        rpi_emit_line(gen, "");
      }
    }
    // Hoist tasks
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type == NODE_TASK_DEF) {
        rpi_emit_line(gen, "def _task_func_%s():", stmt->data.task_def.name);
        gen->indent_level++;
        rpi_emit_line(gen, "while True:");
        gen->indent_level++;
        rpi_statement(gen, stmt->data.task_def.body);
        gen->indent_level--;
        gen->indent_level--;
        rpi_emit_line(gen, "");
      }
    }

    // Main execution in try/finally for GPIO cleanup
    rpi_emit_line(gen, "try:");
    gen->indent_level++;
    rpi_emit_line(gen, "global _i2c, _spi, _uart");
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type != NODE_FUNCTION_DEF && stmt->type != NODE_TASK_DEF &&
          stmt->type != NODE_VAR_DECL && stmt->type != NODE_ARRAY_DECL &&
          stmt->type != NODE_BUFFER_DECL && stmt->type != NODE_SHARED_DECL &&
          stmt->type != NODE_INTERRUPT_PIN &&
          stmt->type != NODE_INTERRUPT_TIMER) {
        rpi_statement(gen, stmt);
      }
    }
    gen->indent_level--;
  } else if (program->data.program.main_block) {
    rpi_emit_line(gen, "try:");
    gen->indent_level++;
    rpi_statement(gen, program->data.program.main_block);
    gen->indent_level--;
  } else {
    rpi_emit_line(gen, "try:");
    gen->indent_level++;
    rpi_emit_line(gen, "pass");
    gen->indent_level--;
  }

  rpi_emit_line(gen, "except KeyboardInterrupt:");
  rpi_emit_line(gen, "    print(\"\\nStopped by user\")");
  rpi_emit_line(gen, "finally:");
  rpi_emit_line(gen, "    GPIO.cleanup()");
  rpi_emit_line(gen, "    print(\"GPIO cleaned up\")");
}
