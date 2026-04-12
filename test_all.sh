#!/bin/bash

# Kinetrix Comprehensive Test Suite
# Compiles all available .kx examples against all 5 target environments.
# Runs ALL tests and reports a complete summary, even if some fail.

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
failed_tests=()

for file in $EXAMPLES; do
    if [ ! -f "$file" ]; then continue; fi
    
    echo "Testing $file..."
    for target in "${TARGETS[@]}"; do
        total_tests=$((total_tests + 1))
        
        # Use target-specific output file to prevent overwrite conflicts
        outfile="/tmp/kx_ci_${target}_$$"
        if ./kcc "$file" -t "$target" -o "$outfile" > /dev/null 2>&1; then
            echo -e "  [${GREEN}PASS${NC}] $target"
            passed_tests=$((passed_tests + 1))
        else
            echo -e "  [${RED}FAIL${NC}] $target"
            failed_tests+=("$file ($target)")
        fi
        rm -f "$outfile" "$outfile".*
    done
done

# Always clean up generated output files
rm -f Kinetrix_Output.*

echo ""
echo "════════════════════════════════════════"
if [ ${#failed_tests[@]} -gt 0 ]; then
    echo -e "${RED}FAILED: $passed_tests / $total_tests tests passed.${NC}"
    echo ""
    echo "Failed tests:"
    for ft in "${failed_tests[@]}"; do
        echo -e "  ${RED}✗${NC} $ft"
    done
    echo "════════════════════════════════════════"
    exit 1
else
    echo -e "${GREEN}All systems go! $passed_tests / $total_tests tests passed.${NC}"
    echo "════════════════════════════════════════"
    exit 0
fi
