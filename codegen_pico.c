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
  case NODE_STRING:
    pico_emit(gen, "\"%s\"", node->data.string.value);
    break;
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
    pico_emit(gen, "(int.from_bytes(_spi.read(1, ");
    pico_expr(gen, node->data.spi_transfer.data);
    pico_emit(gen, "), 'big') if '_spi' in globals() else 0)");
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
  pico_emit_line(gen, "import utime, math\n");

  pico_emit_line(gen, "def _safe_adc(pin):");
  pico_emit_line(gen, "    if pin not in (26, 27, 28, 29): return 0");
  pico_emit_line(gen, "    return ADC(Pin(pin)).read_u16() >> 6\n");

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
