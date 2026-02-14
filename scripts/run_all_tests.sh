#!/bin/bash
# run_all_tests.sh
# Runs all test positions and saves results
# Converted from ChessTest_all.bat

# Configuration
EVAL_SET="data/evaluation_sets/new2012.set"
TEST_DIR="data/test_positions"
RESULTS_DIR="data/test_positions"

# Check if ChessTest exists
if [ ! -f "./ChessTest" ]; then
    echo "Error: ChessTest executable not found. Please build with 'make ChessTest'"
    exit 1
fi

# Check if evaluation set exists
if [ ! -f "$EVAL_SET" ]; then
    echo "Warning: $EVAL_SET not found, using best_so_far.set"
    EVAL_SET="data/evaluation_sets/best_so_far.set"
fi

echo "Running all test positions..."
echo "Using evaluation set: $EVAL_SET"
echo ""

# List of test files
TESTS=(
    "bellin.fin"
    "larsen2.fin"
    "finecomb.fin"
    "sherev.fin"
    "larsen3.fin"
    "larsen1.fin"
    "openings.fin"
    "kotov.fin"
    "hortjan.fin"
    "kcmdm.fin"
    "positional.fin"
    "speelman.fin"
    "jenoban.fin"
    "winatchess.fin"
    "gelfer.fin"
    "hardmid.fin"
    "reinfeld.fin"
    "ece3.fin"
    "krabbe.fin"
    "dvor.fin"
)

# Run each test
for test in "${TESTS[@]}"; do
    if [ -f "$TEST_DIR/$test" ]; then
        echo "Testing: $test"
        result_file="$RESULTS_DIR/${test%.fin}_results.txt"
        ./ChessTest "$TEST_DIR/$test" "$EVAL_SET" | tee "$result_file"
        echo ""
        echo "Results saved to: $result_file"
        echo "---"
    else
        echo "Warning: $test not found, skipping..."
    fi
done

echo ""
echo "All tests complete!"
echo "Results saved in $RESULTS_DIR/"
