// ****************************************************************************
// *                      EVALUATION CONFIGURATION STRUCT                     *
// ****************************************************************************
// Runtime configuration for evaluation parameters that were previously 
// compile-time defines. Allows runtime control over evaluation features.

#pragma once

#include <cstddef>
#include <cstdint>

// =============================================================================
// RUNTIME EVALUATION CONFIGURATION
// =============================================================================

struct EvaluationConfig {
  // King distance features - bonus/penalty based on distance to kings
  // NOTE: Currently disabled by default as it seems to hurt playing strength
  bool useKingDistanceFeatures = false;
  
  // Empty square features - piece-square values for empty squares
  // NOTE: Currently enabled and appears to help
  bool useEmptySquareFeatures = true;
  
  // Super fast eval - use only piece-square features (ignore all other features)
  // Useful for testing speed vs strength tradeoffs
  bool useSuperFastEval = false;
  
  // Linear training - use simple linear training instead of sigmoid activations
  // NOTE: Must match the training method used to create the evaluation set
  bool useLinearTraining = true;
  
  // =============================================================================
  // VALIDATION
  // =============================================================================
  
  [[nodiscard]] bool validate() const {
    // All boolean flags are valid (no ranges to check)
    return true;
  }
};

// =============================================================================
// GLOBAL CONFIGURATION INSTANCE
// =============================================================================
// This is the global configuration used throughout the engine.
// Programs should set this before initializing the engine.

extern EvaluationConfig g_evaluationConfig;

// =============================================================================

