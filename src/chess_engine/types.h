// ****************************************************************************
// *                         CHESS ENGINE TYPES                               *
// ****************************************************************************
// Core data structures used throughout the chess engine
// This header should have minimal dependencies

#pragma once

#include <array>
#include <cstdint>
#include "constants.h"

// =============================================================================
// TYPE ALIASES
// =============================================================================

// Hash keys for Zobrist hashing and transposition table entries
using HashKey = uint64_t;

// Clock time for search timing measurements
using ClockTime = uint64_t;

// =============================================================================
// STRUCTURES
// =============================================================================

// Basic move description
struct MoveStruct {
    int8_t source;         // Move's source square (0-63, -1 for invalid)
    int8_t target;         // Move's target square (0-63, -1 for invalid)
    uint8_t type;          // Move description flags (bitfield)
    uint8_t promote;       // Promotion piece type (0-4)
};

// Game state for move history/repetition detection
struct GameState {
    int8_t colour[BOARD_SQUARES];     // WHITE, BLACK, or NONE (-1)
    int8_t piece[BOARD_SQUARES];      // PAWN, KNIGHT, etc. or NONE (-1)
    uint8_t castlePerm;               // Castling permissions bitfield
    int8_t enPass;                    // En passant square or NO_EN_PASSANT (-1)
    int fiftyCounter;                 // 50-move rule counter
    std::array<int, 2> kingSquare;    // King positions for both sides
    unsigned inCheck : 1;             // Side to move is in check
    unsigned isDraw : 1;              // Position is drawn
    HashKey key;                      // Zobrist hash key
};

// Move list for move generation
// NOTE: Uses fixed-size array for performance (stack allocation during search).
// See comment in constants.h for rationale on keeping this fixed.
struct MoveList {
    MoveStruct moves[MOVELIST_ARRAY_SIZE];
    int numMoves;
};

// Square data for move generation
struct SquareData {
    int testSquare;
    SquareData* skip;
};

// Search data structure
struct SearchData;

// Evaluation parameters class
class EvaluationParameters;

// Hash record for transposition table
struct HashRecord {
    MoveStruct move;            // The best move we found here last time.
    int        score;           // The score of the state.
    uint8_t    flags;           // Flags describing the search at this state.
    uint8_t    depth;           // The depth we were at when we searched it.
    HashKey    key;             // Key to use (stored for quick comparison).
    HashKey    nextKey;         // Key we have after move (0 otherwise).
};
