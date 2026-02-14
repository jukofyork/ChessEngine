// ****************************************************************************
// *                         CHESS ENGINE HEADER FILE                         *
// ****************************************************************************
// Modernized for C++20

#pragma once

// =============================================================================
// STANDARD C++ HEADERS (replacing old C headers)
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>

// Shared types and constants
#include "types.h"
#include "constants.h"

// =============================================================================
// CORE LIBRARIES
// =============================================================================

#include "../core/error_handling.h"

// =============================================================================
// CONSTANTS (using constexpr instead of #define)
// =============================================================================

// Version string
constexpr char VERSION[] = "2003_new_start: [V8 = Slim-lined Eval - C++20 Modernized]";

// Player Colors
constexpr int WHITE = 0;
constexpr int BLACK = 1;

// Piece Types
constexpr int PAWN = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK = 3;
constexpr int QUEEN = 4;
constexpr int KING = 5;

// Move initialization constants
constexpr int NO_PROMOTION = 0;  // For MoveStruct.Promote when not a promotion

// Empty square indicator
constexpr int NONE = -1;

// Move type flags
constexpr int NORMAL_MOVE = 0;
constexpr int CAPTURE = 1;
constexpr int CASTLE = 2;
constexpr int EN_PASSANT = 4;
constexpr int TWO_SQUARES = 8;
constexpr int PAWN_MOVE = 16;
constexpr int PROMOTION = 32;

// Castling permissions
constexpr int WHITE_KING_SIDE = 1;
constexpr int WHITE_QUEEN_SIDE = 2;
constexpr int BLACK_KING_SIDE = 4;
constexpr int BLACK_QUEEN_SIDE = 8;
constexpr int NO_EN_PASSANT = -1;

// Lookup table sentinel
constexpr int END_OF_LOOKUP = -1;

// =============================================================================
// SQUARE CONSTANTS (A8 = 0, H1 = 63)
// =============================================================================

enum Square : int {
    A8 = 0,  B8,  C8,  D8,  E8,  F8,  G8,  H8,
    A7 = 8,  B7,  C7,  D7,  E7,  F7,  G7,  H7,
    A6 = 16, B6,  C6,  D6,  E6,  F6,  G6,  H6,
    A5 = 24, B5,  C5,  D5,  E5,  F5,  G5,  H5,
    A4 = 32, B4,  C4,  D4,  E4,  F4,  G4,  H4,
    A3 = 40, B3,  C3,  D3,  E3,  F3,  G3,  H3,
    A2 = 48, B2,  C2,  D2,  E2,  F2,  G2,  H2,
    A1 = 56, B1,  C1,  D1,  E1,  F1,  G1,  H1
};

// =============================================================================
// HELPER FUNCTIONS (replacing macros)
// =============================================================================

[[nodiscard]] inline constexpr int getOtherSide(int currentSide) noexcept {
    return 1 - currentSide;
}

[[nodiscard]] inline constexpr int getFile(int square) noexcept {
    return square & 7;
}

[[nodiscard]] inline constexpr int getRank(int square) noexcept {
    return square >> 3;
}

[[nodiscard]] inline constexpr int getSquare(char file, char rank) {
    return (static_cast<int>('8' - rank) * 8) + static_cast<int>(file - 'a');
}

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

// Game history functions
void initAll();
bool makeMove(MoveStruct& moveToMake);
void takeMoveBack();

// Attack testing functions
[[nodiscard]] bool isAttacked(int square, int sideAttacking);
bool singleAttack(int targetSquare, int newEnemySquare);
bool testExposure(int targetSquare, int evacuatedSquare, int sideAttacking);

// Draw testing functions
[[nodiscard]] bool testRepetition();
bool testSingleRepetition(int minMoveNum);
[[nodiscard]] bool testNotEnoughMaterial();

// Move generation functions
void genLegalMoves(MoveList& moves);
void genMoves(MoveList& moves);
void genCaptures(MoveList& moves);
void genPush(MoveList& Moves, int source, int target, int type);

// Lookup table generation functions
void generateMoveTables();
void generateExposedAttackTable();
void initPosData();

// Hash key functions
void initHashCodes();
[[nodiscard]] HashKey currentKey();

