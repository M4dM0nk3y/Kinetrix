/* Kinetrix V3.1 Multi-Target Compiler - Main Driver
 * Supports: Arduino, ESP32, Raspberry Pi, Pico (MicroPython), ROS2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "error.h"
#include "parser.h"
#include "codegen.h"
#include "pin_tracker.h"
#include <unistd.h>
#include <dirent.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Kinetrix V3.1 Multi-Target Compiler\n");
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "Usage: %s <source.kx> [-o output] [--target <target>]\n\n", prog);
    fprintf(stderr, "Targets:\n");
    fprintf(stderr, "  arduino  (default)  Arduino Uno/Mega/Nano    → .ino\n");
    fprintf(stderr, "  esp32               ESP32 / ESP8266          → .cpp\n");
    fprintf(stderr, "  rpi                 Raspberry Pi (Python)    → .py\n");
    fprintf(stderr, "  pico                Raspberry Pi Pico        → .py\n");
    fprintf(stderr, "  ros2                ROS2 C++ Node            → .cpp\n\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s robot.kx                           # Arduino (default)\n", prog);
    fprintf(stderr, "  %s robot.kx --target esp32 -o out.cpp\n", prog);
    fprintf(stderr, "  %s robot.kx --target rpi   -o out.py\n", prog);
    fprintf(stderr, "  %s robot.kx --target pico  -o out.py\n", prog);
    fprintf(stderr, "  %s robot.kx --target ros2  -o node.cpp\n", prog);
}

static Target parse_target(const char *name) {
    if (strcmp(name, "arduino") == 0) return TARGET_ARDUINO;
    if (strcmp(name, "esp32")   == 0) return TARGET_ESP32;
    if (strcmp(name, "rpi")     == 0) return TARGET_RPI;
    if (strcmp(name, "pico")    == 0) return TARGET_PICO;
    if (strcmp(name, "ros2")    == 0) return TARGET_ROS2;
    fprintf(stderr, "Error: Unknown target '%s'\n", name);
    fprintf(stderr, "Valid targets: arduino, esp32, rpi, pico, ros2\n");
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 2) { print_usage(argv[0]); return 1; }

    const char *input_file  = argv[1];
    const char *output_file = NULL;
    Target       target      = TARGET_ARDUINO;
    int          diagnostics = 0;

    // Parse command-line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            target = parse_target(argv[++i]);
        } else if (strcmp(argv[i], "--diagnostics") == 0) {
            diagnostics = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]); return 0;
        }
    }

    // Default output filename based on target
    char default_out[256];
    if (!output_file) {
        const char *ext = target_extension(target);
        snprintf(default_out, sizeof(default_out), "Kinetrix_Output%s", ext);
        output_file = default_out;
    }

    printf("Kinetrix V3.1 Multi-Target Compiler\n");
    printf("=====================================\n");
    printf("Input:  %s\n", input_file);
    printf("Output: %s\n", output_file);
    printf("Target: %s\n\n", target_name(target));

    // Pre-process: merge kpm dependencies and main file
    char merged_path[256];
    snprintf(merged_path, sizeof(merged_path), "/tmp/kinetrix_build_%d.kx", getpid());
    FILE *merged_file = fopen(merged_path, "w");
    if (!merged_file) {
        fprintf(stderr, "Error: Could not create temp build file\n");
        return 1;
    }
    
    // Auto-include installed packages if they exist
    DIR *dir = opendir("kinetrix_modules");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            char mod_path[512];
            snprintf(mod_path, sizeof(mod_path), "kinetrix_modules/%s/index.kx", ent->d_name);
            FILE *mod_file = fopen(mod_path, "r");
            if (mod_file) {
                char ch;
                while ((ch = fgetc(mod_file)) != EOF) fputc(ch, merged_file);
                fputc('\n', merged_file);
                fclose(mod_file);
            }
        }
        closedir(dir);
    }
    
    // Append the main user file
    FILE *main_file = fopen(input_file, "r");
    if (!main_file) {
        fprintf(stderr, "Error: Could not open '%s'\n", input_file);
        fclose(merged_file);
        remove(merged_path);
        return 1;
    }
    char ch;
    while ((ch = fgetc(main_file)) != EOF) fputc(ch, merged_file);
    fclose(main_file);
    fclose(merged_file);

    // Open merged source file for Lexer/Parser
    FILE *source = fopen(merged_path, "r");
    if (!source) {
        fprintf(stderr, "Error: Could not read merged build file\n");
        return 1;
    }

    // Parse and build AST
    ErrorList *errors  = error_list_create(10);
    printf("Parsing...\n");
    Parser    *parser  = parser_create(source, errors);
    ASTNode   *program = parser_parse(parser);
    fclose(source);

    if (errors->count > 0) {
        fprintf(stderr, "\nCompilation failed with %d error(s):\n", errors->count);
        Error *err = errors->head;
        while (err) {
            fprintf(stderr, "  Line %d, Col %d: %s\n", err->line, err->column, err->message);
            err = err->next;
        }
        parser_free(parser);
        error_list_free(errors);
        return 1;
    }
    printf("✓ Parsing successful\n");

    if (diagnostics) {
        ast_track_pins(program);
        if (program->data.program.pin_count > 0)
            printf("Found %d GPIO pins\n", program->data.program.pin_count);
    }

    // Open output file
    FILE *output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Could not open output '%s'\n", output_file);
        parser_free(parser);
        error_list_free(errors);
        return 1;
    }

    // Generate code for target
    printf("Generating %s code...\n", target_name(target));
    CodeGen *gen = codegen_create_for_target(output, target);
    codegen_generate(gen, program);
    codegen_free(gen);
    fclose(output);

    printf("✓ Code generation successful\n\n");

    ast_free(program);
    parser_free(parser);
    error_list_free(errors);

    printf("✓ Compilation successful!\n");
    printf("Generated: %s\n\n", output_file);
    
    // Cleanup Temp File
    remove(merged_path);

    // Target-specific upload instructions
    switch (target) {
        case TARGET_ARDUINO:
            printf("Next steps:\n");
            printf("  Open %s in Arduino IDE → select board → Upload\n", output_file);
            printf("  OR: arduino-cli compile --fqbn arduino:avr:uno .\n");
            break;
        case TARGET_ESP32:
            printf("Next steps:\n");
            printf("  Open %s in Arduino IDE with ESP32 board package\n", output_file);
            printf("  Select: Tools → Board → ESP32 Dev Module → Upload\n");
            break;
        case TARGET_RPI:
            printf("Next steps:\n");
            printf("  pip install RPi.GPIO Adafruit-MCP3008\n");
            printf("  python3 %s\n", output_file);
            break;
        case TARGET_PICO:
            printf("Next steps:\n");
            printf("  Install MicroPython on your Pico first\n");
            printf("  Then: mpremote copy %s :main.py\n", output_file);
            printf("  OR:   Open in Thonny IDE → Run\n");
            break;
        case TARGET_ROS2:
            printf("Next steps:\n");
            printf("  Place %s in your ROS2 package src/\n", output_file);
            printf("  colcon build && ros2 run <pkg> kinetrix_node\n");
            break;
    }
    return 0;
}
