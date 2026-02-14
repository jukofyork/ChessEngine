// ****************************************************************************
// *                              X-INTERFACE STUFF                           *
// ****************************************************************************

#pragma once

// =============================================================================
// STANDARD INCLUDES
// =============================================================================

#include <fstream>
#include <iostream>

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

struct SearchData;
struct MoveStruct;
class EvaluationParameters;

// =============================================================================
// OWN EXTRA FUNCTIONS:
// =============================================================================

// Function from parse_pgn.cpp
bool convertFromSAN(char* sanMove, MoveStruct& algMove);

// =============================================================================
// PROTOTYPES:
// =============================================================================

void printBoard(int sideUpBoard);
void printMove(const MoveStruct &move);
void writeMinimalHeader(int numMoves,int gameResult,std::ofstream &outFile);
void readMinimalHeader(int &numMoves,int &gameResult,std::ifstream &inFile);
void writeMinimalMove(const MoveStruct &move,bool isQuiescent,std::ofstream &outFile);
void readMinimalMove(MoveStruct &move,uint8_t &isQuiescent,std::ifstream &inFile);
void printLine(SearchData &sd,MoveStruct &line,int moveScore,char boundType);
int playGame(int modeOfPlay,int searchDepth,double maxTimeSeconds,bool useBell,
             bool showOutput,bool showThinking,double randomSwing,
             const EvaluationParameters &epw,const EvaluationParameters &epb);

// =============================================================================
// CONSTANTS:
// =============================================================================

// Returned as result from PlayGame().
constexpr int WHITE_MATES = 0;            // White has mated and won.
constexpr int BLACK_MATES = 1;            // Black has mated and won.
constexpr int STALEMATE = 2;              // Games drawn from stalemate.
constexpr int FIFTY_MOVE_RULE = 3;        // Games drawn from 50-move-rule.
constexpr int THREE_IDENTICLE_POS = 4;    // Three posisions same.
constexpr int NOT_ENOUGH_MATERIAL = 5;    // Can't win from here.
constexpr int WHITE_RETIRES = 6;          // White has retired.
constexpr int BLACK_RETIRES = 7;          // Black has retired.

// Square colours.
constexpr int LIGHT_COLOUR = 190;         // Light square colour (grey only!).
constexpr int DARK_COLOUR = 130;          // Dark square colour (grey only!).

// For drawing coords with.
constexpr int COORD_COLOR_R = 0;          // Coordinate colour (Red).
constexpr int COORD_COLOR_G = 0;          // Coordinate colour (Green).
constexpr int COORD_COLOR_B = 0;          // Coordinate colour (Blue).
constexpr int COORD_INDENT = 2;           // Number of pixels to indent by.

// =============================================================================
// BINARY FILE FORMAT CONSTANTS
// =============================================================================
// These define the bit layout for the minimal binary file format.
// All values avoid 0 bytes by using offset bits.
//
// DESIGN RATIONALE:
// The binary format is designed to be compact while avoiding null bytes ('\0')
// which could cause issues with C-string handling. This is achieved by:
// 1. Adding padding bits set to 1 at strategic positions
// 2. Offsetting result values by +2 to ensure they're never 0
// 3. Storing (promote-1) so promotion value 0 becomes stored value -1 (handled specially)
//
// GAME HEADER FORMAT (2 bytes = 16 bits):
// ======================================
// Bit layout: [15] [14] [13..1] [0]
//             |    |    |       |
//             |    |    |       +-- Always 1 (padding to avoid null byte)
//             |    |    +-- 13 bits: Number of moves in game (max 8191)
//             |    +-- Result + 2 (0-3 encoded as 2-5 to avoid 0)
//             +-- Result + 2 (continued)
//
// Example: Game with 100 moves, white wins (result=0)
//   Encoded: bit0=1, bits1-13=100, bits14-15=0+2=2
//   Value: 1 | (100 << 1) | (2 << 14) = 0x8001 + 200 = 0x80C9

