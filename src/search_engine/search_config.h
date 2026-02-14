// ****************************************************************************
// *                      SEARCH CONFIGURATION STRUCT                         *
// ****************************************************************************
// Runtime configuration for search parameters that were previously compile-time
// constants. Allows dynamic allocation of search arrays based on user-specified
// limits and available memory.

#pragma once

#include <cstddef>
#include <cstdint>
#include "../chess_engine/types.h"

// =============================================================================
// RUNTIME SEARCH CONFIGURATION
// =============================================================================

struct SearchConfig {
  // Default values (match original compile-time constants)
  static constexpr size_t DEFAULT_MAX_PLYS_PER_GAME = 1000;
  static constexpr size_t DEFAULT_MAX_QUIESCE_DEPTH = 500;
  static constexpr size_t DEFAULT_HASH_SIZE_MB = 512;  // ~16M entries at default size
  
  // Maximum allowed values for validation
  static constexpr size_t MAX_PLYS_PER_GAME_LIMIT = 10000;
  static constexpr size_t MAX_QUIESCE_DEPTH_LIMIT = 10000;
  static constexpr size_t MAX_HASH_SIZE_MB = 1024 * 1024;  // 1TB
  
  // Game history limits (fixed at defaults for now)
  size_t maxPlysPerGame = DEFAULT_MAX_PLYS_PER_GAME;
  
  // Quiescence depth limit (used for array sizing)
  size_t maxQuiesceDepth = DEFAULT_MAX_QUIESCE_DEPTH;
  
  // Hash table configuration (configurable via CLI)
  // User specifies size in MB, we calculate actual slots
  size_t hashSizeMB = DEFAULT_HASH_SIZE_MB;
  
  // Computed: Actual number of hash slots (power of 2)
  // This is computed from hashSizeMB and HashRecord size
  size_t numHashSlots = 0;
  
  // Computed: Power of 2 used for the hash table
  // Table size = 2^hashPow2
  size_t hashPow2 = 0;
  
  // =============================================================================
  // SEARCH ALGORITHM FLAGS (formerly compile-time defines)
  // =============================================================================
  
  // Enable search diagnostics - adds bounds checking for debugging
  bool enableSearchDiagnostics = false;
  
  // Note: Null move heuristic and hash table in quiescence are always enabled
  // as they provide essential search optimizations with no practical reason
  // to disable them in normal operation.
  
  // =============================================================================
  // INITIALIZATION
  // =============================================================================
  
  // Default constructor - computes derived values
  SearchConfig() {
    computeHashSize();
  }
  
  // Constructor with explicit MB value
  explicit SearchConfig(size_t hashMB) : hashSizeMB(hashMB) {
    computeHashSize();
  }
  
  // =============================================================================
  // HASH SIZE CALCULATION
  // =============================================================================
  // Calculates the hash table power of 2 from MB input.
  // Finds the largest power of 2 that fits within the specified MB limit.
  
  void computeHashSize() {
    // Get actual size of hash record at runtime
    const size_t recordSize = sizeof(HashRecord);
    
    // Calculate target number of records from MB
    // hashSizeMB * 1024 * 1024 bytes available
    // Divide by record size to get number of slots
    const size_t targetSlots = (hashSizeMB * 1024 * 1024) / recordSize;
    
    // Find the largest power of 2 that doesn't exceed targetSlots
    // We want: 2^hashPow2 <= targetSlots
    hashPow2 = 0;
    size_t slots = 1;
    
    while (slots <= targetSlots / 2) {
      slots *= 2;
      hashPow2++;
    }
    
    // Ensure minimum reasonable size (2^16 = 64K slots)
    if (hashPow2 < 16) {
      hashPow2 = 16;
      slots = 1 << 16;
    }
    
    // Ensure we don't overflow size_t
    if (hashPow2 >= sizeof(size_t) * 8 - 1) {
      hashPow2 = sizeof(size_t) * 8 - 2;
      slots = 1ULL << hashPow2;
    }
    
    numHashSlots = slots;
  }
  
  // =============================================================================
  // VALIDATION
  // =============================================================================
  
  [[nodiscard]] bool validate() const {
    // All size values must be reasonable
    if (maxPlysPerGame == 0 || maxPlysPerGame > MAX_PLYS_PER_GAME_LIMIT) return false;
    if (maxQuiesceDepth == 0 || maxQuiesceDepth > MAX_QUIESCE_DEPTH_LIMIT) return false;
    if (hashSizeMB == 0 || hashSizeMB > MAX_HASH_SIZE_MB) return false;
    if (numHashSlots == 0) return false;
    if (hashPow2 < 16 || hashPow2 >= sizeof(size_t) * 8) return false;
    // Boolean flags are always valid
    return true;
  }
  
  // =============================================================================
  // UTILITY
  // =============================================================================
  
  // Calculate actual memory usage of hash table
  [[nodiscard]] size_t getHashMemoryBytes() const {
    return numHashSlots * sizeof(HashRecord);
  }
  
  [[nodiscard]] size_t getHashMemoryMB() const {
    return getHashMemoryBytes() / (1024 * 1024);
  }
};

// =============================================================================
// GLOBAL CONFIGURATION INSTANCE
// =============================================================================
// This is the global configuration used throughout the engine.
// Programs should set this before initializing the engine.

extern SearchConfig g_searchConfig;

// =============================================================================
// HASH TABLE INDEX CALCULATION
// =============================================================================
// Maps a 64-bit hash key to a table index using XOR folding.
// Uses the configured power of 2 size.

[[nodiscard]] inline size_t foldHashKey(HashKey key, size_t pow2) {
  size_t index = 0;
  
  // XOR fold high bits down
  for (int shift = 64 - static_cast<int>(pow2); shift >= 0; shift -= static_cast<int>(pow2)) {
    index ^= static_cast<size_t>(key >> shift);
  }
  
  // Mask to get final index within table size
  return index & ((1ULL << pow2) - 1);
}

// Convenience function using global config
[[nodiscard]] inline size_t foldHashKey(HashKey key) {
  return foldHashKey(key, g_searchConfig.hashPow2);
}

