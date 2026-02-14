// *****************************************************************************
// *                             GLOBAL VARIABLES                              *
// *****************************************************************************
// Definitions for global variables declared in globals.h
// Arrays are dynamically allocated based on SearchConfig parameters.

#include "globals.h"
#include "../search_engine/search_config.h"
#include "../core/error_handling.h"

// =============================================================================
// CURRENT GAME/MOVE HISTORY (DYNAMICALLY ALLOCATED)
// =============================================================================

std::unique_ptr<GameState[]> g_gameHistory;
int g_moveNum;
int g_currentSide;
GameState* g_currentState;
int8_t* g_currentColour;
int8_t* g_currentPiece;

std::unique_ptr<MoveStruct[]> g_movesMade;

// =============================================================================
// INITIALIZATION
// =============================================================================

void initGlobals(const SearchConfig& config) {
  // Allocate game history arrays based on configuration
  try {
    g_gameHistory = std::make_unique<GameState[]>(config.maxPlysPerGame);
    g_movesMade = std::make_unique<MoveStruct[]>(config.maxPlysPerGame);
  } catch (const std::bad_alloc& e) {
    FATAL_ERROR("Failed to allocate global arrays: " + std::string(e.what()) + 
                " (requested " + std::to_string(config.maxPlysPerGame) + " plies)");
    return;  // Never reached, but keeps compiler happy
  }
  
  // Initialize pointers to first element for convenient access
  g_currentState = g_gameHistory.get();
  if (g_currentState != nullptr) {
    g_currentColour = g_currentState[0].colour;
    g_currentPiece = g_currentState[0].piece;
  }
  
  // Reset state
  g_moveNum = 0;
  g_currentSide = 0;  // WHITE
}

bool areGlobalsInitialized() {
  return g_gameHistory != nullptr && g_movesMade != nullptr;
}

// =============================================================================
// LOOKUP TABLES
// =============================================================================

int g_knightMoves[64][9];
int g_diagonalMoves[64][4][8];
int g_straightMoves[64][4][8];
int g_kingMoves[64][9];
int g_exposedAttackTable[64][64];
bool g_knightAttackTable[64][64];
SquareData g_posData[6][64][64];

// =============================================================================
// HASH CODES
// =============================================================================

HashKey g_hashCode[2][6][64];
HashKey g_enPassantHashCode[64];
HashKey g_castleHashCode[16];
HashKey g_sideHashCode;

// =============================================================================
