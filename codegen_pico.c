/* Kinetrix Raspberry Pi Pico (MicroPython) Code Generator */

#include "ast.h"
#include "codegen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pico_indent(CodeGen *gen) {
  for (int i = 0; i < gen->indent_level; i++)
    fprintf(gen->output, "    ");
}
static void pico_emit(CodeGen *gen, const char *fmt, ...) {
  va_list a;
  va_start(a, fmt);
  vfprintf(gen->output, fmt, a);
  va_end(a);
}
static void pico_emit_line(CodeGen *gen, const char *fmt, ...) {
  pico_indent(gen);
  va_list a;
  va_start(a, fmt);
  vfprintf(gen->output, fmt, a);
  va_end(a);
  fprintf(gen->output, "\n");
}

static void pico_expr(CodeGen *gen, ASTNode *node);
static void pico_stmt(CodeGen *gen, ASTNode *node);

static void pico_expr(CodeGen *gen, ASTNode *node) {
  if (!node)
    return;
  switch (node->type) {
  case NODE_NUMBER:
    pico_emit(gen, "%g", node->data.number.value);
    break;
  case NODE_STRING: {
    /* Escape backslashes and quotes for MicroPython string literal */
    pico_emit(gen, "\"");
    for (const char *p = node->data.string.value; *p; p++) {
      if (*p == '\\') pico_emit(gen, "\\\\");
      else if (*p == '"') pico_emit(gen, "\\\"");
      else if (*p == '\n') pico_emit(gen, "\\n");
      else if (*p == '\r') pico_emit(gen, "\\r");
      else if (*p == '\t') pico_emit(gen, "\\t");
      else fputc(*p, gen->output);
    }
    pico_emit(gen, "\"");
    break;
  }
  case NODE_BOOL:
    pico_emit(gen, "%s", node->data.boolean.value ? "True" : "False");
    break;
  case NODE_IDENTIFIER:
    pico_emit(gen, "%s", node->data.identifier.name);
    break;
  case NODE_BINARY_OP: {
    const char *op = "+";
    switch (node->data.binary_op.op) {
    case OP_ADD:
      op = "+";
      break;
    case OP_SUB:
      op = "-";
      break;
    case OP_MUL:
      op = "*";
      break;
    case OP_DIV:
      op = "/";
      break;
    case OP_MOD:
      op = "%";
      break;
    case OP_EQ:
      op = "==";
      break;
    case OP_NEQ:
      op = "!=";
      break;
    case OP_LT:
      op = "<";
      break;
    case OP_GT:
      op = ">";
      break;
    case OP_LTE:
      op = "<=";
      break;
    case OP_GTE:
      op = ">=";
      break;
    case OP_AND:
      op = "and";
      break;
    case OP_OR:
      op = "or";
      break;
    default:
      break;
    }
    if (node->data.binary_op.op == OP_ADD &&
        ((node->data.binary_op.left->value_type &&
          node->data.binary_op.left->value_type->kind == TYPE_STRING) ||
         (node->data.binary_op.right->value_type &&
          node->data.binary_op.right->value_type->kind == TYPE_STRING))) {
      pico_emit(gen, "(str(");
      pico_expr(gen, node->data.binary_op.left);
      pico_emit(gen, ") + str(");
      pico_expr(gen, node->data.binary_op.right);
      pico_emit(gen, "))");
    } else if (node->data.binary_op.op == OP_DIV) {
      pico_emit(gen, "(0 if (");
      pico_expr(gen, node->data.binary_op.right);
      pico_emit(gen, ") == 0 else (");
      pico_expr(gen, node->data.binary_op.left);
      pico_emit(gen, ") / (");
      pico_expr(gen, node->data.binary_op.right);
      pico_emit(gen, "))");
    } else {
      pico_emit(gen, "(");
      pico_expr(gen, node->data.binary_op.left);
      pico_emit(gen, " %s ", op);
      pico_expr(gen, node->data.binary_op.right);
      pico_emit(gen, ")");
    }
    break;
  }
  case NODE_UNARY_OP:
    pico_emit(gen, node->data.unary_op.op == OP_NOT ? "not (" : "-(");
    pico_expr(gen, node->data.unary_op.operand);
    pico_emit(gen, ")");
    break;
  case NODE_CALL: {
    const char *nm = node->data.call.name;
    if (strcmp(nm, "map") == 0 && node->data.call.arg_count == 5) {
      pico_emit(gen, "int((");
      pico_expr(gen, node->data.call.args[0]);
      pico_emit(gen, " - ");
      pico_expr(gen, node->data.call.args[1]);
      pico_emit(gen, ") * (");
      pico_expr(gen, node->data.call.args[4]);
      pico_emit(gen, " - ");
      pico_expr(gen, node->data.call.args[3]);
      pico_emit(gen, ") // (");
      pico_expr(gen, node->data.call.args[2]);
      pico_emit(gen, " - ");
      pico_expr(gen, node->data.call.args[1]);
      pico_emit(gen, ") + ");
      pico_expr(gen, node->data.call.args[3]);
      pico_emit(gen, ")");
    } else if (strcmp(nm, "constrain") == 0) {
      pico_emit(gen, "max(");
      pico_expr(gen, node->data.call.args[1]);
      pico_emit(gen, ", min(");
      pico_expr(gen, node->data.call.args[2]);
      pico_emit(gen, ", ");
      pico_expr(gen, node->data.call.args[0]);
      pico_emit(gen, "))");
    } else if (strcmp(nm, "delayMicroseconds") == 0) {
      pico_emit(gen, "utime.sleep_us(int(");
      pico_expr(gen, node->data.call.args[0]);
      pico_emit(gen, "))");
    } else {
      pico_emit(gen, "%s(", nm);
      for (int i = 0; i < node->data.call.arg_count; i++) {
        if (i > 0)
          pico_emit(gen, ", ");
        pico_expr(gen, node->data.call.args[i]);
      }
      pico_emit(gen, ")");
    }
    break;
  }
  case NODE_ANALOG_READ:
    pico_emit(gen, "_safe_adc(");
    pico_expr(gen, node->data.gpio.pin);
    pico_emit(gen, ")");
    break;
  case NODE_GPIO_READ:
    pico_emit(gen, "Pin(");
    pico_expr(gen, node->data.gpio.pin);
    pico_emit(gen, ", Pin.IN).value()");
    break;
  case NODE_ARRAY_ACCESS:
    pico_expr(gen, node->data.array_access.array);
    pico_emit(gen, "[int(");
    pico_expr(gen, node->data.array_access.index);
    pico_emit(gen, ")]");
    break;
  case NODE_CAST:
    pico_emit(gen, "%s(",
              strcmp(type_to_ctype(node->data.cast_op.target_type), "float") ==
                      0
                  ? "float"
                  : "int");
    pico_expr(gen, node->data.cast_op.operand);
    pico_emit(gen, ")");
    break;
  case NODE_STRUCT_ACCESS:
    pico_expr(gen, node->data.struct_access.object);
    pico_emit(gen, "['%s']", node->data.struct_access.member);
    break;
  case NODE_I2C_READ:
    pico_emit(gen, "(_i2c.readfrom_mem(");
    pico_expr(gen, node->data.i2c.address);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.i2c.data);
    pico_emit(gen, ", 1)[0] if '_i2c' in globals() else 0)");
    break;
  case NODE_I2C_DEVICE_READ:
    pico_emit(gen, "(_i2c.readfrom_mem(");
    pico_expr(gen, node->data.i2c_device_read.device_addr);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.i2c_device_read.reg_addr);
    pico_emit(gen, ", 1)[0] if '_i2c' in globals() else 0)");
    break;
  case NODE_SERIAL_RECV:
    pico_emit(gen, "(_uart.read(1) if '_uart' in globals() else 0)");
    break;
  case NODE_SPI_TRANSFER:
    pico_emit(gen, "(_kx_spi.read(1, ");
    pico_expr(gen, node->data.spi_transfer.data);
    pico_emit(gen, ")[0])");
    break;

  case NODE_ENCODER_READ:
    pico_emit(gen,
              "(_kx_encoder_pos if '_kx_encoder_pos' in globals() else 0)");
    break;

  /* Wave 3: Communication Expressions */
  case NODE_BLE_RECEIVE:
    pico_emit(gen, "_kx_ble_msg");
    break;
  case NODE_WIFI_IP:
    pico_emit(gen, "_kx_wifi_ip");
    break;
  case NODE_MQTT_READ:
    pico_emit(gen, "_kx_mqtt_msg");
    break;
  case NODE_HTTP_GET:
    pico_emit(gen, "(urequests.get(");
    pico_expr(gen, node->data.unary.child);
    pico_emit(gen, ").text if 'urequests' in globals() else '')");
    break;
  case NODE_WS_RECEIVE:
    pico_emit(gen, "_kx_ws_msg");
    break;

  case NODE_PID_COMPUTE:
    pico_emit(gen, "_kx_compute_pid(");
    pico_expr(gen, node->data.pid_compute.current_val);
    pico_emit(gen, ")");
    break;
  case NODE_KALMAN_COMPUTE:
    pico_emit(gen, "_kx_kalman_update(");
    pico_expr(gen, node->data.kalman_compute.raw_value);
    pico_emit(gen, ")");
    break;
  case NODE_AI_COMPUTE:
    pico_emit(gen, "_kx_ai_invoke(");
    pico_expr(gen, node->data.ai_compute.input_array);
    pico_emit(gen, ")");
    break;
  case NODE_PATH_COMPUTE:
    pico_emit(gen, "_kx_path_compute(");
    pico_expr(gen, node->data.path_compute.from_x);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.path_compute.from_y);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.path_compute.to_x);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.path_compute.to_y);
    pico_emit(gen, ")");
    break;
  case NODE_PULSE_READ:
    pico_emit(gen, "machine.time_pulse_us(Pin(");
    pico_expr(gen, node->data.gpio.pin);
    pico_emit(gen, ", Pin.IN), 1, 30000)");
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
    pico_emit(gen, "%s(", fn);
    pico_expr(gen, node->data.math_func.arg1);
    if (node->data.math_func.arg2) {
      pico_emit(gen, ", ");
      pico_expr(gen, node->data.math_func.arg2);
    }
    pico_emit(gen, ")");
    break;
  }
  case NODE_RADIO_AVAILABLE:
    pico_emit(gen, "_radio_available()");
    break;
  case NODE_DEVICE_WRITE:
  case NODE_STRUCT_DEF:
  case NODE_I2C_DEVICE_WRITE:
  case NODE_I2C_DEVICE_READ_ARRAY:
    break;
  /* Wave 4: Advanced Robotics & Storage Expressions */
  case NODE_IMU_READ_X:
    pico_emit(gen, "(float(_kx_imu.acceleration[0]) if _kx_imu else 0.0)");
    break;
  case NODE_IMU_READ_Y:
    pico_emit(gen, "(float(_kx_imu.acceleration[1]) if _kx_imu else 0.0)");
    break;
  case NODE_IMU_READ_Z:
    pico_emit(gen, "(float(_kx_imu.acceleration[2]) if _kx_imu else 0.0)");
    break;
  case NODE_IMU_ORIENT:
    pico_emit(gen, "(float(_kx_imu.euler[0]) if _kx_imu else 0.0)");
    break;
  case NODE_GPS_READ_LAT:
    pico_emit(gen, "(float(_kx_gps.latitude) if _kx_gps else 0.0)");
    break;
  case NODE_GPS_READ_LON:
    pico_emit(gen, "(float(_kx_gps.longitude) if _kx_gps else 0.0)");
    break;
  case NODE_GPS_READ_ALT:
    pico_emit(gen, "(float(_kx_gps.altitude) if _kx_gps else 0.0)");
    break;
  case NODE_GPS_READ_SPD:
    pico_emit(gen, "(float(_kx_gps.spd_over_grnd) if _kx_gps else 0.0)");
    break;
  case NODE_LIDAR_READ:
    pico_emit(gen, "(float(_kx_lidar.read()) if _kx_lidar else 0.0)");
    break;
  case NODE_FILE_READ:
    pico_emit(gen, "(str(_kx_file.read()) if _kx_file else \"\")");
    break;

  /* Wave 5 Expressions */
  case NODE_CAM_DETECT: pico_emit(gen, "(1 if _kx_husky else 0)"); break;
  case NODE_CAM_OBJ_X: pico_emit(gen, "(float(_kx_husky_x) if _kx_husky else 0.0)"); break;
  case NODE_CAM_OBJ_Y: pico_emit(gen, "(float(_kx_husky_y) if _kx_husky else 0.0)"); break;

  default:
    pico_emit(gen, "0");
    break;
  }
}

