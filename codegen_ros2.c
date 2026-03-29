/* Kinetrix ROS2 Code Generator
 * Target: ROS2 (Robot Operating System 2) C++ Node
 * Output: .cpp file — a complete rclcpp node
 * Build with: colcon build inside a ROS2 workspace
 * Run with:   ros2 run <pkg> <node_name>
 */

#include "ast.h"
#include "codegen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ros2_expr(CodeGen *gen, ASTNode *node);
static void ros2_stmt(CodeGen *gen, ASTNode *node);

static void ros2_expr(CodeGen *gen, ASTNode *node) {
  if (!node)
    return;
  switch (node->type) {
  case NODE_NUMBER:
    codegen_emit(gen, "%g", node->data.number.value);
    break;
  case NODE_STRING:
    codegen_emit(gen, "std::string(\"%s\")", node->data.string.value);
    break;
  case NODE_BOOL:
    codegen_emit(gen, "%s", node->data.boolean.value ? "true" : "false");
    break;
  case NODE_IDENTIFIER:
    codegen_emit(gen, "%s_", node->data.identifier.name);
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
      op = "&&";
      break;
    case OP_OR:
      op = "||";
      break;
    default:
      break;
    }
    if (node->data.binary_op.op == OP_ADD &&
        ((node->data.binary_op.left->value_type &&
          node->data.binary_op.left->value_type->kind == TYPE_STRING) ||
         (node->data.binary_op.right->value_type &&
          node->data.binary_op.right->value_type->kind == TYPE_STRING))) {

      int r_string =
          (node->data.binary_op.right->value_type &&
           node->data.binary_op.right->value_type->kind == TYPE_STRING) ||
          node->data.binary_op.right->type == NODE_STRING;
      int l_string =
          (node->data.binary_op.left->value_type &&
           node->data.binary_op.left->value_type->kind == TYPE_STRING) ||
          node->data.binary_op.left->type == NODE_STRING;

      codegen_emit(gen, "(");
      if (!l_string)
        codegen_emit(gen, "std::to_string(");
      ros2_expr(gen, node->data.binary_op.left);
      if (!l_string)
        codegen_emit(gen, ")");

      codegen_emit(gen, " + ");

      if (!r_string)
        codegen_emit(gen, "std::to_string(");
      ros2_expr(gen, node->data.binary_op.right);
      if (!r_string)
        codegen_emit(gen, ")");
      codegen_emit(gen, ")");
    } else {
      codegen_emit(gen, "(");
      ros2_expr(gen, node->data.binary_op.left);
      codegen_emit(gen, " %s ", op);
      ros2_expr(gen, node->data.binary_op.right);
      codegen_emit(gen, ")");
    }
    break;
  }
  case NODE_UNARY_OP:
    codegen_emit(gen, node->data.unary_op.op == OP_NOT ? "!(" : "-(");
    ros2_expr(gen, node->data.unary_op.operand);
    codegen_emit(gen, ")");
    break;
  case NODE_CALL: {
    const char *nm = node->data.call.name;
    if (strcmp(nm, "map") == 0 && node->data.call.arg_count == 5) {
      codegen_emit(gen, "(int)((");
      ros2_expr(gen, node->data.call.args[0]);
      codegen_emit(gen, " - ");
      ros2_expr(gen, node->data.call.args[1]);
      codegen_emit(gen, ") * (");
      ros2_expr(gen, node->data.call.args[4]);
      codegen_emit(gen, " - ");
      ros2_expr(gen, node->data.call.args[3]);
      codegen_emit(gen, ") / (");
      ros2_expr(gen, node->data.call.args[2]);
      codegen_emit(gen, " - ");
      ros2_expr(gen, node->data.call.args[1]);
      codegen_emit(gen, ") + ");
      ros2_expr(gen, node->data.call.args[3]);
      codegen_emit(gen, ")");
    } else if (strcmp(nm, "constrain") == 0) {
      codegen_emit(gen, "std::max(");
      ros2_expr(gen, node->data.call.args[1]);
      codegen_emit(gen, ", std::min(");
      ros2_expr(gen, node->data.call.args[2]);
      codegen_emit(gen, ", ");
      ros2_expr(gen, node->data.call.args[0]);
      codegen_emit(gen, "))");
    } else {
      codegen_emit(gen, "%s(", nm);
      for (int i = 0; i < node->data.call.arg_count; i++) {
        if (i > 0)
          codegen_emit(gen, ", ");
        ros2_expr(gen, node->data.call.args[i]);
      }
      codegen_emit(gen, ")");
    }
    break;
  }
  case NODE_ANALOG_READ:
    /* In ROS2, analog reads come from topic subscriptions */
    codegen_emit(gen, "sensor_val_");
    ros2_expr(gen, node->data.gpio.pin);
    break;
  case NODE_GPIO_READ:
    codegen_emit(gen, "pin_state_");
    ros2_expr(gen, node->data.gpio.pin);
    break;
  case NODE_RADIO_AVAILABLE:
    /* Placeholder for ros2 topic subscription if implemented */
    codegen_emit(gen, "0.0");
    break;
  case NODE_RADIO_READ:
    codegen_emit(gen, "0.0");
    break;
  case NODE_ENCODER_READ:
    codegen_emit(gen, "_kx_encoder_pos");
    break;
  case NODE_PID_COMPUTE:
    codegen_emit(gen, "compute_pid(");
    ros2_expr(gen, node->data.pid_compute.current_val);
    codegen_emit(gen, ")");
    break;
  case NODE_KALMAN_COMPUTE:
    codegen_emit(gen, "compute_kalman_update(");
    ros2_expr(gen, node->data.kalman_compute.raw_value);
    codegen_emit(gen, ")");
    break;
  case NODE_AI_COMPUTE:
    codegen_emit(gen, "_kx_ai_invoke(");
    ros2_expr(gen, node->data.ai_compute.input_array);
    codegen_emit(gen, ")");
    break;
  case NODE_PATH_COMPUTE:
    codegen_emit(gen, "_kx_path_compute(");
    ros2_expr(gen, node->data.path_compute.from_x);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.path_compute.from_y);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.path_compute.to_x);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.path_compute.to_y);
    codegen_emit(gen, ")");
    break;

  /* Wave 3: Communication Expressions */
  case NODE_BLE_RECEIVE:
    codegen_emit(gen, "ble_msg_");
    break;
  case NODE_WIFI_IP:
    codegen_emit(gen, "wifi_ip_");
    break;
  case NODE_MQTT_READ:
    codegen_emit(gen, "mqtt_msg_");
    break;
  case NODE_HTTP_GET:
    codegen_emit(gen, "std::string(\"\") /* HTTP GET stub */");
    break;
  case NODE_WS_RECEIVE:
    codegen_emit(gen, "ws_msg_");
    break;

  /* Wave 4: Advanced Robotics & Storage Expressions */
  case NODE_IMU_READ_X:
    codegen_emit(gen, "imu_ax_");
    break;
  case NODE_IMU_READ_Y:
    codegen_emit(gen, "imu_ay_");
    break;
  case NODE_IMU_READ_Z:
    codegen_emit(gen, "imu_az_");
    break;
  case NODE_IMU_ORIENT:
    codegen_emit(gen, "imu_head_");
    break;
  case NODE_GPS_READ_LAT:
    codegen_emit(gen, "gps_lat_");
    break;
  case NODE_GPS_READ_LON:
    codegen_emit(gen, "gps_lon_");
    break;
  case NODE_GPS_READ_ALT:
    codegen_emit(gen, "gps_alt_");
    break;
  case NODE_GPS_READ_SPD:
    codegen_emit(gen, "gps_spd_");
    break;
  case NODE_LIDAR_READ:
    codegen_emit(gen, "lidar_dist_");
    break;
  case NODE_FILE_READ:
    codegen_emit(gen, "file_data_");
    break;

  /* Wave 5 Expressions */
  case NODE_CAM_DETECT: codegen_emit(gen, "(cam_detect_ ? 1.0 : 0.0)"); break;
  case NODE_CAM_OBJ_X: codegen_emit(gen, "cam_obj_x_"); break;
  case NODE_CAM_OBJ_Y: codegen_emit(gen, "cam_obj_y_"); break;

  default:
    codegen_emit(gen, "/* unsupported expr */ 0");
    break;
  }
}

