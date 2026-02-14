// ****************************************************************************
// *                        SEARCH ENGINE HEADER FILE                         *
// ****************************************************************************
// Modernized for C++20 - Phase 1: Constants and Inline Functions

#pragma once

#include <cstdint>
#include <vector>
#include <array>

#include "../chess_engine/types.h"
#include "../chess_engine/chess_engine.h"
#include "../core/timing.h"
#include "evaluation.h"
#include "search_config.h"

// How many hash slots to use (must make a power of 2).
// 2003: Allowed to set this lower now, so that we can do quick test games.
#ifndef HASH_POW_2
constexpr int HASH_POW_2 = 24;                          // 2^N (16+ good?).
#endif

constexpr int NUM_HASH_SLOTS = (1 << HASH_POW_2);       // The number of slots.

// This is the window to add/subtract from the last ply's Max positional
// score to save on calls to Eval().
constexpr double EVAL_WINDOW = 0.99;                    // 0.5 to 0.99 seems ok at the mo?

// This is the root aspiration window.
constexpr double ASPIRATION_WINDOW = 0.5;

// For calling PlayGame() function with.
constexpr int TWO_HUMANS = 0;                           // Two Human Players.
constexpr int COMPUTER_WHITE = 1;                       // Computer Plays White.
constexpr int COMPUTER_BLACK = 2;                       // Computer Plays Black.
constexpr int TWO_COMPUTERS = 3;                        // Computers Play each other.

// For setting with a time limit for PlayGame() and Think().
constexpr uint32_t INFINITE_DEPTH = 0xffffffff;     // Use Time Limit only.
constexpr uint32_t INFINITE_TIME = 0xffffffff;      // Use Depth limit only.

// Quiescence depth limit (used for array sizing, not a hard search limit)
constexpr int MAX_QUIESCE_DEPTH = 500;

// These are used as the Max values, so the constants are not in the program.
constexpr int WIN_SCORE = 10000000;                     // Score returned for a Win (- for loss)
constexpr int DRAW_CONTEMPT = 0;                        // How much down to take a draw (-ve!).
constexpr int KILLER_SORT_SCORE = 10000000;             // Sort value to use for killer move.
constexpr int CAPTURE_SORT_SCORE = 100000000;           // Sort value to add for a capture.
constexpr int PROMOTION_SORT_SCORE = 100000000;         // Sort value to add for a promotion.
constexpr int PV_SORT_SCORE = 1000000000;               // Sort value to add if PV in Move-List.

// These are for the hash table.
constexpr int LOWERBOUND = 1;                           // Lower bound score.
constexpr int UPPERBOUND = 2;                           // Upper bound score.
constexpr int EXACTSCORE = 4;                           // Exact score.
constexpr int QUIESCENT = 8;                            // Is the position quiescent?

// These are the basic material values: ONLY USED BY BasicMaterialEval().
constexpr int PIECE_VALUE[6] = {10000, 30000, 30000, 50000, 90000, 0};

// =============================================================================

// SEARCH DATA TYPES:

// Note: HashRecord is now defined in types.h

// This is used to keep ruinning material scores (Pawn/Pieces) for each side.
struct RunningMaterial {
  std::vector<std::array<int, 2>> pieceMatValue;
  std::vector<std::array<int, 2>> pawnMatValue;
}; // End RunningMaterial.

// This hold all that is needed during a search.
struct SearchData : RunningMaterial {

  // One for Black and one for White.
  // ES is set by think so that the correct set is used for a whole search.
  EvaluationParameters evalParams;          // Loaded from file in main.

  // --------------------------------------------------------------------------

  // These are the extra stats collected while searching.
  int totalNodesSearched;
  int numAlphaCutOffs;
  int numBetaCutOffs;
  int numMaterialEvals;
  int numTrueEvals;
  int numNullCutOffs;
  int totalMoveGens;
  int numHashCollisions;
  int totalPutHashCount;
  int totalGetHashCount;
  int numHashSuccesses;
  int numCheckExtensions;
  int numMateExtensions;

  // The Iterative Deepening depth we are on.
  int iterDepth;

  // This is the starting time of the search (for thinking info!).
  ClockTime startTime;