static void pico_stmt(CodeGen *gen, ASTNode *node) {
  if (!node)
    return;
  switch (node->type) {
  case NODE_VAR_DECL:
    pico_indent(gen);
    pico_emit(gen, "%s = ", node->data.var_decl.name);
    if (node->data.var_decl.initializer)
      pico_expr(gen, node->data.var_decl.initializer);
    else
      pico_emit(gen, "0");
    pico_emit(gen, "\n");
    break;
  case NODE_ARRAY_DECL:
    pico_indent(gen);
    pico_emit(gen, "%s = [0] * %d\n", node->data.array_decl.name,
              node->data.array_decl.size);
    break;
  case NODE_BUFFER_DECL:
    pico_indent(gen);
    pico_emit(gen, "%s = []\n", node->data.array_decl.name);
    pico_indent(gen);
    pico_emit(gen, "%s_size = %d\n", node->data.array_decl.name,
              node->data.array_decl.size);
    break;
  case NODE_SHARED_DECL:
    pico_indent(gen);
    pico_emit(gen, "%s = ", node->data.var_decl.name);
    if (node->data.var_decl.initializer)
      pico_expr(gen, node->data.var_decl.initializer);
    else
      pico_emit(gen, "0");
    pico_emit(gen, "\n");
    break;
  case NODE_ASSIGNMENT:
    pico_indent(gen);
    pico_expr(gen, node->data.assignment.target);
    pico_emit(gen, " = ");
    pico_expr(gen, node->data.assignment.value);
    pico_emit(gen, "\n");
    break;
  case NODE_IF:
    pico_indent(gen);
    pico_emit(gen, "if ");
    pico_expr(gen, node->data.if_stmt.condition);
    pico_emit(gen, ":\n");
    gen->indent_level++;
    pico_stmt(gen, node->data.if_stmt.then_block);
    gen->indent_level--;
    if (node->data.if_stmt.else_block) {
      ASTNode *else_b = node->data.if_stmt.else_block;
      if (else_b->type == NODE_BLOCK &&
          else_b->data.block.statement_count == 1 &&
          else_b->data.block.statements[0]->type == NODE_IF) {
        ASTNode *elif = else_b->data.block.statements[0];
        pico_indent(gen);
        pico_emit(gen, "elif ");
        pico_expr(gen, elif->data.if_stmt.condition);
        pico_emit(gen, ":\n");
        gen->indent_level++;
        pico_stmt(gen, elif->data.if_stmt.then_block);
        gen->indent_level--;
        if (elif->data.if_stmt.else_block) {
          pico_emit_line(gen, "else:\n");
          gen->indent_level++;
          pico_stmt(gen, elif->data.if_stmt.else_block);
          gen->indent_level--;
        }
      } else {
        pico_emit_line(gen, "else:\n");
        gen->indent_level++;
        pico_stmt(gen, else_b);
        gen->indent_level--;
      }
    }
    break;
  case NODE_WHILE:
    pico_indent(gen);
    pico_emit(gen, "while ");
    pico_expr(gen, node->data.while_loop.condition);
    pico_emit(gen, ":\n");
    gen->indent_level++;
    pico_stmt(gen, node->data.while_loop.body);
    gen->indent_level--;
    pico_emit(gen, "\n");
    break;
  case NODE_REPEAT: {
    int id = gen->loop_counter++;
    pico_indent(gen);
    pico_emit(gen, "for _i%d in range(int(", id);
    pico_expr(gen, node->data.repeat_loop.count);
    pico_emit(gen, ")):\n");
    gen->indent_level++;
    pico_stmt(gen, node->data.repeat_loop.body);
    gen->indent_level--;
    break;
  }
  case NODE_FOR: {
    int loop_id = gen->loop_counter++;
    pico_indent(gen);
    pico_emit(gen, "_start_%d = int(", loop_id);
    pico_expr(gen, node->data.for_loop.start_expr);
    pico_emit(gen, ")\n");
    pico_indent(gen);
    pico_emit(gen, "_end_%d = int(", loop_id);
    pico_expr(gen, node->data.for_loop.end_expr);
    pico_emit(gen, ")\n");
    if (node->data.for_loop.step_expr) {
      pico_indent(gen);
      pico_emit(gen, "_step_%d = int(", loop_id);
      pico_expr(gen, node->data.for_loop.step_expr);
      pico_emit(gen, ")\n");
    } else {
      pico_indent(gen);
      pico_emit(gen, "_step_%d = 1 if _start_%d <= _end_%d else -1\n", loop_id,
                loop_id, loop_id);
    }
    pico_indent(gen);
    pico_emit(gen,
              "for %s in range(_start_%d, _end_%d + (1 if _step_%d > 0 else "
              "-1), _step_%d):\n",
              node->data.for_loop.var_name, loop_id, loop_id, loop_id, loop_id);
    gen->indent_level++;
    pico_stmt(gen, node->data.for_loop.body);
    gen->indent_level--;
    break;
  }
  case NODE_FOREVER:
    pico_indent(gen);
    pico_emit(gen, "while True:\n");
    gen->indent_level++;
    pico_stmt(gen, node->data.forever_loop.body);
    gen->indent_level--;
    break;
  case NODE_BLOCK:
    for (int i = 0; i < node->data.block.statement_count; i++)
      pico_stmt(gen, node->data.block.statements[i]);
    break;
  case NODE_RETURN:
    pico_indent(gen);
    pico_emit(gen, "return");
    if (node->data.return_stmt.value) {
      pico_emit(gen, " ");
      pico_expr(gen, node->data.return_stmt.value);
    }
    pico_emit(gen, "\n");
    break;
  case NODE_BREAK:
    pico_emit_line(gen, "break\n");
    break;
  case NODE_CONTINUE:
    pico_emit_line(gen, "continue\n");
    break;
  case NODE_GPIO_WRITE:
    pico_indent(gen);
    pico_emit(gen, "Pin(");
    pico_expr(gen, node->data.gpio.pin);
    pico_emit(gen, ", Pin.OUT).value(");
    pico_expr(gen, node->data.gpio.value);
    pico_emit(gen, ")\n");
    break;
  case NODE_ANALOG_WRITE:
    pico_indent(gen);
    pico_emit(gen, "_pwm = PWM(Pin(");
    pico_expr(gen, node->data.gpio.pin);
    pico_emit(gen, ")); _pwm.duty_u16(int(");
    pico_expr(gen, node->data.gpio.value);
    pico_emit(gen, " * 257))\n");
    break;
  case NODE_WAIT:
    pico_indent(gen);
    pico_emit(gen, "utime.sleep_ms(int(");
    pico_expr(gen, node->data.unary.child);
    pico_emit(gen, "))\n");
    break;
  case NODE_PRINT:
    pico_indent(gen);
    pico_emit(gen, "print(");
    pico_expr(gen, node->data.unary.child);
    pico_emit(gen, ", end='')\n");
    break;

  case NODE_PRINTLN:
    pico_indent(gen);
    pico_emit(gen, "print(");
    pico_expr(gen, node->data.unary.child);
    pico_emit(gen, ")\n");
    break;
  case NODE_CALL:
    pico_indent(gen);
    pico_expr(gen, node);
    pico_emit(gen, "\n");
    break;
  case NODE_BUFFER_PUSH:
    pico_indent(gen);
    pico_emit(gen, "%s.append(", node->data.buffer_push.buffer_name);
    pico_expr(gen, node->data.buffer_push.value);
    pico_emit(gen, ")\n");
    pico_indent(gen);
    pico_emit(gen, "if len(%s) > %s_size: %s.pop(0)\n",
              node->data.buffer_push.buffer_name,
              node->data.buffer_push.buffer_name,
              node->data.buffer_push.buffer_name);
    break;
  case NODE_STRUCT_DEF:
    break;
  case NODE_STRUCT_INSTANCE:
    pico_indent(gen);
    pico_emit(gen, "%s = {}\n", node->data.struct_instance.var_name);
    break;
  case NODE_INTERRUPT_PIN:
    pico_indent(gen);
    pico_emit(gen, "Pin(%d, Pin.IN).irq(trigger=Pin.IRQ_",
              node->data.interrupt_pin.pin_number);
    if (node->data.interrupt_pin.mode == INT_MODE_RISING)
      pico_emit(gen, "RISING");
    else if (node->data.interrupt_pin.mode == INT_MODE_FALLING)
      pico_emit(gen, "FALLING");
    else
      pico_emit(gen, "RISING | Pin.IRQ_FALLING");
    pico_emit(gen, ", handler=lambda p: _isr_pin%d())\n",
              node->data.interrupt_pin.pin_number);
    break;
  case NODE_INTERRUPT_TIMER:
    pico_indent(gen);
    pico_emit(gen,
              "machine.Timer().init(period=%d, mode=machine.Timer.PERIODIC, "
              "callback=lambda t: _isr_timer())\n",
              node->data.interrupt_timer.interval);
    break;
  case NODE_DISABLE_INTERRUPTS:
    pico_emit_line(gen, "_irq_state = machine.disable_irq()");
    break;
  case NODE_ENABLE_INTERRUPTS:
    pico_emit_line(gen, "machine.enable_irq(_irq_state)");
    break;
  case NODE_I2C_OPEN:
    pico_emit_line(gen, "_i2c = I2C(0, scl=Pin(5), sda=Pin(4), freq=400000)");
    break;
  case NODE_I2C_DEVICE_READ_ARRAY:
    pico_indent(gen);
    pico_emit(gen, "_i2c_data = _i2c.readfrom_mem(");
    pico_expr(gen, node->data.i2c_device_read_array.device_addr);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.i2c_device_read_array.reg_addr);
    pico_emit(gen, ", int(");
    pico_expr(gen, node->data.i2c_device_read_array.count);
    pico_emit(gen, "))\n");

    pico_indent(gen);
    pico_emit(gen, "for _i2c_i, _i2c_b in enumerate(_i2c_data):\n");
    gen->indent_level++;
    pico_indent(gen);
    pico_emit(gen, "%s[_i2c_i] = _i2c_b\n",
              node->data.i2c_device_read_array.array_name);
    gen->indent_level--;
    break;
  case NODE_I2C_DEVICE_WRITE:
    pico_indent(gen);
    pico_emit(gen, "_i2c.writeto_mem(");
    pico_expr(gen, node->data.i2c_device_write.device_addr);
    pico_emit(gen, ", ");
    pico_expr(
        gen,
        node->data.i2c_device_write
            .value); // register? or just raw bytes? wait, Kinetrix uses value.
    pico_emit(gen, ", bytes([");
    pico_expr(gen, node->data.i2c_device_write.value);
    pico_emit(gen, "]))\n");
    break;
  case NODE_SERIAL_OPEN:
    pico_emit_line(gen, "_uart = UART(0, baudrate=%d, tx=Pin(0), rx=Pin(1))",
                   node->data.serial_open.baud_rate);
    break;
  case NODE_SERIAL_SEND:
    pico_indent(gen);
    pico_emit(gen, "if '_uart' in globals(): _uart.write(str(");
    pico_expr(gen, node->data.serial_send.value);
    pico_emit(gen, "))\n");
    break;
  case NODE_SPI_OPEN:
    pico_emit_line(gen, "_spi = machine.SPI(0, baudrate=%d)",
                   node->data.spi_open.frequency > 0
                       ? node->data.spi_open.frequency
                       : 1000000);
    break;
  case NODE_DEVICE_DEF:
    pico_indent(gen);
    pico_emit(gen, "# Device %s configured\n",
              node->data.device_def.device_name);
    break;
  case NODE_DEVICE_WRITE:
    pico_indent(gen);
    pico_emit(gen, "# Write %s: ", node->data.device_write.device_name);
    pico_expr(gen, node->data.device_write.value);
    pico_emit(gen, "\n");
    break;
  case NODE_TASK_DEF:
    break;
  case NODE_TASK_START:
    pico_emit_line(gen, "import _thread");
    pico_emit_line(gen, "try: _thread.start_new_thread(_task_func_%s, ())",
                   node->data.task_start.task_name);
    pico_emit_line(gen, "except Exception: pass");
    break;
  case NODE_TRY:
    pico_emit_line(gen, "try:");
    gen->indent_level++;
    pico_stmt(gen, node->data.try_stmt.try_block);
    gen->indent_level--;
    if (node->data.try_stmt.error_block) {
      pico_emit_line(gen, "except Exception:");
      gen->indent_level++;
      pico_stmt(gen, node->data.try_stmt.error_block);
      gen->indent_level--;
    }
    break;
  case NODE_ASSERT:
    pico_indent(gen);
    pico_emit(gen, "if not (");
    pico_expr(gen, node->data.assert_stmt.condition);
    pico_emit(gen, "):\n");
    gen->indent_level++;
    if (node->data.assert_stmt.action)
      pico_stmt(gen, node->data.assert_stmt.action);
    else
      pico_emit_line(gen, "raise Exception('Assertion Failed')");
    gen->indent_level--;
    break;
  case NODE_RADIO_SEND:
    pico_indent(gen);
    pico_emit(gen, "_radio_send(");
    pico_expr(gen, node->data.radio_send.peer_id);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.radio_send.data);
    pico_emit(gen, ")\n");
    break;
  case NODE_WATCHDOG_ENABLE:
    pico_emit_line(gen, "_wdt = machine.WDT(timeout=%d)",
                   node->data.watchdog_enable.timeout_ms);
    break;
  case NODE_WATCHDOG_FEED:
    pico_emit_line(gen, "if '_wdt' in globals(): _wdt.feed()");
    break;
  case NODE_OTA_ENABLE:
    pico_emit_line(gen,
                   "# OTA: Enable WebREPL for wireless code push (Pico W)");
    pico_emit_line(gen, "try:");
    pico_emit_line(gen, "    import network, webrepl");
    pico_emit_line(gen, "    wlan = network.WLAN(network.STA_IF)");
    pico_emit_line(gen, "    wlan.active(True)");
    pico_emit_line(gen, "    if wlan.isconnected():");
    pico_emit_line(gen, "        webrepl.start()");
    pico_emit_line(
        gen, "        print('OTA ready via WebREPL at', wlan.ifconfig()[0])");
    pico_emit_line(
        gen, "except: print('OTA: WiFi not available on this Pico variant')");
    break;

  /* ---- Library Wrappers ---- */
  case NODE_SERVO_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_servo = PWM(Pin(int(");
    pico_expr(gen, node->data.servo_attach.pin);
    pico_emit(gen, "))); _kx_servo.freq(50)\n");
    break;
  case NODE_SERVO_MOVE:
    pico_indent(gen);
    pico_emit(gen, "_kx_servo.duty_u16(int(1638 + ");
    pico_expr(gen, node->data.servo_write.angle);
    pico_emit(gen, " * 52000 / 180))\n");
    break;
  case NODE_SERVO_DETACH:
    pico_emit_line(gen, "_kx_servo.deinit()");
    break;
  case NODE_DISTANCE_READ:
    pico_emit(gen, "_kx_read_distance(");
    pico_expr(gen, node->data.distance_read.trigger_pin);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.distance_read.echo_pin);
    pico_emit(gen, ")");
    break;
  case NODE_DHT_ATTACH:
    pico_emit_line(gen, "import dht");
    pico_indent(gen);
    pico_emit(gen, "_kx_dht = dht.DHT%d(Pin(int(",
              node->data.dht_attach.dht_type);
    pico_expr(gen, node->data.dht_attach.pin);
    pico_emit(gen, ")))\n");
    break;
  case NODE_DHT_READ_TEMP:
    pico_emit(gen, "(_kx_dht.measure() or True) and _kx_dht.temperature()");
    break;
  case NODE_DHT_READ_HUMID:
    pico_emit(gen, "(_kx_dht.measure() or True) and _kx_dht.humidity()");
    break;
  case NODE_NEOPIXEL_INIT:
    pico_emit_line(gen, "import neopixel");
    pico_indent(gen);
    pico_emit(gen, "_kx_strip = neopixel.NeoPixel(Pin(int(");
    pico_expr(gen, node->data.neopixel_init.pin);
    pico_emit(gen, ")), int(");
    pico_expr(gen, node->data.neopixel_init.count);
    pico_emit(gen, "))\n");
    break;
  case NODE_NEOPIXEL_SET:
    pico_indent(gen);
    pico_emit(gen, "_kx_strip[int(");
    pico_expr(gen, node->data.neopixel_set.index);
    pico_emit(gen, ")] = (int(");
    pico_expr(gen, node->data.neopixel_set.r);
    pico_emit(gen, "), int(");
    pico_expr(gen, node->data.neopixel_set.g);
    pico_emit(gen, "), int(");
    pico_expr(gen, node->data.neopixel_set.b);
    pico_emit(gen, "))\n");
    break;
  case NODE_NEOPIXEL_SHOW:
    pico_emit_line(gen, "_kx_strip.write()");
    break;
  case NODE_NEOPIXEL_CLEAR:
    pico_emit_line(gen, "_kx_strip.fill((0, 0, 0))");
    pico_emit_line(gen, "_kx_strip.write()");
    break;
  case NODE_LCD_INIT:
    pico_emit_line(gen, "from machine import I2C");
    pico_emit_line(gen, "from lcd_api import LcdApi");
    pico_emit_line(gen, "from pico_i2c_lcd import I2cLcd");
    pico_indent(gen);
    pico_emit(gen,
              "_kx_lcd = I2cLcd(I2C(0, scl=Pin(1), sda=Pin(0)), 0x27, int(");
    pico_expr(gen, node->data.lcd_init.rows);
    pico_emit(gen, "), int(");
    pico_expr(gen, node->data.lcd_init.cols);
    pico_emit(gen, "))\n");
    break;
  case NODE_LCD_PRINT:
    if (node->data.lcd_print.line) {
      pico_indent(gen);
      pico_emit(gen, "_kx_lcd.move_to(0, int(");
      pico_expr(gen, node->data.lcd_print.line);
      pico_emit(gen, "))\n");
    }
    pico_indent(gen);
    pico_emit(gen, "_kx_lcd.putstr(str(");
    pico_expr(gen, node->data.lcd_print.text);
    pico_emit(gen, "))\n");
    break;
  case NODE_LCD_CLEAR:
    pico_emit_line(gen, "_kx_lcd.clear()");
    break;

  /* --- Wave 2 Wrappers --- */
  case NODE_STEPPER_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_stepper_step = Pin(int(");
    pico_expr(gen, node->data.stepper_attach.step_pin);
    pico_emit(gen, "), Pin.OUT)\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_stepper_dir = Pin(int(");
    pico_expr(gen, node->data.stepper_attach.dir_pin);
    pico_emit(gen, "), Pin.OUT)\n");
    pico_emit_line(gen, "_kx_stepper_speed = 100");
    break;
  case NODE_STEPPER_SPEED:
    pico_indent(gen);
    pico_emit(gen, "_kx_stepper_speed = max(1, ");
    pico_expr(gen, node->data.unary.child);
    pico_emit(gen, ")\n");
    break;
  case NODE_STEPPER_MOVE:
    pico_indent(gen);
    pico_emit(gen, "_kx_steps = int(");
    pico_expr(gen, node->data.stepper_move.steps);
    pico_emit(gen, ")\n");
    pico_emit_line(gen, "_kx_stepper_dir.value(1 if _kx_steps > 0 else 0)");
    pico_emit_line(gen, "_kx_delay = 60.0 / (200.0 * _kx_stepper_speed)");
    pico_emit_line(gen, "for _ in range(abs(_kx_steps)):");
    gen->indent_level++;
    pico_emit_line(gen, "_kx_stepper_step.value(1)");
    pico_emit_line(gen, "utime.sleep(_kx_delay / 2)");
    pico_emit_line(gen, "_kx_stepper_step.value(0)");
    pico_emit_line(gen, "utime.sleep(_kx_delay / 2)");
    gen->indent_level--;
    break;
  case NODE_MOTOR_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_motor_fwd = Pin(int(");
    pico_expr(gen, node->data.motor_attach.fwd_pin);
    pico_emit(gen, "), Pin.OUT)\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_motor_rev = Pin(int(");
    pico_expr(gen, node->data.motor_attach.rev_pin);
    pico_emit(gen, "), Pin.OUT)\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_motor_pwm = PWM(Pin(int(");
    pico_expr(gen, node->data.motor_attach.en_pin);
    pico_emit(gen,
              "))); _kx_motor_pwm.freq(1000); _kx_motor_pwm.duty_u16(0)\n");
    break;
  case NODE_MOTOR_MOVE:
    if (node->data.motor_move.direction > 0) {
      pico_emit_line(gen, "_kx_motor_fwd.value(1)");
      pico_emit_line(gen, "_kx_motor_rev.value(0)");
    } else {
      pico_emit_line(gen, "_kx_motor_fwd.value(0)");
      pico_emit_line(gen, "_kx_motor_rev.value(1)");
    }
    pico_indent(gen);
    pico_emit(gen, "_kx_motor_pwm.duty_u16(int((");
    pico_expr(gen, node->data.motor_move.speed);
    pico_emit(gen, ") * 65535 / 255))\n");
    break;
  case NODE_MOTOR_STOP:
    pico_emit_line(gen, "_kx_motor_fwd.value(0)");
    pico_emit_line(gen, "_kx_motor_rev.value(0)");
    pico_emit_line(gen, "_kx_motor_pwm.duty_u16(0)");
    break;

  case NODE_MECANUM_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "global _kx_mec_fl, _kx_mec_fr, _kx_mec_bl, _kx_mec_br, ");
    pico_emit(gen, "_kx_mec_pwm_fl, _kx_mec_pwm_fr, _kx_mec_pwm_bl, _kx_mec_pwm_br\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_mec_fl = int("); pico_expr(gen, node->data.mecanum_attach.fl_pin); pico_emit(gen, ")\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_mec_fr = int("); pico_expr(gen, node->data.mecanum_attach.fr_pin); pico_emit(gen, ")\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_mec_bl = int("); pico_expr(gen, node->data.mecanum_attach.bl_pin); pico_emit(gen, ")\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_mec_br = int("); pico_expr(gen, node->data.mecanum_attach.br_pin); pico_emit(gen, ")\n");
    pico_emit_line(gen, "_kx_mec_pwm_fl = PWM(Pin(_kx_mec_fl)); _kx_mec_pwm_fl.freq(1000); _kx_mec_pwm_fl.duty_u16(0)");
    pico_emit_line(gen, "_kx_mec_pwm_fr = PWM(Pin(_kx_mec_fr)); _kx_mec_pwm_fr.freq(1000); _kx_mec_pwm_fr.duty_u16(0)");
    pico_emit_line(gen, "_kx_mec_pwm_bl = PWM(Pin(_kx_mec_bl)); _kx_mec_pwm_bl.freq(1000); _kx_mec_pwm_bl.duty_u16(0)");
    pico_emit_line(gen, "_kx_mec_pwm_br = PWM(Pin(_kx_mec_br)); _kx_mec_pwm_br.freq(1000); _kx_mec_pwm_br.duty_u16(0)");
    break;

  case NODE_MECANUM_MOVE:
    pico_emit_line(gen, "if _kx_mec_fl is not None:");
    gen->indent_level++;
    pico_indent(gen);
    pico_emit(gen, "_kf_y = "); pico_expr(gen, node->data.mecanum_move.y); pico_emit(gen, "\n");
    pico_indent(gen);
    pico_emit(gen, "_kf_x = "); pico_expr(gen, node->data.mecanum_move.x); pico_emit(gen, "\n");
    pico_indent(gen);
    pico_emit(gen, "_kf_t = "); pico_expr(gen, node->data.mecanum_move.turn); pico_emit(gen, "\n");
    pico_emit_line(gen, "_kx_fl = _kf_y + _kf_x + _kf_t");
    pico_emit_line(gen, "_kx_fr = _kf_y - _kf_x - _kf_t");
    pico_emit_line(gen, "_kx_bl = _kf_y - _kf_x + _kf_t");
    pico_emit_line(gen, "_kx_br = _kf_y + _kf_x - _kf_t");
    pico_emit_line(gen, "_kx_mec_pwm_fl.duty_u16(int(max(0, min(65535, abs(_kx_fl) * 65535 / 255))))");
    pico_emit_line(gen, "_kx_mec_pwm_fr.duty_u16(int(max(0, min(65535, abs(_kx_fr) * 65535 / 255))))");
    pico_emit_line(gen, "_kx_mec_pwm_bl.duty_u16(int(max(0, min(65535, abs(_kx_bl) * 65535 / 255))))");
    pico_emit_line(gen, "_kx_mec_pwm_br.duty_u16(int(max(0, min(65535, abs(_kx_br) * 65535 / 255))))");
    gen->indent_level--;
    break;

  case NODE_MECANUM_STOP:
    pico_emit_line(gen, "if _kx_mec_fl is not None:");
    gen->indent_level++;
    pico_emit_line(gen, "_kx_mec_pwm_fl.duty_u16(0)");
    pico_emit_line(gen, "_kx_mec_pwm_fr.duty_u16(0)");
    pico_emit_line(gen, "_kx_mec_pwm_bl.duty_u16(0)");
    pico_emit_line(gen, "_kx_mec_pwm_br.duty_u16(0)");
    gen->indent_level--;
    break;
  case NODE_ENCODER_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_enc_a = Pin(int(");
    pico_expr(gen, node->data.encoder_attach.pin_a);
    pico_emit(gen, "), Pin.IN, Pin.PULL_UP)\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_enc_b = Pin(int(");
    pico_expr(gen, node->data.encoder_attach.pin_b);
    pico_emit(gen, "), Pin.IN, Pin.PULL_UP)\n");
    pico_emit_line(gen, "_kx_enc_count = 0");
    pico_emit_line(gen, "def _kx_enc_cb(pin):");
    gen->indent_level++;
    pico_emit_line(gen, "global _kx_enc_count");
    pico_emit_line(gen, "if _kx_enc_b.value(): _kx_enc_count += 1");
    pico_emit_line(gen, "else: _kx_enc_count -= 1");
    gen->indent_level--;
    pico_emit_line(gen,
                   "_kx_enc_a.irq(trigger=Pin.IRQ_RISING, handler=_kx_enc_cb)");
    break;
  case NODE_ENCODER_READ:
    pico_emit(gen, "_kx_enc_count");
    break;
  case NODE_ENCODER_RESET:
    pico_emit_line(gen, "global _kx_enc_count; _kx_enc_count = 0");
    break;
  case NODE_ESC_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_esc_pwm = PWM(Pin(int(");
    pico_expr(gen, node->data.esc_attach.pin);
    pico_emit(gen, "))); _kx_esc_pwm.freq(50)\n");
    pico_emit_line(gen,
                   "_kx_esc_pwm.duty_u16(3277) # 5% / 1ms pulse (stopped)");
    break;
  case NODE_ESC_THROTTLE:
    pico_indent(gen);
    pico_emit(gen, "_kx_throttle_val = ");
    pico_expr(gen, node->data.unary.child);
    pico_emit(gen, "\n");
    pico_emit_line(
        gen, "_kx_esc_dc = 3277 + int((_kx_throttle_val / 180.0) * 3277)");
    pico_emit_line(gen, "_kx_esc_pwm.duty_u16(_kx_esc_dc)");
    break;
  case NODE_PID_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_pid_kp = ");
    pico_expr(gen, node->data.pid_attach.kp);
    pico_emit(gen, "\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_pid_ki = ");
    pico_expr(gen, node->data.pid_attach.ki);
    pico_emit(gen, "\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_pid_kd = ");
    pico_expr(gen, node->data.pid_attach.kd);
    pico_emit(gen, "\n");
    pico_emit_line(gen, "_kx_pid_setpoint = 0.0");
    pico_emit_line(gen, "_kx_pid_last_err = 0.0");
    pico_emit_line(gen, "_kx_pid_integral = 0.0");
    pico_emit_line(gen, "_kx_pid_last_time = utime.ticks_ms() / 1000.0");
    break;
  case NODE_PID_TARGET:
    pico_indent(gen);
    pico_emit(gen, "_kx_pid_setpoint = ");
    pico_expr(gen, node->data.unary.child);
    pico_emit(gen, "\n");
    break;

  /* Wave 3: Communication Statements */
  case NODE_BLE_ENABLE:
    pico_indent(gen);
    pico_emit(gen, "global _kx_ble_msg\n");
    pico_indent(gen);
    pico_emit(gen, "print('BLE enable (stub): ', ");
    pico_expr(gen, node->data.ble_enable.name);
    pico_emit(gen, ")\n");
    break;
  case NODE_BLE_ADVERTISE:
    pico_indent(gen);
    pico_emit(gen, "print('BLE advertise (stub): ', ");
    pico_expr(gen, node->data.ble_advertise.data);
    pico_emit(gen, ")\n");
    break;
  case NODE_BLE_SEND:
    pico_indent(gen);
    pico_emit(gen, "print('BLE send (stub): ', ");
    pico_expr(gen, node->data.ble_send.data);
    pico_emit(gen, ")\n");
    break;
  case NODE_WIFI_CONNECT:
    pico_indent(gen);
    pico_emit(gen, "global _kx_wifi_ip\n");
    pico_indent(gen);
    pico_emit(gen, "try:\n");
    gen->indent_level++;
    pico_indent(gen);
    pico_emit(gen, "_sta = network.WLAN(network.STA_IF); _sta.active(True)\n");
    pico_indent(gen);
    pico_emit(gen, "_sta.connect(str(");
    pico_expr(gen, node->data.wifi_connect.ssid);
    pico_emit(gen, "), str(");
    pico_expr(gen, node->data.wifi_connect.password);
    pico_emit(gen, "))\n");
    pico_indent(gen);
    pico_emit(gen, "while not _sta.isconnected(): utime.sleep(0.5)\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_wifi_ip = _sta.ifconfig()[0]\n");
    gen->indent_level--;
    pico_indent(gen);
    pico_emit(gen, "except: pass\n");
    break;
  case NODE_MQTT_CONNECT:
    pico_indent(gen);
    pico_emit(gen, "global _kx_mqtt_client\n");
    pico_indent(gen);
    pico_emit(gen, "try:\n");
    gen->indent_level++;
    pico_indent(gen);
    pico_emit(gen, "_kx_mqtt_client = MQTTClient('KinetrixPico', str(");
    pico_expr(gen, node->data.mqtt_connect.broker);
    pico_emit(gen, "), port=int(");
    pico_expr(gen, node->data.mqtt_connect.port);
    pico_emit(gen, "))\n");
    pico_indent(gen);
    pico_emit(gen, "def _mq_cb(t, p): global _kx_mqtt_msg; _kx_mqtt_msg = "
                   "p.decode('utf-8')\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_mqtt_client.set_callback(_mq_cb)\n");
    pico_indent(gen);
    pico_emit(gen, "_kx_mqtt_client.connect()\n");
    gen->indent_level--;
    pico_indent(gen);
    pico_emit(gen, "except: pass\n");
    break;
  case NODE_MQTT_SUBSCRIBE:
    pico_indent(gen);
    pico_emit(gen, "if _kx_mqtt_client: _kx_mqtt_client.subscribe(str(");
    pico_expr(gen, node->data.mqtt_subscribe.topic);
    pico_emit(gen, "))\n");
    break;
  case NODE_MQTT_PUBLISH:
    pico_indent(gen);
    pico_emit(gen, "if _kx_mqtt_client: _kx_mqtt_client.publish(str(");
    pico_expr(gen, node->data.mqtt_publish.topic);
    pico_emit(gen, "), str(");
    pico_expr(gen, node->data.mqtt_publish.payload);
    pico_emit(gen, "))\n");
    break;
  case NODE_HTTP_POST:
    pico_indent(gen);
    pico_emit(gen, "if 'urequests' in globals(): urequests.post(str(");
    pico_expr(gen, node->data.http_post.url);
    pico_emit(gen, "), data=str(");
    pico_expr(gen, node->data.http_post.body);
    pico_emit(gen, "))\n");
    break;
  case NODE_WS_CONNECT:
    pico_indent(gen);
    pico_emit(gen, "print('WS Connect (stub): ', ");
    pico_expr(gen, node->data.ws_connect.url);
    pico_emit(gen, ")\n");
    break;
  case NODE_WS_SEND:
    pico_indent(gen);
    pico_emit(gen, "print('WS Send (stub): ', ");
    pico_expr(gen, node->data.ws_send.data);
    pico_emit(gen, ")\n");
    break;
  case NODE_WS_CLOSE:
    pico_indent(gen);
    pico_emit(gen, "pass\n");
    break;

  case NODE_PID_COMPUTE:
    pico_emit(gen, "_kx_compute_pid(");
    pico_expr(gen, node->data.pid_compute.current_val);
    pico_emit(gen, ")\n");
    break;

  case NODE_KALMAN_ATTACH:
    pico_indent(gen);
    pico_emit_line(gen, "global _kx_kalman_p, _kx_kalman_x, _kx_kalman_k");
    pico_indent(gen);
    pico_emit_line(gen, "_kx_kalman_p = 1.0; _kx_kalman_x = 0.0; _kx_kalman_k = 0.0");
    break;

  case NODE_KALMAN_COMPUTE:
    pico_indent(gen);
    pico_emit(gen, "_kx_kalman_update(");
    pico_expr(gen, node->data.kalman_compute.raw_value);
    pico_emit(gen, ")\n");
    break;

  case NODE_AI_LOAD:
    pico_indent(gen);
    pico_emit(gen, "# AI Model loaded: ");
    pico_expr(gen, node->data.ai_load.model_path);
    pico_emit(gen, "\n");
    break;

  case NODE_AI_COMPUTE:
    pico_indent(gen);
    pico_emit(gen, "_kx_ai_invoke(");
    pico_expr(gen, node->data.ai_compute.input_array);
    pico_emit(gen, ")\n");
    break;

  case NODE_ARM_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_arm_len[0] = ");
    pico_expr(gen, node->data.arm_attach.len1);
    pico_emit(gen, "; _kx_arm_len[1] = ");
    pico_expr(gen, node->data.arm_attach.len2);
    pico_emit(gen, "; _kx_arm_len[2] = ");
    pico_expr(gen, node->data.arm_attach.len3);
    pico_emit(gen, "\n");
    break;
  case NODE_ARM_MOVE:
    pico_indent(gen);
    pico_emit(gen, "_kx_arm_ik(");
    pico_expr(gen, node->data.arm_move.x);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.arm_move.y);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.arm_move.z);
    pico_emit(gen, ")\n");
    break;
  case NODE_GRID_CREATE:
    pico_indent(gen);
    pico_emit(gen, "_kx_grid_w = int(");
    pico_expr(gen, node->data.grid_create.width);
    pico_emit(gen, "); _kx_grid_h = int(");
    pico_expr(gen, node->data.grid_create.height);
    pico_emit(gen, "); _kx_grid = [[0]*_kx_grid_w for _ in range(_kx_grid_h)]\n");
    break;
  case NODE_GRID_OBSTACLE:
    pico_indent(gen);
    pico_emit(gen, "_kx_grid[int(");
    pico_expr(gen, node->data.grid_obstacle.y);
    pico_emit(gen, ")][int(");
    pico_expr(gen, node->data.grid_obstacle.x);
    pico_emit(gen, ")] = 1\n");
    break;
  case NODE_PATH_COMPUTE:
    pico_indent(gen);
    pico_emit(gen, "_kx_path_compute(");
    pico_expr(gen, node->data.path_compute.from_x);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.path_compute.from_y);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.path_compute.to_x);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.path_compute.to_y);
    pico_emit(gen, ")\n");
    break;
  case NODE_DRONE_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_drone_pins = [");
    pico_expr(gen, node->data.drone_attach.fl);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.drone_attach.fr);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.drone_attach.bl);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.drone_attach.br);
    pico_emit(gen, "]\n");
    pico_indent(gen);
    pico_emit_line(gen, "for p in _kx_drone_pins: Pin(p, Pin.OUT)");
    break;
  case NODE_DRONE_SET:
    pico_indent(gen);
    pico_emit(gen, "_kx_drone_mix(");
    pico_expr(gen, node->data.drone_set.pitch);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.drone_set.roll);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.drone_set.yaw);
    pico_emit(gen, ", ");
    pico_expr(gen, node->data.drone_set.throttle);
    pico_emit(gen, ")\n");
    break;
  case NODE_TONE:
    pico_indent(gen);
    pico_emit(gen, "_pwm = PWM(Pin(");
    pico_expr(gen, node->data.gpio.pin);
    pico_emit(gen, ")); _pwm.freq(int(");
    pico_expr(gen, node->data.gpio.value);
    pico_emit(gen, ")); _pwm.duty_u16(32768)\n");
    break;
  case NODE_NOTONE:
    pico_indent(gen);
    pico_emit(gen, "PWM(Pin(");
    pico_expr(gen, node->data.gpio.pin);
    pico_emit(gen, ")).deinit()\n");
    break;
  case NODE_FUNCTION_DEF:
    if (node->data.function_def.is_extern) {
      pico_indent(gen);
      pico_emit(gen, "# Extern %s function: %s\n",
                node->data.function_def.extern_lang,
                node->data.function_def.name);
      break;
    }
    pico_indent(gen);
    pico_emit(gen, "def %s(", node->data.function_def.name);
    for (int i = 0; i < node->data.function_def.param_count; i++) {
      if (i > 0)
        pico_emit(gen, ", ");
      pico_emit(gen, "%s", node->data.function_def.param_names[i]);
    }
    pico_emit(gen, "):\n");
    gen->indent_level++;
    pico_stmt(gen, node->data.function_def.body);
    gen->indent_level--;
    pico_emit(gen, "\n");
    break;
  /* Wave 4: Advanced Robotics & Storage Statements */
  case NODE_IMU_ATTACH:
    pico_indent(gen);
    pico_emit_line(gen, "global _kx_imu");
    pico_indent(gen);
    pico_emit_line(gen, "try:");
    pico_indent(gen);
    pico_emit_line(gen, "    from bno08x_i2c import BNO08X_I2C");
    pico_indent(gen);
    pico_emit_line(gen,
                   "    _kx_imu = BNO08X_I2C(I2C(0, scl=Pin(5), sda=Pin(4)))");
    pico_indent(gen);
    pico_emit_line(gen, "except: _kx_imu = None");
    break;

  case NODE_GPS_ATTACH:
    pico_indent(gen);
    pico_emit_line(gen, "global _kx_gps_serial, _kx_gps");
    pico_indent(gen);
    pico_emit(gen, "try: _kx_gps_serial = UART(1, baudrate=int(");
    pico_expr(gen, node->data.gps_attach.baud);
    pico_emit(gen, "), tx=Pin(4), rx=Pin(5))\n");
    pico_indent(gen);
    pico_emit_line(gen, "except: _kx_gps_serial = None");
    break;

  case NODE_LIDAR_ATTACH:
    pico_indent(gen);
    pico_emit_line(gen, "global _kx_lidar");
    pico_indent(gen);
    pico_emit_line(gen, "try:");
    pico_indent(gen);
    pico_emit_line(gen, "    import vl53l0x");
    pico_indent(gen);
    pico_emit_line(
        gen, "    _kx_lidar = vl53l0x.VL53L0X(I2C(0, scl=Pin(5), sda=Pin(4)))");
    pico_indent(gen);
    pico_emit_line(gen, "except: _kx_lidar = None");
    break;

  case NODE_SD_MOUNT:
    pico_indent(gen);
    pico_emit_line(gen, "try:");
    pico_indent(gen);
    pico_emit_line(gen, "    import os, sdcard");
    pico_indent(gen);
    pico_emit(gen, "    _sd = sdcard.SDCard(SPI(1, sck=Pin(10), mosi=Pin(11), "
                   "miso=Pin(12)), Pin(");
    pico_expr(gen, node->data.sd_mount.cs_pin);
    pico_emit(gen, "))\n");
    pico_indent(gen);
    pico_emit_line(gen, "    os.mount(_sd, '/sd')");
    pico_indent(gen);
    pico_emit_line(gen, "except: print('SD Mount Failed')");
    break;

  case NODE_FILE_OPEN:
    pico_indent(gen);
    pico_emit(gen, "global _kx_file; _kx_file = open('/sd/' + str(");
    pico_expr(gen, node->data.file_open.filename);
    pico_emit(gen, "), 'a+')\n");
    break;

  case NODE_FILE_WRITE:
    pico_indent(gen);
    pico_emit(gen, "if _kx_file: _kx_file.write(str(");
    pico_expr(gen, node->data.file_write.data);
    pico_emit(gen, ") + '\\n')\n");
    break;

  case NODE_FILE_CLOSE:
    pico_indent(gen);
    pico_emit_line(gen, "if _kx_file: _kx_file.close(); _kx_file = None");
    break;

  /* Wave 5 Statements */
  case NODE_OLED_ATTACH:
    pico_indent(gen);
    pico_emit_line(gen, "global _kx_oled");
    pico_indent(gen);
    pico_emit_line(gen, "try: from ssd1306 import SSD1306_I2C; _kx_oled = SSD1306_I2C(128, 64, I2C(0, scl=Pin(5), sda=Pin(4)))");
    pico_indent(gen);
    pico_emit_line(gen, "except: _kx_oled = None");
    break;
  case NODE_OLED_PRINT:
    pico_indent(gen);
    pico_emit(gen, "if _kx_oled: _kx_oled.text(str(");
    pico_expr(gen, node->data.oled_print.text);
    pico_emit(gen, "), int(");
    pico_expr(gen, node->data.oled_print.x);
    pico_emit(gen, "), int(");
    pico_expr(gen, node->data.oled_print.y);
    pico_emit(gen, "))\n");
    break;
  case NODE_OLED_DRAW:
    pico_indent(gen);
    pico_emit_line(gen, "# OLED Draw (framebuf primitives)");
    break;
  case NODE_OLED_SHOW:
    pico_indent(gen);
    pico_emit_line(gen, "if _kx_oled: _kx_oled.show()");
    break;
  case NODE_OLED_CLEAR:
    pico_indent(gen);
    pico_emit_line(gen, "if _kx_oled: _kx_oled.fill(0)");
    break;
  case NODE_AUDIO_ATTACH:
    pico_indent(gen);
    pico_emit(gen, "_kx_audio_pin = PWM(Pin(int(");
    pico_expr(gen, node->data.audio_attach.pin);
    pico_emit(gen, ")))\n");
    break;
  case NODE_PLAY_FREQ:
    pico_indent(gen);
    pico_emit(gen, "if _kx_audio_pin: _kx_audio_pin.freq(int(");
    pico_expr(gen, node->data.play_freq.frequency);
    pico_emit(gen, ")); _kx_audio_pin.duty_u16(32768); utime.sleep_ms(int(");
    pico_expr(gen, node->data.play_freq.duration);
    pico_emit(gen, ")); _kx_audio_pin.duty_u16(0)\n");
    break;
  case NODE_PLAY_SOUND:
    pico_indent(gen);
    pico_emit_line(gen, "# Play sound (not supported on Pico)");
    break;
  case NODE_SET_VOLUME:
    pico_indent(gen);
    pico_emit_line(gen, "# Set volume (not supported on Pico)");
    break;
  case NODE_CAM_ATTACH:
    pico_indent(gen);
    pico_emit_line(gen, "global _kx_husky, _kx_husky_x, _kx_husky_y");
    pico_indent(gen);
    pico_emit_line(gen, "try: from huskylensPythonLibrary import HuskyLensLibrary; _kx_husky = HuskyLensLibrary('I2C')");
    pico_indent(gen);
    pico_emit_line(gen, "except: _kx_husky = None");
    break;

  default:
    break;
  }
}