static void ros2_stmt(CodeGen *gen, ASTNode *node) {
  if (!node)
    return;
  switch (node->type) {
  case NODE_VAR_DECL:
    codegen_emit_indent(gen);
    if (node->data.var_decl.is_const) {
      codegen_emit(gen, "const ");
    }
    codegen_emit(gen, "double %s_", node->data.var_decl.name);
    if (node->data.var_decl.initializer) {
      codegen_emit(gen, " = ");
      ros2_expr(gen, node->data.var_decl.initializer);
    } else
      codegen_emit(gen, " = 0.0");
    codegen_emit(gen, ";\n");
    break;
  case NODE_ASSIGNMENT:
    codegen_emit_indent(gen);
    ros2_expr(gen, node->data.assignment.target);
    codegen_emit(gen, " = ");
    ros2_expr(gen, node->data.assignment.value);
    codegen_emit(gen, ";\n");
    break;
  case NODE_IF:
    codegen_emit_indent(gen);
    codegen_emit(gen, "if (");
    ros2_expr(gen, node->data.if_stmt.condition);
    codegen_emit(gen, ") {\n");
    gen->indent_level++;
    ros2_stmt(gen, node->data.if_stmt.then_block);
    gen->indent_level--;
    codegen_emit_line(gen, "}");
    if (node->data.if_stmt.else_block) {
      codegen_emit(gen, " else {\n");
      gen->indent_level++;
      ros2_stmt(gen, node->data.if_stmt.else_block);
      gen->indent_level--;
      codegen_emit_line(gen, "}");
    }
    codegen_emit(gen, "\n");
    break;
  case NODE_WHILE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "while (");
    ros2_expr(gen, node->data.while_loop.condition);
    codegen_emit(gen, ") {\n");
    gen->indent_level++;
    ros2_stmt(gen, node->data.while_loop.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  case NODE_REPEAT: {
    int id = gen->loop_counter++;
    codegen_emit_line(gen, "for (int _i%d = 0; _i%d < (int)(", id, id);
    ros2_expr(gen, node->data.repeat_loop.count);
    codegen_emit(gen, "); _i%d++) {\n", id);
    gen->indent_level++;
    ros2_stmt(gen, node->data.repeat_loop.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  }
  case NODE_FOR: {
    int loop_id = gen->loop_counter++;
    codegen_emit_indent(gen);
    codegen_emit(gen, "int _start_%d = (", loop_id);
    ros2_expr(gen, node->data.for_loop.start_expr);
    codegen_emit(gen, ");\n");

    codegen_emit_indent(gen);
    codegen_emit(gen, "int _end_%d = (", loop_id);
    ros2_expr(gen, node->data.for_loop.end_expr);
    codegen_emit(gen, ");\n");

    codegen_emit_indent(gen);
    if (node->data.for_loop.step_expr) {
      codegen_emit(gen, "int _step_%d = (", loop_id);
      ros2_expr(gen, node->data.for_loop.step_expr);
      codegen_emit(gen, ");\n");
    } else {
      codegen_emit(gen, "int _step_%d = (_start_%d <= _end_%d) ? 1 : -1;\n",
                   loop_id, loop_id, loop_id);
    }

    codegen_emit_line(gen,
                      "for (int %s_ = _start_%d; _step_%d > 0 ? %s_ <= _end_%d "
                      ": %s_ >= _end_%d; %s_ += _step_%d) {\n",
                      node->data.for_loop.var_name, loop_id, loop_id,
                      node->data.for_loop.var_name, loop_id,
                      node->data.for_loop.var_name, loop_id,
                      node->data.for_loop.var_name, loop_id);
    gen->indent_level++;
    ros2_stmt(gen, node->data.for_loop.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  }
  case NODE_FOREVER:
    /* In ROS2, forever loops become timer callbacks */
    codegen_emit_line(gen, "/* loop forever → ROS2 timer callback */\n");
    ros2_stmt(gen, node->data.forever_loop.body);
    break;
  case NODE_BLOCK:
    for (int i = 0; i < node->data.block.statement_count; i++)
      ros2_stmt(gen, node->data.block.statements[i]);
    break;
  case NODE_DEVICE_DEF:
    codegen_emit_indent(gen);
    codegen_emit(gen, "const int %s = ", node->data.device_def.device_name);
    ros2_expr(gen, node->data.device_def.address_or_baud);
    codegen_emit(gen, ";\n");
    break;
  case NODE_BREAK:
    codegen_emit_line(gen, "break;\n");
    break;
  case NODE_CONTINUE:
    codegen_emit_line(gen, "continue;\n");
    break;
  case NODE_RETURN:
    codegen_emit_indent(gen);
    codegen_emit(gen, "return");
    if (node->data.return_stmt.value) {
      codegen_emit(gen, " ");
      ros2_expr(gen, node->data.return_stmt.value);
    }
    codegen_emit(gen, ";\n");
    break;
  case NODE_DEVICE_WRITE:
  case NODE_STRUCT_DEF:
  case NODE_I2C_DEVICE_WRITE:
  case NODE_I2C_DEVICE_READ_ARRAY:
    /* Placeholders for ROS2 I2C expansion */
    codegen_emit_indent(gen);
    codegen_emit(
        gen,
        "/* I2C / Struct / Device operations unmapped in ros2 frontend */\n");
    break;
  case NODE_GPIO_WRITE: {
    /* Publish HIGH/LOW to /gpio/pin_N topic */
    codegen_emit_indent(gen);
    codegen_emit(gen, "{ auto _m = std_msgs::msg::Bool(); _m.data = (bool)(");
    ros2_expr(gen, node->data.gpio.value);
    codegen_emit(gen, "); gpio_pub_->publish(_m); }\n");
    break;
  }
  case NODE_ANALOG_WRITE: {
    codegen_emit_indent(gen);
    codegen_emit(gen, "{ auto _m = std_msgs::msg::Float64(); _m.data = ");
    ros2_expr(gen, node->data.gpio.value);
    codegen_emit(gen, "; pwm_pub_->publish(_m); }\n");
    break;
  }
  case NODE_WAIT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "rclcpp::sleep_for(std::chrono::milliseconds((int64_t)(");
    ros2_expr(gen, node->data.unary.child);
    codegen_emit(gen, ")));\n");
    break;
  case NODE_PRINT:
    codegen_emit_indent(gen);
    codegen_emit(gen,
                 "RCLCPP_INFO(this->get_logger(), \"%%s\", std::to_string(");
    ros2_expr(gen, node->data.unary.child);
    codegen_emit(gen, ").c_str());\n");
    break;

  case NODE_PRINTLN:
    codegen_emit_indent(gen);
    codegen_emit(gen,
                 "RCLCPP_INFO(this->get_logger(), \"%%s\", std::to_string(");
    ros2_expr(gen, node->data.unary.child);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_CALL:
    codegen_emit_indent(gen);
    ros2_expr(gen, node);
    codegen_emit(gen, ";\n");
    break;
  case NODE_FUNCTION_DEF:
    if (node->data.function_def.is_extern) {
      codegen_emit_line(gen, "/* Extern %s function: %s */",
                        node->data.function_def.extern_lang,
                        node->data.function_def.name);
      break;
    }
    codegen_emit_line(gen, "double %s(", node->data.function_def.name);
    for (int i = 0; i < node->data.function_def.param_count; i++) {
      if (i > 0)
        codegen_emit(gen, ", ");
      codegen_emit(gen, "double %s", node->data.function_def.param_names[i]);
    }
    codegen_emit(gen, ") {\n");
    gen->indent_level++;
    ros2_stmt(gen, node->data.function_def.body);
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");
    break;
  case NODE_RADIO_SEND:
    /* Expected to publish to a radio output topic for fleet comms */
    codegen_emit_indent(gen);
    codegen_emit(gen, "/* ROS2 Radio Send Placeholder */\n");
    break;

  /* --- Wave 2 Wrappers --- */
  case NODE_STEPPER_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "// Stepper attach (Step: ");
    ros2_expr(gen, node->data.stepper_attach.step_pin);
    codegen_emit(gen, ", Dir: ");
    ros2_expr(gen, node->data.stepper_attach.dir_pin);
    codegen_emit(gen, ")\n");
    break;
  case NODE_STEPPER_SPEED:
    codegen_emit_indent(gen);
    codegen_emit(gen, "stepper_speed_ = std::max(1.0, (double)(");
    ros2_expr(gen, node->data.unary.child);
    codegen_emit(gen, "));\n");
    break;
  case NODE_STEPPER_MOVE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "{ auto _m = std_msgs::msg::Float64(); _m.data = ");
    ros2_expr(gen, node->data.stepper_move.steps);
    codegen_emit(gen, "; stepper_pub_->publish(_m); }\n");
    break;
  case NODE_MOTOR_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "// Motor attach (En: ");
    ros2_expr(gen, node->data.motor_attach.en_pin);
    codegen_emit(gen, ", Fwd: ");
    ros2_expr(gen, node->data.motor_attach.fwd_pin);
    codegen_emit(gen, ")\n");
    break;
  case NODE_MOTOR_MOVE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "{ auto _m = std_msgs::msg::Float64(); _m.data = ");
    ros2_expr(gen, node->data.motor_move.speed);
    codegen_emit(gen, " * %d; motor_pub_->publish(_m); }\n",
                 node->data.motor_move.direction);
    break;
  case NODE_MOTOR_STOP:
    codegen_emit_indent(gen);
    codegen_emit_line(
        gen, "{ auto _m = std_msgs::msg::Float64(); _m.data = 0; "
             "motor_pub_->publish(_m); }");
    break;

  case NODE_MECANUM_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "// Mecanum attach (FL: ");
    ros2_expr(gen, node->data.mecanum_attach.fl_pin);
    codegen_emit(gen, ", FR: ");
    ros2_expr(gen, node->data.mecanum_attach.fr_pin);
    codegen_emit(gen, ")\n");
    break;

  case NODE_MECANUM_MOVE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "{\n");
    gen->indent_level++;
    codegen_emit_indent(gen); codegen_emit(gen, "double _kf_y = "); ros2_expr(gen, node->data.mecanum_move.y); codegen_emit(gen, ";\n");
    codegen_emit_indent(gen); codegen_emit(gen, "double _kf_x = "); ros2_expr(gen, node->data.mecanum_move.x); codegen_emit(gen, ";\n");
    codegen_emit_indent(gen); codegen_emit(gen, "double _kf_t = "); ros2_expr(gen, node->data.mecanum_move.turn); codegen_emit(gen, ";\n");
    codegen_emit_line(gen, "auto _m1 = std_msgs::msg::Float64(); _m1.data = _kf_y + _kf_x + _kf_t; mec_fl_pub_->publish(_m1);");
    codegen_emit_line(gen, "auto _m2 = std_msgs::msg::Float64(); _m2.data = _kf_y - _kf_x - _kf_t; mec_fr_pub_->publish(_m2);");
    codegen_emit_line(gen, "auto _m3 = std_msgs::msg::Float64(); _m3.data = _kf_y - _kf_x + _kf_t; mec_bl_pub_->publish(_m3);");
    codegen_emit_line(gen, "auto _m4 = std_msgs::msg::Float64(); _m4.data = _kf_y + _kf_x - _kf_t; mec_br_pub_->publish(_m4);");
    gen->indent_level--;
    codegen_emit_indent(gen); codegen_emit(gen, "}\n");
    break;

  case NODE_MECANUM_STOP:
    codegen_emit_indent(gen);
    codegen_emit_line(gen, "{ auto _m = std_msgs::msg::Float64(); _m.data = 0; mec_fl_pub_->publish(_m); mec_fr_pub_->publish(_m); mec_bl_pub_->publish(_m); mec_br_pub_->publish(_m); }");
    break;
  case NODE_ENCODER_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "// Encoder attach (A: ");
    ros2_expr(gen, node->data.encoder_attach.pin_a);
    codegen_emit(gen, ", B: ");
    ros2_expr(gen, node->data.encoder_attach.pin_b);
    codegen_emit(gen, ")\n");
    break;
  case NODE_ENCODER_READ:
    codegen_emit(gen, "encoder_count_"); // Assuming a subscriber updates this
    break;
  case NODE_ENCODER_RESET:
    codegen_emit_line(gen, "encoder_count_ = 0.0;");
    break;
  case NODE_ESC_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "// ESC attach (Pin: ");
    ros2_expr(gen, node->data.esc_attach.pin);
    codegen_emit(gen, ")\n");
    break;
  case NODE_ESC_THROTTLE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "{ auto _m = std_msgs::msg::Float64(); _m.data = ");
    ros2_expr(gen, node->data.unary.child);
    codegen_emit(gen, "; esc_pub_->publish(_m); }\n");
    break;
  case NODE_PID_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "pid_kp_ = ");
    ros2_expr(gen, node->data.pid_attach.kp);
    codegen_emit(gen, "; ");
    codegen_emit(gen, "pid_ki_ = ");
    ros2_expr(gen, node->data.pid_attach.ki);
    codegen_emit(gen, "; ");
    codegen_emit(gen, "pid_kd_ = ");
    ros2_expr(gen, node->data.pid_attach.kd);
    codegen_emit(gen, ";\n");
    codegen_emit_line(gen, "pid_integral_ = 0.0; pid_last_err_ = 0.0;");
    codegen_emit_line(gen, "pid_last_time_ = this->now();");
    break;
  case NODE_PID_TARGET:
    codegen_emit_indent(gen);
    codegen_emit(gen, "pid_setpoint_ = ");
    ros2_expr(gen, node->data.unary.child);
    codegen_emit(gen, ";\n");
    break;
  case NODE_PID_COMPUTE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "compute_pid(");
    ros2_expr(gen, node->data.pid_compute.current_val);
    codegen_emit(gen, ");\n");
    break;

  case NODE_KALMAN_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit_line(gen, "kalman_p_ = 1.0; kalman_x_ = 0.0; kalman_k_ = 0.0;");
    break;

  case NODE_KALMAN_COMPUTE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "compute_kalman_update(");
    ros2_expr(gen, node->data.kalman_compute.raw_value);
    codegen_emit(gen, ");\n");
    break;

  case NODE_AI_LOAD:
    codegen_emit_indent(gen);
    codegen_emit(gen, "// AI Model loaded: ");
    ros2_expr(gen, node->data.ai_load.model_path);
    codegen_emit(gen, "\n");
    break;

  case NODE_AI_COMPUTE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_ai_invoke(");
    ros2_expr(gen, node->data.ai_compute.input_array);
    codegen_emit(gen, ");\n");
    break;

  case NODE_ARM_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_arm_dof = (int)");
    ros2_expr(gen, node->data.arm_attach.dof);
    codegen_emit(gen, "; _kx_arm_len[0] = ");
    ros2_expr(gen, node->data.arm_attach.len1);
    codegen_emit(gen, "; _kx_arm_len[1] = ");
    ros2_expr(gen, node->data.arm_attach.len2);
    codegen_emit(gen, "; _kx_arm_len[2] = ");
    ros2_expr(gen, node->data.arm_attach.len3);
    codegen_emit(gen, ";\n");
    break;
  case NODE_ARM_MOVE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_arm_ik(");
    ros2_expr(gen, node->data.arm_move.x);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.arm_move.y);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.arm_move.z);
    codegen_emit(gen, ");\n");
    break;
  case NODE_GRID_CREATE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_grid_w = (int)");
    ros2_expr(gen, node->data.grid_create.width);
    codegen_emit(gen, "; _kx_grid_h = (int)");
    ros2_expr(gen, node->data.grid_create.height);
    codegen_emit(gen, "; memset(_kx_grid, 0, sizeof(_kx_grid));\n");
    break;
  case NODE_GRID_OBSTACLE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_grid[(int)");
    ros2_expr(gen, node->data.grid_obstacle.y);
    codegen_emit(gen, "][(int)");
    ros2_expr(gen, node->data.grid_obstacle.x);
    codegen_emit(gen, "] = 1;\n");
    break;
  case NODE_PATH_COMPUTE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_path_compute(");
    ros2_expr(gen, node->data.path_compute.from_x);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.path_compute.from_y);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.path_compute.to_x);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.path_compute.to_y);
    codegen_emit(gen, ");\n");
    break;
  case NODE_DRONE_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_drone_pins[0] = ");
    ros2_expr(gen, node->data.drone_attach.fl);
    codegen_emit(gen, "; _kx_drone_pins[1] = ");
    ros2_expr(gen, node->data.drone_attach.fr);
    codegen_emit(gen, "; _kx_drone_pins[2] = ");
    ros2_expr(gen, node->data.drone_attach.bl);
    codegen_emit(gen, "; _kx_drone_pins[3] = ");
    ros2_expr(gen, node->data.drone_attach.br);
    codegen_emit(gen, ";\n");
    break;
  case NODE_DRONE_SET:
    codegen_emit_indent(gen);
    codegen_emit(gen, "_kx_drone_mix(");
    ros2_expr(gen, node->data.drone_set.pitch);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.drone_set.roll);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.drone_set.yaw);
    codegen_emit(gen, ", ");
    ros2_expr(gen, node->data.drone_set.throttle);
    codegen_emit(gen, ");\n");
    break;

  /* Wave 3: Communication Statements */
  case NODE_BLE_ENABLE:
    codegen_emit_indent(gen);
    codegen_emit(
        gen,
        "RCLCPP_INFO(this->get_logger(), \"BLE Enable: %%s\", std::string(");
    ros2_expr(gen, node->data.ble_enable.name);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_BLE_ADVERTISE:
    codegen_emit_indent(gen);
    codegen_emit(
        gen,
        "RCLCPP_INFO(this->get_logger(), \"BLE Advertise: %%s\", std::string(");
    ros2_expr(gen, node->data.ble_advertise.data);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_BLE_SEND:
    codegen_emit_indent(gen);
    codegen_emit(
        gen, "RCLCPP_INFO(this->get_logger(), \"BLE Send: %%s\", std::string(");
    ros2_expr(gen, node->data.ble_send.data);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_WIFI_CONNECT:
    codegen_emit_indent(gen);
    codegen_emit(
        gen,
        "RCLCPP_INFO(this->get_logger(), \"WiFi Connect: %%s\", std::string(");
    ros2_expr(gen, node->data.wifi_connect.ssid);
    codegen_emit(gen, ").c_str());\n");
    codegen_emit_line(gen, "wifi_ip_ = \"127.0.0.1\";");
    break;
  case NODE_MQTT_CONNECT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "RCLCPP_INFO(this->get_logger(), \"MQTT Connect to: "
                      "%%s:%%d\", std::string(");
    ros2_expr(gen, node->data.mqtt_connect.broker);
    codegen_emit(gen, ").c_str(), (int)(");
    ros2_expr(gen, node->data.mqtt_connect.port);
    codegen_emit(gen, "));\n");
    break;
  case NODE_MQTT_SUBSCRIBE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "RCLCPP_INFO(this->get_logger(), \"MQTT Subscribe: "
                      "%%s\", std::string(");
    ros2_expr(gen, node->data.mqtt_subscribe.topic);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_MQTT_PUBLISH:
    codegen_emit_indent(gen);
    codegen_emit(gen, "RCLCPP_INFO(this->get_logger(), \"MQTT Publish to %%s: "
                      "%%s\", std::string(");
    ros2_expr(gen, node->data.mqtt_publish.topic);
    codegen_emit(gen, ").c_str(), std::string(");
    ros2_expr(gen, node->data.mqtt_publish.payload);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_HTTP_POST:
    codegen_emit_indent(gen);
    codegen_emit(gen, "RCLCPP_INFO(this->get_logger(), \"HTTP POST to %%s: "
                      "%%s\", std::string(");
    ros2_expr(gen, node->data.http_post.url);
    codegen_emit(gen, ").c_str(), std::string(");
    ros2_expr(gen, node->data.http_post.body);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_WS_CONNECT:
    codegen_emit_indent(gen);
    codegen_emit(
        gen,
        "RCLCPP_INFO(this->get_logger(), \"WS Connect to %%s\", std::string(");
    ros2_expr(gen, node->data.ws_connect.url);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_WS_SEND:
    codegen_emit_indent(gen);
    codegen_emit(
        gen, "RCLCPP_INFO(this->get_logger(), \"WS Send: %%s\", std::string(");
    ros2_expr(gen, node->data.ws_send.data);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_WS_CLOSE:
    codegen_emit_indent(gen);
    codegen_emit(gen, "RCLCPP_INFO(this->get_logger(), \"WS Close\");\n");
    break;

  /* Wave 4: Advanced Robotics & Storage Statements */
  case NODE_IMU_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit_line(gen, "RCLCPP_INFO(this->get_logger(), \"IMU Attach\");");
    break;

  case NODE_GPS_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit(
        gen,
        "RCLCPP_INFO(this->get_logger(), \"GPS Attach (Baud: %d)\", (int)(");
    ros2_expr(gen, node->data.gps_attach.baud);
    codegen_emit(gen, "));\n");
    break;

  case NODE_LIDAR_ATTACH:
    codegen_emit_indent(gen);
    codegen_emit_line(gen,
                      "RCLCPP_INFO(this->get_logger(), \"Lidar Attach\");");
    break;

  case NODE_SD_MOUNT:
    codegen_emit_indent(gen);
    codegen_emit(
        gen, "RCLCPP_INFO(this->get_logger(), \"SD Mount (CS: %d)\", (int)(");
    ros2_expr(gen, node->data.sd_mount.cs_pin);
    codegen_emit(gen, "));\n");
    break;

  case NODE_FILE_OPEN:
    codegen_emit_indent(gen);
    codegen_emit(
        gen, "RCLCPP_INFO(this->get_logger(), \"File Open: %s\", std::string(");
    ros2_expr(gen, node->data.file_open.filename);
    codegen_emit(gen, ").c_str());\n");
    break;

  case NODE_FILE_WRITE:
    codegen_emit_indent(gen);
    codegen_emit(
        gen,
        "RCLCPP_INFO(this->get_logger(), \"File Write: %s\", std::string(");
    ros2_expr(gen, node->data.file_write.data);
    codegen_emit(gen, ").c_str());\n");
    break;

  case NODE_FILE_CLOSE:
    codegen_emit_indent(gen);
    codegen_emit_line(gen, "RCLCPP_INFO(this->get_logger(), \"File Close\");");
    break;

  /* Wave 5 Statements */
  case NODE_OLED_ATTACH:
    codegen_emit_line(gen, "    RCLCPP_INFO(this->get_logger(), \"OLED Attach\");");
    break;
  case NODE_OLED_PRINT:
    codegen_emit_indent(gen);
    codegen_emit(gen, "    RCLCPP_INFO(this->get_logger(), \"OLED Print: %%s\", std::string(");
    ros2_expr(gen, node->data.oled_print.text);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_OLED_DRAW:
    codegen_emit_line(gen, "    RCLCPP_INFO(this->get_logger(), \"OLED Draw\");");
    break;
  case NODE_OLED_SHOW:
    codegen_emit_line(gen, "    RCLCPP_INFO(this->get_logger(), \"OLED Show\");");
    break;
  case NODE_OLED_CLEAR:
    codegen_emit_line(gen, "    RCLCPP_INFO(this->get_logger(), \"OLED Clear\");");
    break;
  case NODE_AUDIO_ATTACH:
    codegen_emit_line(gen, "    RCLCPP_INFO(this->get_logger(), \"Audio Attach\");");
    break;
  case NODE_PLAY_FREQ:
    codegen_emit_indent(gen);
    codegen_emit(gen, "    RCLCPP_INFO(this->get_logger(), \"Play Freq: %%f\", (double)(");
    ros2_expr(gen, node->data.play_freq.frequency);
    codegen_emit(gen, "));\n");
    break;
  case NODE_PLAY_SOUND:
    codegen_emit_indent(gen);
    codegen_emit(gen, "    RCLCPP_INFO(this->get_logger(), \"Play Sound: %%s\", std::string(");
    ros2_expr(gen, node->data.play_sound.name);
    codegen_emit(gen, ").c_str());\n");
    break;
  case NODE_SET_VOLUME:
    codegen_emit_indent(gen);
    codegen_emit(gen, "    audio_volume_ = (int)(");
    ros2_expr(gen, node->data.set_volume.level);
    codegen_emit(gen, ");\n");
    break;
  case NODE_CAM_ATTACH:
    codegen_emit_line(gen, "    RCLCPP_INFO(this->get_logger(), \"Camera Attach\");");
    break;

  default:
    break;
  }
}

