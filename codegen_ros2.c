/* Kinetrix ROS2 Code Generator
 * Target: ROS2 (Robot Operating System 2) C++ Node
 * Output: .cpp file — a complete rclcpp node
 * Build with: colcon build inside a ROS2 workspace
 * Run with:   ros2 run <pkg> <node_name>
 */

#include "codegen.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static void ros2_expr(CodeGen *gen, ASTNode *node);
static void ros2_stmt(CodeGen *gen, ASTNode *node);

static void ros2_expr(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_NUMBER:     codegen_emit(gen, "%g", node->data.number.value); break;
        case NODE_STRING:     codegen_emit(gen, "std::string(\"%s\")", node->data.string.value); break;
        case NODE_BOOL:       codegen_emit(gen, "%s", node->data.boolean.value ? "true" : "false"); break;
        case NODE_IDENTIFIER: codegen_emit(gen, "%s_", node->data.identifier.name); break;
        case NODE_BINARY_OP: {
            const char *op = "+";
            switch (node->data.binary_op.op) {
                case OP_ADD: op="+"; break; case OP_SUB: op="-"; break;
                case OP_MUL: op="*"; break; case OP_DIV: op="/"; break;
                case OP_MOD: op="%"; break; case OP_EQ: op="=="; break;
                case OP_NEQ: op="!="; break; case OP_LT: op="<"; break;
                case OP_GT: op=">"; break; case OP_LTE: op="<="; break;
                case OP_GTE: op=">="; break; case OP_AND: op="&&"; break;
                case OP_OR:  op="||"; break; default: break;
            }
            codegen_emit(gen, "(");
            ros2_expr(gen, node->data.binary_op.left);
            codegen_emit(gen, " %s ", op);
            ros2_expr(gen, node->data.binary_op.right);
            codegen_emit(gen, ")");
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
                ros2_expr(gen, node->data.call.args[0]); codegen_emit(gen, " - ");
                ros2_expr(gen, node->data.call.args[1]); codegen_emit(gen, ") * (");
                ros2_expr(gen, node->data.call.args[4]); codegen_emit(gen, " - ");
                ros2_expr(gen, node->data.call.args[3]); codegen_emit(gen, ") / (");
                ros2_expr(gen, node->data.call.args[2]); codegen_emit(gen, " - ");
                ros2_expr(gen, node->data.call.args[1]); codegen_emit(gen, ") + ");
                ros2_expr(gen, node->data.call.args[3]); codegen_emit(gen, ")");
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
                    if (i > 0) codegen_emit(gen, ", ");
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
        default: codegen_emit(gen, "0.0"); break;
    }
}

static void ros2_stmt(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_VAR_DECL:
            codegen_emit_line(gen, "double %s_", node->data.var_decl.name);
            if (node->data.var_decl.initializer) { codegen_emit(gen, " = "); ros2_expr(gen, node->data.var_decl.initializer); }
            else codegen_emit(gen, " = 0.0");
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
        case NODE_FOREVER:
            /* In ROS2, forever loops become timer callbacks */
            codegen_emit_line(gen, "/* loop forever → ROS2 timer callback */\n");
            ros2_stmt(gen, node->data.forever_loop.body);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++)
                ros2_stmt(gen, node->data.block.statements[i]);
            break;
        case NODE_RETURN:
            codegen_emit_indent(gen); codegen_emit(gen, "return");
            if (node->data.return_stmt.value) { codegen_emit(gen, " "); ros2_expr(gen, node->data.return_stmt.value); }
            codegen_emit(gen, ";\n");
            break;
        case NODE_BREAK:
            codegen_emit_line(gen, "break;\n");
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
            codegen_emit(gen, "RCLCPP_INFO(get_logger(), \"%%s\", std::to_string(");
            ros2_expr(gen, node->data.unary.child);
            codegen_emit(gen, ").c_str());\n");
            break;
        case NODE_CALL:
            codegen_emit_indent(gen); ros2_expr(gen, node); codegen_emit(gen, ";\n");
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
                if (i > 0) codegen_emit(gen, ", ");
                codegen_emit(gen, "double %s", node->data.function_def.param_names[i]);
            }
            codegen_emit(gen, ") {\n");
            gen->indent_level++;
            ros2_stmt(gen, node->data.function_def.body);
            gen->indent_level--;
            codegen_emit_line(gen, "}\n");
            break;
        default: break;
    }
}

void codegen_generate_ros2(CodeGen *gen, ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;

    codegen_emit_line(gen, "// Generated by Kinetrix Compiler (Target: ROS2)");
    codegen_emit_line(gen, "// Build: Place in a ROS2 package src/ and run: colcon build");
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
                    codegen_emit_line(gen, "extern double %s(", s->data.function_def.name);
                    for (int j = 0; j < s->data.function_def.param_count; j++) {
                        if (j > 0) codegen_emit(gen, ", ");
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
    codegen_emit_line(gen, "gpio_pub_ = create_publisher<std_msgs::msg::Bool>(\"/gpio/out\", 10);");
    codegen_emit_line(gen, "pwm_pub_  = create_publisher<std_msgs::msg::Float64>(\"/pwm/out\", 10);");
    codegen_emit_line(gen, "// Timer drives the main robot loop");
    codegen_emit_line(gen, "timer_ = create_wall_timer(10ms, std::bind(&KinetrixNode::loop, this));");
    gen->indent_level--;
    codegen_emit_line(gen, "}\n");

    /* Declare member variables for pins */
    codegen_emit_line(gen, "private:");
    codegen_emit_line(gen, "  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr gpio_pub_;");
    codegen_emit_line(gen, "  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pwm_pub_;");
    codegen_emit_line(gen, "  rclcpp::TimerBase::SharedPtr timer_;");
    codegen_emit_line(gen, "  double sensor_val_0_ = 0.0, pin_state_0_ = 0.0;\n");

    codegen_emit_line(gen, "  void loop() {");
    gen->indent_level++;
    if (block && block->type == NODE_BLOCK) {
        for (int i = 0; i < block->data.block.statement_count; i++) {
            ASTNode *s = block->data.block.statements[i];
            if (s && s->type != NODE_FUNCTION_DEF) ros2_stmt(gen, s);
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
