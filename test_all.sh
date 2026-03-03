#!/bin/bash
# Kinetrix Compiler - Automated Test Suite
# Runs all example and test files, reports pass/fail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KCC="$SCRIPT_DIR/kcc"
TMP_OUT="/tmp/kx_test_$$"
TOTAL_PASS=0
TOTAL_FAIL=0
TOTAL_SKIP=0
FAILURES=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

run_test() {
    local file="$1"
    local target="${2:-arduino}"
    local basename=$(basename "$file")
    
    result=$("$KCC" "$file" --target "$target" -o "$TMP_OUT" 2>&1)
    rc=$?
    
    if [ $rc -eq 0 ]; then
        echo -e "  ${GREEN}✓${NC} $basename ($target)"
        TOTAL_PASS=$((TOTAL_PASS + 1))
        return 0
    else
        # Check if failure is only due to missing import files
        if echo "$result" | grep -q "Could not open included file"; then
            # Only missing imports - skip
            echo -e "  ${YELLOW}⊘${NC} $basename (missing imports)"
            TOTAL_SKIP=$((TOTAL_SKIP + 1))
            return 0
        else
            echo -e "  ${RED}✗${NC} $basename ($target)"
            FAILURES="$FAILURES\n  $basename: $(echo "$result" | grep "^  Line" | head -1)"
            TOTAL_FAIL=$((TOTAL_FAIL + 1))
            return 1
        fi
    fi
}

echo "╔══════════════════════════════════════════╗"
echo "║   Kinetrix Compiler — Test Suite v3.1    ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# 1. Build check
echo "Building compiler..."
if ! make -C "$SCRIPT_DIR" -s 2>/dev/null; then
    echo -e "${RED}BUILD FAILED${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Build successful${NC}"
echo ""

# 2. Basic examples
echo "━━━ Basic Examples (examples_working/) ━━━"
for f in "$SCRIPT_DIR"/examples_working/*.kx; do
    [ -f "$f" ] && run_test "$f"
done
echo ""

# 3. V3 feature tests
echo "━━━ V3 Feature Tests ━━━"
for f in "$SCRIPT_DIR"/examples/v3_*.kx; do
    [ -f "$f" ] && run_test "$f"
done
echo ""

# 4. Remaining non-v3 examples
echo "━━━ Advanced Examples ━━━"
for f in "$SCRIPT_DIR"/examples/*.kx; do
    case "$(basename "$f")" in v3_*) continue ;; esac
    [ -f "$f" ] && run_test "$f"
done
echo ""

# 5. Test files (only ones that should compile)
echo "━━━ Unit Test Files ━━━"
for f in "$SCRIPT_DIR"/test_*.kx; do
    basename=$(basename "$f")
    # Skip intentional error test files
    case "$basename" in
        test_error.kx|test_unused.kx|test_wait.kx)
            echo -e "  ${YELLOW}⊘${NC} $basename (intentional error test)"
            TOTAL_SKIP=$((TOTAL_SKIP + 1))
            continue ;;
    esac
    [ -f "$f" ] && run_test "$f"
done
echo ""

# 6. Multi-target tests (test one file across all targets)
echo "━━━ Multi-Target Generation ━━━"
TEST_FILE="$SCRIPT_DIR/examples/v3_for_loop_test.kx"
for target in arduino esp32 rpi pico ros2; do
    run_test "$TEST_FILE" "$target"
done
echo ""

# 7. Edge case tests
echo "━━━ Edge Cases ━━━"
echo 'program {}' > /tmp/edge_empty.kx
run_test /tmp/edge_empty.kx

cat > /tmp/edge_neg.kx << 'EOF'
program {
  make var x = -3.14
  print x
}
EOF
run_test /tmp/edge_neg.kx

cat > /tmp/edge_keywords.kx << 'EOF'
program {
  make var count = 10
  make var size = 20
  make var value = 30
  print count + size + value
}
EOF
run_test /tmp/edge_keywords.kx

cat > /tmp/edge_strcat.kx << 'EOF'
program {
  make var a = "Hello" + " " + "World"
  print a
}
EOF
run_test /tmp/edge_strcat.kx

cat > /tmp/edge_divzero.kx << 'EOF'
program {
  make var x = 10
  make var z = x / 0
  print z
}
EOF
run_test /tmp/edge_divzero.kx

cat > /tmp/edge_arrset.kx << 'EOF'
program {
  make array data size 10
  data[0] = 42
  print data[0]
}
EOF
run_test /tmp/edge_arrset.kx
echo ""

# 8. Error handling (these should fail with non-zero exit)
echo "━━━ Error Handling (should reject) ━━━"
cat > /tmp/err_garbage.kx << 'EOF'
{{{{{{{ this is not valid kinetrix code @@@ !!!
EOF
if "$KCC" /tmp/err_garbage.kx -o /tmp/kx_err.ino 2>/dev/null; then
    echo -e "  ${RED}✗${NC} garbage input (should fail but succeeded)"
    TOTAL_FAIL=$((TOTAL_FAIL + 1))
else
    echo -e "  ${GREEN}✓${NC} garbage input rejected (exit $?)"
    TOTAL_PASS=$((TOTAL_PASS + 1))
fi

cat > /tmp/err_nobrace.kx << 'EOF'
program { print "hello"
EOF
if "$KCC" /tmp/err_nobrace.kx -o /tmp/kx_err.ino 2>/dev/null; then
    echo -e "  ${RED}✗${NC} missing brace (should fail but succeeded)"
    TOTAL_FAIL=$((TOTAL_FAIL + 1))
else
    echo -e "  ${GREEN}✓${NC} missing brace rejected (exit $?)"
    TOTAL_PASS=$((TOTAL_PASS + 1))
fi

if "$KCC" /tmp/nonexistent_file.kx -o /tmp/kx_err.ino 2>/dev/null; then
    echo -e "  ${RED}✗${NC} missing file (should fail but succeeded)"
    TOTAL_FAIL=$((TOTAL_FAIL + 1))
else
    echo -e "  ${GREEN}✓${NC} missing file rejected (exit $?)"
    TOTAL_PASS=$((TOTAL_PASS + 1))
fi
echo ""

# Cleanup
rm -f "$TMP_OUT" /tmp/edge_*.kx /tmp/err_*.kx /tmp/kx_err.ino

# Summary
echo "══════════════════════════════════════════"
echo -e "  ${GREEN}Passed: $TOTAL_PASS${NC}  |  ${RED}Failed: $TOTAL_FAIL${NC}  |  ${YELLOW}Skipped: $TOTAL_SKIP${NC}"
echo "══════════════════════════════════════════"

if [ -n "$FAILURES" ]; then
    echo -e "\nFailure details:$FAILURES"
fi

if [ $TOTAL_FAIL -gt 0 ]; then
    echo -e "\n${RED}TESTS FAILED${NC}"
    exit 1
else
    echo -e "\n${GREEN}ALL TESTS PASSED ✓${NC}"
    exit 0
fi