void codegen_generate_ros2(CodeGen *gen, ASTNode *program) {
  if (!program || program->type != NODE_PROGRAM)
    return;

  codegen_emit_line(gen, "// Generated by Kinetrix Compiler (Target: ROS2)");
  codegen_emit_line(
      gen, "// Build: Place in a ROS2 package src/ and run: colcon build");
  codegen_emit_line(gen, "// Run:   ros2 run <your_pkg> kinetrix_node\n");
  codegen_emit_line(gen, "#include <rclcpp/rclcpp.hpp>");
  codegen_emit_line(gen, "#include <std_msgs/msg/bool.hpp>");
  codegen_emit_line(gen, "#include <std_msgs/msg/float64.hpp>");
  codegen_emit_line(gen, "#include <std_msgs/msg/string.hpp>");
  codegen_emit_line(gen, "#include <chrono>");
  codegen_emit_line(gen, "#include <algorithm>  // std::min, std::max");
  codegen_emit_line(gen, "#include <cmath>\n");
  codegen_emit_line(gen, "using namespace std::chrono_literals;\n");

  /* Standalone helper functions before the class */
  ASTNode *block = program->data.program.main_block;
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *s = block->data.block.statements[i];
      if (s && s->type == NODE_FUNCTION_DEF) {
        if (s->data.function_def.is_extern) {
          codegen_emit_line(gen, "extern double %s(",
                            s->data.function_def.name);
          for (int j = 0; j < s->data.function_def.param_count; j++) {
            if (j > 0)
              codegen_emit(gen, ", ");
            codegen_emit(gen, "double %s", s->data.function_def.param_names[j]);
          }
          codegen_emit(gen, ");\n\n");
          continue;
        }
        ros2_stmt(gen, s);
      }
    }
  }

  /* ROS2 Node class */
  codegen_emit_line(gen, "class KinetrixNode : public rclcpp::Node {");
  codegen_emit_line(gen, "public:");
  gen->indent_level++;
  codegen_emit_line(gen, "KinetrixNode() : Node(\"kinetrix_node\") {");
  gen->indent_level++;
  codegen_emit_line(
      gen,
      "gpio_pub_ = create_publisher<std_msgs::msg::Bool>(\"/gpio/out\", 10);");
  codegen_emit_line(
      gen, "pwm_pub_  = create_publisher<std_msgs::msg::Float64>(\"/pwm/out\", "
           "10);");
  codegen_emit_line(
      gen,
      "stepper_pub_ = "
      "create_publisher<std_msgs::msg::Float64>(\"/kinetrix/stepper\", 10);");
  codegen_emit_line(
      gen,
      "motor_pub_ = "
      "create_publisher<std_msgs::msg::Float64>(\"/kinetrix/motor\", 10);");
  codegen_emit_line(
      gen, "esc_pub_ = "
           "create_publisher<std_msgs::msg::Float64>(\"/kinetrix/esc\", 10);");

  /* Wave 6 Publishers */
  codegen_emit_line(gen, "mec_fl_pub_ = create_publisher<std_msgs::msg::Float64>(\"/kinetrix/mecanum_fl\", 10);");
  codegen_emit_line(gen, "mec_fr_pub_ = create_publisher<std_msgs::msg::Float64>(\"/kinetrix/mecanum_fr\", 10);");
  codegen_emit_line(gen, "mec_bl_pub_ = create_publisher<std_msgs::msg::Float64>(\"/kinetrix/mecanum_bl\", 10);");
  codegen_emit_line(gen, "mec_br_pub_ = create_publisher<std_msgs::msg::Float64>(\"/kinetrix/mecanum_br\", 10);");
  codegen_emit_line(gen, "// Timer drives the main robot loop");
  codegen_emit_line(gen, "timer_ = create_wall_timer(10ms, "
                         "std::bind(&KinetrixNode::loop, this));");
  gen->indent_level--;
  codegen_emit_line(gen, "}\n");

  /* Declare member variables for pins */
  codegen_emit_line(gen, "private:");
  codegen_emit_line(
      gen, "  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr gpio_pub_;");
  codegen_emit_line(
      gen, "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pwm_pub_;");
  codegen_emit_line(
      gen,
      "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr stepper_pub_;");
  codegen_emit_line(
      gen,
      "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr motor_pub_;");
  codegen_emit_line(
      gen, "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr esc_pub_;");

  /* Wave 6 Globals */
  codegen_emit_line(gen, "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr mec_fl_pub_;");
  codegen_emit_line(gen, "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr mec_fr_pub_;");
  codegen_emit_line(gen, "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr mec_bl_pub_;");
  codegen_emit_line(gen, "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr mec_br_pub_;");

  codegen_emit_line(gen, "  rclcpp::TimerBase::SharedPtr timer_;");
  codegen_emit_line(gen, "  double sensor_val_0_ = 0.0, pin_state_0_ = 0.0;");

  /* Wave 6 Kalman Globals */
  codegen_emit_line(gen, "  double kalman_q_ = 0.01;");
  codegen_emit_line(gen, "  double kalman_r_ = 0.1;");
  codegen_emit_line(gen, "  double kalman_x_ = 0.0;");
  codegen_emit_line(gen, "  double kalman_p_ = 1.0;");
  codegen_emit_line(gen, "  double kalman_k_ = 0.0;\n");

  /* Wave 2 Globals */
  codegen_emit_line(gen, "  double stepper_speed_ = 100.0;");
  codegen_emit_line(gen, "  double encoder_count_ = 0.0;");
  codegen_emit_line(gen,
                    "  double pid_kp_ = 0.0, pid_ki_ = 0.0, pid_kd_ = 0.0;");
  codegen_emit_line(gen, "  double pid_setpoint_ = 0.0, pid_last_err_ = 0.0, "
                         "pid_integral_ = 0.0;");
  codegen_emit_line(gen, "  rclcpp::Time pid_last_time_;");

  /* Wave 3 Globals */
  codegen_emit_line(gen, "  std::string ble_msg_ = \"\";");
  codegen_emit_line(gen, "  std::string mqtt_msg_ = \"\";");
  codegen_emit_line(gen, "  std::string ws_msg_ = \"\";");
  codegen_emit_line(gen, "  std::string wifi_ip_ = \"\";\n");

  /* Wave 4 Globals */
  codegen_emit_line(
      gen,
      "  double imu_ax_ = 0.0, imu_ay_ = 0.0, imu_az_ = 0.0, imu_head_ = 0.0;");
  codegen_emit_line(gen, "  double gps_lat_ = 0.0, gps_lon_ = 0.0, gps_alt_ = "
                         "0.0, gps_spd_ = 0.0;");
  codegen_emit_line(gen, "  double lidar_dist_ = 0.0;\n");

  /* Wave 5 Globals */
  codegen_emit_line(gen, "  bool cam_detect_ = false;");
  codegen_emit_line(gen, "  double cam_obj_x_ = 0.0, cam_obj_y_ = 0.0;");
  codegen_emit_line(gen, "  int audio_volume_ = 100;\n");
  /* PID Helper Function */
  codegen_emit_line(gen, "  void compute_pid(double current_val) {");
  codegen_emit_line(gen, "    auto now = this->now();");
  codegen_emit_line(gen, "    double dt = (now - pid_last_time_).seconds();");
  codegen_emit_line(gen, "    if (dt <= 0.0) dt = 0.001;");
  codegen_emit_line(gen, "    double err = pid_setpoint_ - current_val;");
  codegen_emit_line(gen, "    pid_integral_ += err * dt;");
  codegen_emit_line(gen, "    double deriv = (err - pid_last_err_) / dt;");
  codegen_emit_line(gen, "    double out = (pid_kp_ * err) + (pid_ki_ * "
                         "pid_integral_) + (pid_kd_ * deriv);");
  codegen_emit_line(gen, "    pid_last_err_ = err;");
  codegen_emit_line(gen, "    pid_last_time_ = now;");
  codegen_emit_line(gen, "    // Output handling requires user application to "
                         "subscribe/route Kinetrix state");
  codegen_emit_line(gen, "  }\n");

  /* Kalman Helper Function */
  codegen_emit_line(gen, "  double compute_kalman_update(double mea) {");
  codegen_emit_line(gen, "    kalman_p_ = kalman_p_ + kalman_q_;");
  codegen_emit_line(gen, "    kalman_k_ = kalman_p_ / (kalman_p_ + kalman_r_);");
  codegen_emit_line(gen, "    kalman_x_ = kalman_x_ + kalman_k_ * (mea - kalman_x_);");
  codegen_emit_line(gen, "    kalman_p_ = (1.0 - kalman_k_) * kalman_p_;");
  codegen_emit_line(gen, "    return kalman_x_;");
  codegen_emit_line(gen, "  }\n");

  /* Wave 6 AI Helpers */
  codegen_emit_line(gen, "  double _kx_ai_invoke(double input) {");
  codegen_emit_line(gen, "    return 0.0; // TFLite Micro omitted for generic target");
  codegen_emit_line(gen, "  }");
  codegen_emit_line(gen, "  double _kx_ai_invoke(double* input_array) {");
  codegen_emit_line(gen, "    return 0.0;");
  codegen_emit_line(gen, "  }\n");

  /* Wave 7 ROS2 Globals & Helpers */
  codegen_emit_line(gen, "  int _kx_arm_dof = 3; double _kx_arm_len[4] = {0,0,0,0}; double _kx_arm_angles[4] = {0,0,0,0};");
  codegen_emit_line(gen, "  void _kx_arm_ik(double tx, double ty, double tz) {");
  codegen_emit_line(gen, "    double r=sqrt(tx*tx+ty*ty), d=sqrt(r*r+tz*tz), L1=_kx_arm_len[0], L2=_kx_arm_len[1];");
  codegen_emit_line(gen, "    double ca=(d*d-L1*L1-L2*L2)/(2.0*L1*L2); if(ca<-1)ca=-1; if(ca>1)ca=1;");
  codegen_emit_line(gen, "    _kx_arm_angles[1]=acos(ca); _kx_arm_angles[0]=atan2(tz,r)-atan2(L2*sin(_kx_arm_angles[1]),L1+L2*ca);");
  codegen_emit_line(gen, "    _kx_arm_angles[2]=atan2(ty,tx);");
  codegen_emit_line(gen, "  }\n");
  codegen_emit_line(gen, "  int _kx_grid_w=0,_kx_grid_h=0; int _kx_grid[64][64]; int _kx_path_result[256]; int _kx_path_len=0;");
  codegen_emit_line(gen, "  int _kx_path_compute(int sx,int sy,int gx,int gy) {");
  codegen_emit_line(gen, "    _kx_path_len=0; if(sx==gx&&sy==gy)return 0;");
  codegen_emit_line(gen, "    int v[64][64]; memset(v,0,sizeof(v)); int qx[4096],qy[4096],qp[4096]; int qf=0,qb=0;");
  codegen_emit_line(gen, "    qx[qb]=sx;qy[qb]=sy;qp[qb]=-1;qb++;v[sy][sx]=1; int dx[]={1,-1,0,0},dy[]={0,0,1,-1};");
  codegen_emit_line(gen, "    while(qf<qb){int cx=qx[qf],cy=qy[qf],cp=qf;qf++;");
  codegen_emit_line(gen, "      if(cx==gx&&cy==gy){int t=cp;while(t!=-1){_kx_path_result[_kx_path_len++]=qx[t]*100+qy[t];t=qp[t];}return _kx_path_len;}");
  codegen_emit_line(gen, "      for(int i=0;i<4;i++){int nx=cx+dx[i],ny=cy+dy[i];");
  codegen_emit_line(gen, "        if(nx>=0&&nx<_kx_grid_w&&ny>=0&&ny<_kx_grid_h&&!v[ny][nx]&&!_kx_grid[ny][nx]){v[ny][nx]=1;qx[qb]=nx;qy[qb]=ny;qp[qb]=cp;qb++;}}}");
  codegen_emit_line(gen, "    return 0;");
  codegen_emit_line(gen, "  }\n");
  codegen_emit_line(gen, "  int _kx_drone_pins[4] = {-1,-1,-1,-1};");
  codegen_emit_line(gen, "  void _kx_drone_mix(double pitch,double roll,double yaw,double throttle) {");
  codegen_emit_line(gen, "    RCLCPP_INFO(this->get_logger(), \"Drone mix: p=%.1f r=%.1f y=%.1f t=%.1f\", pitch, roll, yaw, throttle);");
  codegen_emit_line(gen, "  }\n");

  codegen_emit_line(gen, "  void loop() {");
  gen->indent_level++;
  if (block && block->type == NODE_BLOCK) {
    for (int i = 0; i < block->data.block.statement_count; i++) {
      ASTNode *s = block->data.block.statements[i];
      if (s && s->type != NODE_FUNCTION_DEF)
        ros2_stmt(gen, s);
    }
  } else if (block) {
    ros2_stmt(gen, block);
  }
  gen->indent_level--;
  codegen_emit_line(gen, "  }");
  gen->indent_level--;
  codegen_emit_line(gen, "};\n");

  codegen_emit_line(gen, "int main(int argc, char **argv) {");
  codegen_emit_line(gen, "  rclcpp::init(argc, argv);");
  codegen_emit_line(gen, "  rclcpp::spin(std::make_shared<KinetrixNode>());");
  codegen_emit_line(gen, "  rclcpp::shutdown();");
  codegen_emit_line(gen, "  return 0;");
  codegen_emit_line(gen, "}");
}