  // Store both wall clock and CPU start times for reporting.
  double wallClockStart;
  double cpuStart;

  // Used for exiting searches when time is up.
  ClockTime stopTime;   // For storing the stopping time in CPU secs.


  // This is the maximum positional score we have seen for each ply.
  // These are then used with the window to see if we can use an estimate rather
  // than call Eval().
  std::vector<int> minPositionEval;
  std::vector<int> maxPositionEval;

  // This is the maximum positional difference seen between any two plys.
  int maxPositionalDiff;

  // This is for the aspiration search.
  int rootAlpha,rootBeta;

  // --------------------------------------------------------------------------

  // This is the transpostion (hash) table.
  std::vector<HashRecord> hashTable;

  // This is the hash move to be done.
  std::vector<MoveStruct> hashMoves; // -1,-1,-1,-1 if empty.

  // Temporary history used to store sort (guessed) value.
  // Made higher for cutoffs.
  // Captures/Promotions/Checks/etc override this and get searched first.
  std::array<std::array<int, 64>, 64> moveHistory;

  // The killer move is tried after all captures have been tried, but before
  // the history move value is used (eg medium score).
  std::vector<MoveStruct> killerMovesOld; // This is first best moves.
  std::vector<MoveStruct> killerMovesNew; // This is last best moves.

  // --------------------------------------------------------------------------

  // The move the computer has chosen.
  MoveStruct computersMove;
  int        computersMoveScore; 

  // Constructor to initialize vectors based on configuration
  explicit SearchData(const SearchConfig& config = SearchConfig()) {
    reset(config);
  }

  // Reset/reinitialize all data structures
  void reset(const SearchConfig& config = SearchConfig()) {
    // Initialize RunningMaterial vectors
    pieceMatValue.assign(config.maxQuiesceDepth, std::array<int, 2>{0, 0});
    pawnMatValue.assign(config.maxQuiesceDepth, std::array<int, 2>{0, 0});

    // Initialize search vectors
    minPositionEval.assign(config.maxQuiesceDepth, 0);
    maxPositionEval.assign(config.maxQuiesceDepth, 0);
    hashTable.assign(config.numHashSlots, HashRecord{});
    hashMoves.assign(config.maxQuiesceDepth, MoveStruct{-1, -1, 0, 0});
    killerMovesOld.assign(config.maxQuiesceDepth, MoveStruct{-1, -1, 0, 0});
    killerMovesNew.assign(config.maxQuiesceDepth, MoveStruct{-1, -1, 0, 0});

    // Clear MoveHistory
    for (auto& row : moveHistory) {
      row.fill(0);
    }

    // Reset stats
    totalNodesSearched = 0;
    numAlphaCutOffs = 0;
    numBetaCutOffs = 0;
    numMaterialEvals = 0;
    numTrueEvals = 0;
    numNullCutOffs = 0;
    totalMoveGens = 0;
    numHashCollisions = 0;
    totalPutHashCount = 0;
    totalGetHashCount = 0;
    numHashSuccesses = 0;
    numCheckExtensions = 0;
    numMateExtensions = 0;
    iterDepth = 0;
    startTime = 0;
    wallClockStart = 0.0;
    cpuStart = 0.0;
    stopTime = 0;
    maxPositionalDiff = 0;
    rootAlpha = 0;
    rootBeta = 0;
    computersMove = MoveStruct{-1, -1, 0, 0};
    computersMoveScore = 0;
  }

}; // End SearchData structure.

// ============================================================================
// MODERN INLINE FUNCTIONS (replacing macros)
// ============================================================================
// These are defined after SearchData so they can reference it

// To see if the score is a mating score.
[[nodiscard]] inline constexpr bool isMateScore(int score) noexcept {
  return (score >= (WIN_SCORE - 100)) || (score <= ((-WIN_SCORE) + 100));
}

// Find out how many plies until mate (MUST CHECK FOR MATE FIRST!).
[[nodiscard]] inline constexpr int getMateIn(int score) noexcept {
  return score > 0 ? (WIN_SCORE - score) : ((-WIN_SCORE) - score);
}

// Score returned for Draw.
[[nodiscard]] inline constexpr int getDrawScore() noexcept {
  return 0;
}

