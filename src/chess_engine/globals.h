// *****************************************************************************
// *                             GLOBAL VARIABLES                              *
// *****************************************************************************
// Modernized for C++20 - Dynamic allocation for game history arrays
// Arrays that were previously fixed-size C arrays are now dynamically allocated
// based on SearchConfig parameters for safer memory usage.

#pragma once

#include <cstdint>
#include <memory>
#include "types.h"
#include "constants.h"

// Forward declaration to avoid circular include
struct SearchConfig;

// =============================================================================
// CURRENT GAME/MOVE HISTORY (DYNAMICALLY ALLOCATED)
// =============================================================================
// These arrays are now allocated dynamically based on SearchConfig::maxPlysPerGame.
// They are accessed via unique_ptr and raw pointers for compatibility.

extern std::unique_ptr<GameState[]> g_gameHistory;
extern int g_moveNum;
extern int g_currentSide;
extern GameState* g_currentState;
extern int8_t* g_currentColour;
extern int8_t* g_currentPiece;

extern std::unique_ptr<MoveStruct[]> g_movesMade;

// =============================================================================
// INITIALIZATION FUNCTIONS
// =============================================================================
// These must be called before using the global arrays.

// Initialize global arrays with the specified configuration.
// Must be called before any game operations.
void initGlobals(const SearchConfig& config);

// Check if globals have been initialized.
[[nodiscard]] bool areGlobalsInitialized();

// =============================================================================
// LOOKUP TABLES
// =============================================================================

extern int g_knightMoves[64][9];
extern int g_diagonalMoves[64][4][8];
extern int g_straightMoves[64][4][8];
extern int g_kingMoves[64][9];
extern int g_exposedAttackTable[64][64];
extern bool g_knightAttackTable[64][64];
extern SquareData g_posData[6][64][64];

// =============================================================================
// HASH CODES
// =============================================================================

extern HashKey g_hashCode[2][6][64];
extern HashKey g_enPassantHashCode[64];
extern HashKey g_castleHashCode[16];
extern HashKey g_sideHashCode;

// =============================================================================
