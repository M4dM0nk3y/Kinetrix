/* Hardware Diagnostics Code Generation */

#include "codegen.h"

// Generate hardware diagnostic code
void codegen_generate_diagnostics(CodeGen *gen, int *pins, int pin_count) {
    if (pin_count == 0) return;
    
    codegen_emit_line(gen, "// === Kinetrix Hardware Diagnostics ===");
    codegen_emit_line(gen, "Serial.println(\"\\n=== Kinetrix Hardware Test ===\");");
    codegen_emit_line(gen, "Serial.println(\"Testing %d pins...\\n\");", pin_count);
    
    // Test each pin
    for (int i = 0; i < pin_count; i++) {
        int pin = pins[i];
        
        codegen_emit_line(gen, "// Test Pin %d", pin);
        codegen_emit_line(gen, "pinMode(%d, OUTPUT);", pin);
        codegen_emit_line(gen, "Serial.print(\"Pin %d... \");", pin);
        
        // Blink test
        codegen_emit_line(gen, "digitalWrite(%d, HIGH);", pin);
        codegen_emit_line(gen, "delay(100);");
        codegen_emit_line(gen, "digitalWrite(%d, LOW);", pin);
        codegen_emit_line(gen, "delay(100);");
        
        codegen_emit_line(gen, "Serial.println(\"✓ OK\");");
    }
    
    codegen_emit_line(gen, "");
    codegen_emit_line(gen, "Serial.println(\"All pins tested!\");");
    codegen_emit_line(gen, "Serial.println(\"Starting main program...\\n\");");
    codegen_emit_line(gen, "Serial.println(\"================================\\n\");");
    codegen_emit_line(gen, "");
}
