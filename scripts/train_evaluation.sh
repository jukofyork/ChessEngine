#!/bin/bash
# train_evaluation.sh
# Trains the evaluation function
# Converted from TrainEval.bat

# Configuration
TRAINING_DATA="data/training/all_random.dat"
OUTPUT_SET="data/evaluation_sets/new2012.set"

echo "Training evaluation function..."
echo "Training data: $TRAINING_DATA"
echo "Output set: $OUTPUT_SET"
echo ""

# Check if training data exists
if [ ! -f "$TRAINING_DATA" ]; then
    echo "Error: Training data not found at $TRAINING_DATA"
    echo "Please ensure the training data file is in place."
    exit 1
fi

# Check if TrainEval exists
if [ ! -f "./TrainEval" ]; then
    echo "Error: TrainEval executable not found. Please build with 'make TrainEval'"
    exit 1
fi

# Run training
./TrainEval "$TRAINING_DATA" "$OUTPUT_SET"

echo ""
echo "Training complete!"
echo "New evaluation set saved to: $OUTPUT_SET"
