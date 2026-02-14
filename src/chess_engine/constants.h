// ****************************************************************************
// *                         CHESS ENGINE CONSTANTS                           *
// ****************************************************************************
// Shared constants used across the chess engine
// This header should be lightweight and have minimal dependencies

#pragma once

// =============================================================================
// FIXED ARRAY SIZE LIMITS
// =============================================================================
// NOTE: MAX_PLYS_PER_GAME is now configurable via SearchConfig (see search_config.h)
// These constants remain for specific fixed-size arrays that we keep as C-style
// for performance reasons (frequent stack allocation during search).

// MoveList buffer size - kept as fixed array for stack allocation performance.
// This is a hard upper limit; move generation will stop at this limit with a
// warning. In practice, chess positions rarely exceed 200 moves per position.
// We may revisit making this dynamic in the future, but for now the fixed
// array on the stack is faster than heap allocation during the hot search path.
constexpr int MOVELIST_ARRAY_SIZE = 2000;    // Buffer size for MoveList struct

// Board geometry constants
constexpr int BOARD_SIZE = 8;              // 8x8 board
constexpr int BOARD_SQUARES = 64;          // Total squares on board
constexpr int EPSILON = 0.000001;          // Floating point epsilon for comparisons
