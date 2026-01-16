#!/bin/bash
#==============================================================================
# Test: QBE type check for storel with w operand
#
# This test verifies that QBE accepts storel/loadl with 'w' type operands
# on the w65816 target, which is necessary because:
# - w65816 uses 'l' class for addresses (pointers)
# - w65816 uses 'w' class for 16-bit integers
# - Storing an int through a pointer requires storel with w operand
#==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QBE="$SCRIPT_DIR/../../bin/qbe"

# Test case: storel with w operand (this is what cproc generates)
TEST_QIL=$(cat <<'EOF'
function w $test() {
@start
    %.1 =l alloc4 4
    %.2 =w copy 42
    storel %.2, %.1
    %.3 =w loadl %.1
    ret %.3
}
EOF
)

echo "Testing: storel with 'w' operand on w65816 target"
echo "$TEST_QIL" | "$QBE" -t w65816 -o /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "[PASS] QBE accepts storel with 'w' operand for w65816"
    exit 0
else
    echo "[FAIL] QBE rejects storel with 'w' operand for w65816"
    echo "This needs to be fixed for the compiler to work."
    exit 1
fi