constexpr int HEADER_MOVE_BITS = 13;           // Number of bits for move count
constexpr int HEADER_RESULT_BITS = 2;          // Number of bits for result
constexpr int HEADER_MOVE_SHIFT = 1;           // Shift to align move count (after padding bit)
constexpr int HEADER_RESULT_SHIFT = 14;        // Shift to align result (last 2 bits)
constexpr int HEADER_MOVE_MASK = (1 << HEADER_MOVE_BITS) - 1;  // Mask: 0x1FFF (8191)
constexpr int HEADER_RESULT_OFFSET = 2;        // Offset added to result to avoid 0

// MOVE RECORD FORMAT (3 bytes = 24 bits):
// =======================================
// Bit layout (visualized as 24-bit value):
// [23] [22] [21..20] [19..14] [13] [12..7] [6] [5..0]
//  |    |     |        |       |     |      |    |
//  |    |     |        |       |     |      |    +-- Source square (0-63, 6 bits)
//  |    |     |        |       |     |      +-- Always 1 (padding)
//  |    |     |        |       |     +-- Target square (0-63, 6 bits)
//  |    |     |        |       +-- Always 1 (padding)
//  |    |     |        +-- Move type flags (6 bits)
//  |    |     +-- Promotion piece-1 (0-3, 2 bits: 0=None, 1=N, 2=B, 3=R, 4=Q stored as 0-3)
//  |    +-- Quiescent flag (1 bit)
//  +-- Always 1 (padding)
//
// Total bits used: 6+1+6+1+6+2+1+1 = 24 bits (exactly 3 bytes)
//
// Example: Move e2-e4 (source=52, target=36), normal move, not quiescent
//   Encoded: bit0-5=52, bit6=1, bits7-12=36, bit13=1, bits14-19=0, bits20-21=0, bit22=0, bit23=1
//   Note: Promotion is stored as (piece-1), so 0=None, 1=N, 2=B, 3=R, 4=Q -> stored as 0,0,1,2,3

constexpr int MOVE_SOURCE_BITS = 6;            // Bits for source square (0-63)
constexpr int MOVE_TARGET_BITS = 6;            // Bits for target square (0-63)
constexpr int MOVE_TYPE_BITS = 6;              // Bits for move type flags
constexpr int MOVE_PROMOTE_BITS = 2;           // Bits for promotion piece-1
constexpr int MOVE_QUISCENT_BITS = 1;          // Bits for quiescent flag

constexpr int MOVE_SOURCE_SHIFT = 0;           // Source at bits 0-5
constexpr int MOVE_TARGET_SHIFT = 7;           // Target at bits 7-12 (after padding at 6)
constexpr int MOVE_TYPE_SHIFT = 14;            // Type at bits 14-19 (after padding at 13)
constexpr int MOVE_PROMOTE_SHIFT = 20;         // Promote at bits 20-21
constexpr int MOVE_QUISCENT_SHIFT = 22;        // Quiescent at bit 22

// Masks for extracting field values after shifting
constexpr int MOVE_SOURCE_MASK = (1 << MOVE_SOURCE_BITS) - 1;   // 0x3F (63)
constexpr int MOVE_TARGET_MASK = (1 << MOVE_TARGET_BITS) - 1;   // 0x3F (63)
constexpr int MOVE_TYPE_MASK = (1 << MOVE_TYPE_BITS) - 1;       // 0x3F (63)
constexpr int MOVE_PROMOTE_MASK = (1 << MOVE_PROMOTE_BITS) - 1; // 0x03 (3)

// Padding bits set to 1 to avoid null bytes in output
// These are strategically placed at byte boundaries:
// - Bit 6: Ensures low byte is never 0
// - Bit 13: Ensures middle byte is never 0  
// - Bit 23: Ensures high byte is never 0
constexpr int MOVE_PAD_BIT_6 = 6;              // Padding at bit 6 (between source and target)
constexpr int MOVE_PAD_BIT_13 = 13;            // Padding at bit 13 (between target and type)
constexpr int MOVE_PAD_BIT_23 = 23;            // Padding at bit 23 (highest bit)

// =============================================================================