// Returns true if the search should time out.
[[nodiscard]] inline bool shouldTimeOut(const SearchData& sd) {
  return (sd.iterDepth > 2 && getTime() >= sd.stopTime);
}

// Get material value for a side - overloaded for RunningMaterial.
[[nodiscard]] inline int getMaterial(const RunningMaterial& rm, int side, int currentPly) noexcept {
  return rm.pieceMatValue[currentPly][side] + rm.pawnMatValue[currentPly][side];
}

[[nodiscard]] inline int getPawnMaterial(const RunningMaterial& rm, int side, int currentPly) noexcept {
  return rm.pawnMatValue[currentPly][side];
}

[[nodiscard]] inline int getPieceMaterial(const RunningMaterial& rm, int side, int currentPly) noexcept {
  return rm.pieceMatValue[currentPly][side];
}

// Get material sum for both sides.
[[nodiscard]] inline int getMaterialSum(const RunningMaterial& rm, int currentPly) noexcept {
  return getMaterial(rm, WHITE, currentPly) + getMaterial(rm, BLACK, currentPly);
}

[[nodiscard]] inline int getPawnMaterialSum(const RunningMaterial& rm, int currentPly) noexcept {
  return getPawnMaterial(rm, WHITE, currentPly) + getPawnMaterial(rm, BLACK, currentPly);
}

[[nodiscard]] inline int getPieceMaterialSum(const RunningMaterial& rm, int currentPly) noexcept {
  return getPieceMaterial(rm, WHITE, currentPly) + getPieceMaterial(rm, BLACK, currentPly);
}

// Get material evaluation from current side's perspective.
[[nodiscard]] inline int getMaterialEval(const RunningMaterial& rm, int currentSide, int otherSide, int currentPly) noexcept {
  return getMaterial(rm, currentSide, currentPly) - getMaterial(rm, otherSide, currentPly);
}

[[nodiscard]] inline int getPawnMaterialEval(const RunningMaterial& rm, int currentSide, int otherSide, int currentPly) noexcept {
  return getPawnMaterial(rm, currentSide, currentPly) - getPawnMaterial(rm, otherSide, currentPly);
}

[[nodiscard]] inline int getPieceMaterialEval(const RunningMaterial& rm, int currentSide, int otherSide, int currentPly) noexcept {
  return getPieceMaterial(rm, currentSide, currentPly) - getPieceMaterial(rm, otherSide, currentPly);
}

// =============================================================================

// PROTOTYPES:

// Think function.
MoveStruct think(int searchDepth,double maxTimeSeconds,bool showOutput,
                 bool showThinking,double randomSwing,const EvaluationParameters &evalParams);

// This should be called after make move to keep the material eval consistent.
void updateMaterialEvaluation(RunningMaterial &searchData,int currentPly,
                              const MoveStruct &moveMade);

// Searching function.
int search(SearchData &searchData,int currentPly,int alpha,int beta,int depth,
           bool nullMove);

// Quiescent Searching function.
int quiesceSearch(SearchData &searchData,int currentPly,int alpha,int beta,
                  bool nullMove);

// Move ordering functions.
void scoreMoves(SearchData &searchData,int currentPly,MoveList &moves,
                int moveScores[MOVELIST_ARRAY_SIZE],bool nullMove);
void sortMoves(MoveList &moves,int moveScores[MOVELIST_ARRAY_SIZE],int source);

// Quick Search functions (MATERIAL ONLY + NO BOOK-KEEPING).
[[nodiscard]] int isQuiescent(void);               // Returns -1, for time-out.
[[nodiscard]] int getQuiescentScore(RunningMaterial &searchData);
int quickQuiesceSearch(RunningMaterial &searchData,int currentPly,int alpha,int beta);

// Transposition Table functions.
int foldHashKey(HashKey key,int numElementsPow2); // Fold key to index.
void ttPut(SearchData &searchData,int currentPly,int depth,int alpha,int beta,int score,
           MoveStruct move,HashKey nextKey);
uint8_t ttGet(SearchData &searchData,int currentPly,int depth,int &score,MoveStruct &move,
              HashKey &nextKey);

// =============================================================================


