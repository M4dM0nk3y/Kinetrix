#!/bin/bash

# Kinetrix Comprehensive Test Suite
# Compiles all available .kx examples against all 5 target environments.

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "Starting Kinetrix CI Suite..."
make clean > /dev/null 2>&1
make > /dev/null

TARGETS=("arduino" "esp32" "rpi" "pico" "ros2")
EXAMPLES=$(ls examples/*.kx 2>/dev/null || true)
EXAMPLES+=" wave3_test.kx" # Fallback if examples don't exist

if [ -z "$EXAMPLES" ]; then
    echo "No examples found to test!"
    exit 1
fi

total_tests=0
passed_tests=0

for file in $EXAMPLES; do
    if [ ! -f "$file" ]; then continue; fi
    
    echo "Testing $file..."
    for target in "${TARGETS[@]}"; do
        total_tests=$((total_tests + 1))
        
        # Capture stderr and stdout
        if ./kcc "$file" -t "$target" > /dev/null 2>&1; then
            echo -e "  [${GREEN}PASS${NC}] $target"
            passed_tests=$((passed_tests + 1))
        else
            echo -e "  [${RED}FAIL${NC}] $target"
            # Run again without output redirect to show the error
            ./kcc "$file" -t "$target" || true
            exit 1 # Fail fast on first error
        fi
    done
done

echo ""
echo -e "${GREEN}All systems go! $passed_tests / $total_tests tests passed.${NC}"
rm -f Kinetrix_Output.*