void codegen_generate_pico(CodeGen *gen, ASTNode *program) {
  if (!program || program->type != NODE_PROGRAM)
    return;
  pico_emit_line(gen, "# Generated by Kinetrix Compiler (Target: Raspberry Pi "
                      "Pico / MicroPython)");
  pico_emit_line(gen,
                 "# Flash: Thonny IDE  OR  mpremote copy robot.py :main.py\n");
  pico_emit_line(gen, "from machine import Pin, ADC, PWM, I2C, UART");
  pico_emit_line(gen, "import utime, math");

  /* Wave 3 Imports */
  pico_emit_line(gen, "try: import network");
  pico_emit_line(gen, "except: pass");
  pico_emit_line(gen, "try: import urequests");
  pico_emit_line(gen, "except: pass");
  pico_emit_line(gen, "try: from umqtt.simple import MQTTClient");
  pico_emit_line(gen, "except: pass\n");

  pico_emit_line(gen, "def _safe_adc(pin):");
  pico_emit_line(gen, "    if pin not in (26, 27, 28, 29): return 0");
  pico_emit_line(gen, "    return ADC(Pin(pin)).read_u16() >> 6\n");

  pico_emit_line(gen, "# Wave 2 Globals");
  pico_emit_line(gen, "_kx_stepper_step, _kx_stepper_dir = None, None");
  pico_emit_line(gen, "_kx_stepper_speed = 100");
  pico_emit_line(
      gen, "_kx_motor_pwm, _kx_motor_fwd, _kx_motor_rev = None, None, None");
  pico_emit_line(gen, "_kx_enc_a, _kx_enc_b, _kx_enc_count = None, None, 0");
  pico_emit_line(gen, "_kx_esc_pwm = None");
  pico_emit_line(gen, "_kx_pid_kp, _kx_pid_ki, _kx_pid_kd = 0, 0, 0");
  pico_emit_line(
      gen, "_kx_pid_setpoint, _kx_pid_last_err, _kx_pid_integral = 0, 0, 0");
  pico_emit_line(gen, "_kx_pid_last_time = utime.ticks_ms() / 1000.0");

  /* Wave 3 Globals */
  pico_emit_line(gen, "_kx_mqtt_client = None");
  pico_emit_line(gen, "_kx_mqtt_msg = ''");
  pico_emit_line(gen, "_kx_ws_client = None");
  pico_emit_line(gen, "_kx_ws_msg = ''");
  pico_emit_line(gen, "_kx_ble_msg = ''");
  pico_emit_line(gen, "_kx_wifi_ip = ''\n");

  /* Wave 4 Modules & Globals */
  pico_emit_line(gen, "_kx_imu = None");
  pico_emit_line(gen, "_kx_gps_serial = None");
  pico_emit_line(gen, "_kx_gps = None");
  pico_emit_line(gen, "_kx_lidar = None");
  pico_emit_line(gen, "_kx_file = None\n");

  /* Wave 6 Globals */
  pico_emit_line(gen, "_kx_mec_fl = None; _kx_mec_fr = None; _kx_mec_bl = None; _kx_mec_br = None");
  pico_emit_line(gen, "_kx_mec_pwm_fl = None; _kx_mec_pwm_fr = None; _kx_mec_pwm_bl = None; _kx_mec_pwm_br = None\n");

  /* Wave 6 Kalman Globals */
  pico_emit_line(gen, "_kx_kalman_q = 0.01; _kx_kalman_r = 0.1");
  pico_emit_line(gen, "_kx_kalman_x = 0.0; _kx_kalman_p = 1.0; _kx_kalman_k = 0.0\n");

  pico_emit_line(gen, "def _kx_kalman_update(mea):");
  pico_emit_line(gen, "    global _kx_kalman_q, _kx_kalman_r, _kx_kalman_x, _kx_kalman_p, _kx_kalman_k");
  pico_emit_line(gen, "    _kx_kalman_p = _kx_kalman_p + _kx_kalman_q");
  pico_emit_line(gen, "    _kx_kalman_k = _kx_kalman_p / (_kx_kalman_p + _kx_kalman_r)");
  pico_emit_line(gen, "    _kx_kalman_x = _kx_kalman_x + _kx_kalman_k * (mea - _kx_kalman_x)");
  pico_emit_line(gen, "    _kx_kalman_p = (1.0 - _kx_kalman_k) * _kx_kalman_p");
  pico_emit_line(gen, "    return _kx_kalman_x\n");

  /* Wave 6 AI Helpers */
  pico_emit_line(gen, "def _kx_ai_invoke(input_data):");
  pico_emit_line(gen, "    return 0.0 # TFLite Micro omitted for generic target\n");

  /* Wave 7 MicroPython Helpers */
  pico_emit_line(gen, "import math");
  pico_emit_line(gen, "_kx_arm_len = [0.0, 0.0, 0.0, 0.0]");
  pico_emit_line(gen, "_kx_arm_angles = [0.0, 0.0, 0.0, 0.0]");
  pico_emit_line(gen, "def _kx_arm_ik(tx, ty, tz):");
  pico_emit_line(gen, "    global _kx_arm_angles");
  pico_emit_line(gen, "    r = math.sqrt(tx*tx + ty*ty)");
  pico_emit_line(gen, "    d = math.sqrt(r*r + tz*tz)");
  pico_emit_line(gen, "    L1, L2 = _kx_arm_len[0], _kx_arm_len[1]");
  pico_emit_line(gen, "    ca = (d*d-L1*L1-L2*L2)/(2.0*L1*L2) if L1*L2!=0 else 0");
  pico_emit_line(gen, "    ca = max(-1, min(1, ca))");
  pico_emit_line(gen, "    _kx_arm_angles[1] = math.acos(ca)");
  pico_emit_line(gen, "    _kx_arm_angles[0] = math.atan2(tz,r) - math.atan2(L2*math.sin(_kx_arm_angles[1]),L1+L2*ca)");
  pico_emit_line(gen, "    _kx_arm_angles[2] = math.atan2(ty, tx)\n");

  pico_emit_line(gen, "_kx_grid_w=0; _kx_grid_h=0; _kx_grid=[]; _kx_path_result=[]");
  pico_emit_line(gen, "def _kx_path_compute(sx,sy,gx,gy):");
  pico_emit_line(gen, "    global _kx_path_result; _kx_path_result=[]");
  pico_emit_line(gen, "    if sx==gx and sy==gy: return []");
  pico_emit_line(gen, "    visited=[[False]*_kx_grid_w for _ in range(_kx_grid_h)]");
  pico_emit_line(gen, "    q=[(sx,sy,[])]; visited[sy][sx]=True");
  pico_emit_line(gen, "    while q:");
  pico_emit_line(gen, "        cx,cy,path=q.pop(0)");
  pico_emit_line(gen, "        if cx==gx and cy==gy: _kx_path_result=path+[(cx,cy)]; return _kx_path_result");
  pico_emit_line(gen, "        for dx,dy in [(1,0),(-1,0),(0,1),(0,-1)]:");
  pico_emit_line(gen, "            nx,ny=cx+dx,cy+dy");
  pico_emit_line(gen, "            if 0<=nx<_kx_grid_w and 0<=ny<_kx_grid_h and not visited[ny][nx] and not _kx_grid[ny][nx]:");
  pico_emit_line(gen, "                visited[ny][nx]=True; q.append((nx,ny,path+[(cx,cy)]))");
  pico_emit_line(gen, "    return []\n");

  pico_emit_line(gen, "_kx_drone_pins=[-1,-1,-1,-1]");
  pico_emit_line(gen, "def _kx_drone_mix(pitch,roll,yaw,throttle):");
  pico_emit_line(gen, "    vals=[int(throttle+pitch+roll-yaw),int(throttle+pitch-roll+yaw),int(throttle-pitch+roll+yaw),int(throttle-pitch-roll-yaw)]");
  pico_emit_line(gen, "    vals=[max(0,min(65535,v*257)) for v in vals]");
  pico_emit_line(gen, "    for i,p in enumerate(_kx_drone_pins):");
  pico_emit_line(gen, "        if p>=0: PWM(Pin(p)).duty_u16(vals[i])\n");

  /* Wave 5 Globals */
  pico_emit_line(gen, "_kx_oled = None");
  pico_emit_line(gen, "_kx_audio_pin = None");
  pico_emit_line(gen, "_kx_husky = None");
  pico_emit_line(gen, "_kx_husky_x, _kx_husky_y = 0, 0\n");

  pico_emit_line(gen, "def _kx_compute_pid(current_val):");
  pico_emit_line(
      gen,
      "    global _kx_pid_kp, _kx_pid_ki, _kx_pid_kd, _kx_pid_setpoint, \\");
  pico_emit_line(
      gen, "           _kx_pid_last_err, _kx_pid_integral, _kx_pid_last_time");
  pico_emit_line(gen, "    now = utime.ticks_ms() / 1000.0");
  pico_emit_line(gen, "    dt = now - _kx_pid_last_time");
  pico_emit_line(gen, "    if dt <= 0.0: dt = 0.001");
  pico_emit_line(gen, "    err = _kx_pid_setpoint - current_val");
  pico_emit_line(gen, "    _kx_pid_integral += err * dt");
  pico_emit_line(gen, "    deriv = (err - _kx_pid_last_err) / dt");
  pico_emit_line(gen, "    out = (_kx_pid_kp * err) + (_kx_pid_ki * "
                      "_kx_pid_integral) + (_kx_pid_kd * deriv)");
  pico_emit_line(gen, "    _kx_pid_last_err = err");
  pico_emit_line(gen, "    _kx_pid_last_time = now");
  pico_emit_line(gen, "    return out\n");

  // Add lightweight radio function stubs to Pico MicroPython output
  pico_emit_line(gen, "def _radio_send(peer, data):");
  pico_emit_line(
      gen, "    pass # Placeholder for Pico NRF24 or ESP-NOW transmission\n");
  pico_emit_line(gen, "def _radio_available():");
  pico_emit_line(gen, "    return 0\n");
  pico_emit_line(gen, "def _radio_read():");
  pico_emit_line(gen, "    return 0\n");

  ASTNode *block = program->data.program.main_block;
  if (block && block->type == NODE_BLOCK) {
    // Hoist global vars, arrays, buffers
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type == NODE_VAR_DECL || stmt->type == NODE_ARRAY_DECL ||
          stmt->type == NODE_BUFFER_DECL || stmt->type == NODE_SHARED_DECL) {
        pico_stmt(gen, stmt);
      }
    }
    // Hoist functions
    for (int i = 0; i < block->data.block.statement_count; i++) {
      if (block->data.block.statements[i]->type == NODE_FUNCTION_DEF)
        pico_stmt(gen, block->data.block.statements[i]);
    }
    // Hoist ISRs
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type == NODE_INTERRUPT_PIN) {
        pico_emit_line(
            gen, "def _isr_pin%d():", stmt->data.interrupt_pin.pin_number);
        gen->indent_level++;
        pico_stmt(gen, stmt->data.interrupt_pin.body);
        gen->indent_level--;
        pico_emit_line(gen, "");
      } else if (stmt->type == NODE_INTERRUPT_TIMER) {
        pico_emit_line(gen, "def _isr_timer():");
        gen->indent_level++;
        pico_stmt(gen, stmt->data.interrupt_timer.body);
        gen->indent_level--;
        pico_emit_line(gen, "");
      }
    }
    // Hoist tasks
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type == NODE_TASK_DEF) {
        pico_emit_line(gen, "def _task_func_%s():", stmt->data.task_def.name);
        gen->indent_level++;
        pico_emit_line(gen, "while True:");
        gen->indent_level++;
        pico_stmt(gen, stmt->data.task_def.body);
        gen->indent_level--;
        gen->indent_level--;
        pico_emit_line(gen, "");
      }
    }
    // Main block logic
    pico_emit_line(gen, "def _kinetrix_main():");
    gen->indent_level++;
    pico_emit_line(gen, "global _irq_state, _i2c, _spi, _uart, _pwm, _wdt");
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *stmt = block->data.block.statements[i];
      if (stmt->type != NODE_FUNCTION_DEF && stmt->type != NODE_TASK_DEF &&
          stmt->type != NODE_VAR_DECL && stmt->type != NODE_ARRAY_DECL &&
          stmt->type != NODE_BUFFER_DECL && stmt->type != NODE_SHARED_DECL &&
          stmt->type != NODE_INTERRUPT_PIN &&
          stmt->type != NODE_INTERRUPT_TIMER) {
        pico_stmt(gen, stmt);
      }
    }
    gen->indent_level--;
    pico_emit_line(gen, "if __name__ == '__main__':");
    pico_emit_line(gen, "    _kinetrix_main()");
  } else if (block) {
    pico_stmt(gen, block);
  }
}
