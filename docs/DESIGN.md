# Chess Engine Design Document

**Version:** Modernized C++20 Edition  
**Original Development:** 2001-2012  
**Modernization:** February 2026  
**Document Version:** 2.0

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Architecture Overview](#2-architecture-overview)
3. [Chess Engine Module - Board Representation](#3-chess-engine-module---board-representation)
4. [Chess Engine Module - Move Generation](#4-chess-engine-module---move-generation)
5. [Chess Engine Module - Game State Management](#5-chess-engine-module---game-state-management)
6. [Search Engine Module - Alpha-Beta Search](#6-search-engine-module---alpha-beta-search)
7. [Search Engine Module - Quiescence Search](#7-search-engine-module---quiescence-search)
8. [Search Engine Module - Heuristics](#8-search-engine-module---heuristics)
9. [Search Engine Module - Transposition Table](#9-search-engine-module---transposition-table)
10. [Evaluation System Architecture](#10-evaluation-system-architecture)
11. [Evaluation System Training Algorithm](#11-evaluation-system-training-algorithm)
12. [Interface Module - UCI and PGN](#12-interface-module---uci-and-pgn)
13. [Programs and Entry Points](#13-programs-and-entry-points)
14. [File Formats](#14-file-formats)
15. [Build System](#15-build-system)
16. [Appendices](#16-appendices)

---

## 1. Executive Summary

### 1.1 Project Overview

This is a full-featured chess engine implementing alpha-beta search with quiescence search, trainable evaluation function, and UCI protocol support. The engine was originally developed between 2001-2012 and modernized in February 2026 to use C++20 standards with modern memory management and type safety.

### 1.2 Key Capabilities

- **Search Performance:** ~2,000,000+ nodes/second (wall clock), ~500,000+ nodes/second (CPU time)
- **Search Depth:** Unlimited search depth (limited only by quiescence and time constraints)
- **Evaluation:** 2,796 trainable parameters across three game stages
- **Protocol:** Full UCI (Universal Chess Interface) support
- **Training:** Temporal difference learning from game databases

### 1.3 Development History

| Period | Version | Key Changes |
|--------|---------|-------------|
| 2001-2003 | Initial | Neural network evaluation |
| 2003_v8 | Staged evaluation | Opening/middlegame/endgame stages |
| 2008 | Linear training | TD learning approach |
| 2012 | Final updates | Before hiatus |
| 2026 | Modernization | C++20, Makefile build, dual timing, dynamic allocation |
| 2026 v2 | Architecture cleanup | Removed MAX_SEARCH_DEPTH, SearchConfig struct, dynamic arrays |
| 2026 v3 | Conditional compilation removal | Removed USE_* flags, runtime EvaluationConfig |

### 1.4 Directory Structure

```
src/
├── chess_engine/      # Board representation, move generation
│   ├── types.h        # Core data structures (MoveStruct, GameState)
│   ├── constants.h    # Fixed-size array limits and board constants
│   ├── globals.h      # Global variable declarations (now dynamically allocated)
├── search_engine/     # Alpha-beta search, evaluation
│   ├── search_config.h       # Runtime search configuration struct
│   ├── evaluation_config.h   # Runtime evaluation configuration struct
├── interface/         # UCI protocol, PGN parsing
├── core/              # Utilities (timing, error handling)
└── programs/          # Main entry points

data/
├── evaluation_sets/   # Trained weights (.set files)
├── test_positions/    # Test positions (.fin files)
└── training/          # Training databases (.dat files)

docs/                  # Documentation
scripts/               # Utility scripts
```

---

## 2. Architecture Overview

### 2.1 Module Structure

The engine is organized into four main modules:

#### 2.1.1 ChessEngine Module (`src/chess_engine/`)

**Purpose:** Board representation, legal move generation, game state management

**Key Files:**
- `types.h` - Core data structures (MoveStruct, GameState, MoveList, HashRecord)
- `constants.h` - Fixed-size array limits (MOVELIST_ARRAY_SIZE, BOARD_SQUARES)
- `chess_engine.h` - Function prototypes and move type constants
- `globals.h` - Global variable declarations (now using dynamic allocation)
- `globals.cpp` - Global variable definitions and initialization
- `move_generation.cpp` - Legal move generation
- `attack_tests.cpp` - Attack and check detection
- `game_history.cpp` - Move making and undo
- `lookup_tables.cpp` - Precomputed move tables
- `hash_key_codes.cpp` - Zobrist hashing

#### 2.1.2 SearchEngine Module (`src/search_engine/`)

**Purpose:** AI search algorithms, position evaluation

**Key Files:**
- `search_config.h` - Runtime search configuration (SearchConfig struct)
- `search_engine.h` - Search data structures and search-specific constants
- `think.cpp` - Iterative deepening controller
- `search.cpp` - Main alpha-beta search
- `quiescent_search.cpp` - Quiescence search
- `evaluation.cpp/.h` - Trainable evaluation function
- `material_evaluation.cpp` - Incremental material tracking
- `move_ordering.cpp` - Move ordering heuristics
- `transposition_table.cpp` - Hash table operations
- `quick_search.cpp` - Fast tactical search

#### 2.1.3 Interface Module (`src/interface/`)

**Purpose:** Game loop, UCI protocol, PGN parsing

**Key Files:**
- `interface.cpp/.h` - Game loop and board display
- `parse_pgn.cpp` - SAN move parsing

#### 2.1.4 Core Module (`src/core/`)

**Purpose:** Utility functions

**Key Files:**
- `cli_parser.h` - Command-line argument parsing
- `error_handling.h` - Error logging macros
- `timing.h` - Dual-mode timing system

### 2.2 Component Dependencies

```
Programs (ChessTest, PlayChess, TrainEval)
    ↓
Interface (PlayGame, UCI, PGN)
    ↓
SearchEngine (Think, Search, Evaluate)
    ↓
ChessEngine (MakeMove, GenMoves, Attack)
    ↓
Core (Timing, Error Handling)
```

### 2.3 Global Variable Strategy

Global variables are used for game state and lookup tables for performance. **Key modernization:** Game history arrays are now dynamically allocated:

```cpp
// In globals.h - Dynamically allocated game state
gameHistory:   std::unique_ptr<GameState[]>  // Size: SearchConfig::maxPlysPerGame
movesMade:     std::unique_ptr<MoveStruct[]> // Size: SearchConfig::maxPlysPerGame
currentState:  GameState*                    // Points into gameHistory array
currentColour: int8_t*                       // Alias to currentState->Colour
currentPiece:  int8_t*                       // Alias to currentState->Piece

// Fixed-size lookup tables (stack-allocated for performance)
knightMoves:      int[64][9]              // Knight attack offsets
diagonalMoves:    int[64][4][8]           // Diagonal ray offsets
straightMoves:    int[64][4][8]           // Orthogonal ray offsets
kingMoves:        int[64][9]              // King move offsets
exposedAttackTable: int[64][64]           // Direction between squares
posData:          SquareData[6][64][64]   // Sliding piece move generation
```

### 2.4 Dynamic Memory Management

**Initialization Pattern:**
```cpp
// Before any game operations, initialize globals with desired config
SearchConfig config;
config.maxPlysPerGame = 1000;     // Default
config.hashSizeMB = 512;           // 512MB hash table
initGlobals(config);               // Allocates gameHistory, movesMade arrays

// Use engine...

// Automatic cleanup when unique_ptr goes out of scope
```

---

## 3. Chess Engine Module - Board Representation

### 3.1 Square Indexing

The board uses 0x88-style indexing with 64 contiguous squares:

```
  A B C D E F G H
8 0 1 2 3 4 5 6 7     Rank 8: indices 0-7
7 8 9 10...        15     Rank 7: indices 8-15
...                      ...
1 56...          63     Rank 1: indices 56-63
```

**Helper Functions (marked [[nodiscard]] and noexcept):**
```cpp
[[nodiscard]] inline constexpr int GetFile(int square) noexcept { return square & 7; }
[[nodiscard]] inline constexpr int GetRank(int square) noexcept { return square >> 3; }
[[nodiscard]] inline constexpr int FlipSquare(int square) noexcept { 
    return square ^ 56;  // Equivalent to (56 + square) - (16 * (square / 8))
}
```

### 3.2 Piece and Color Constants

```cpp
// Piece types (array indices)
constexpr int PAWN   = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK   = 3;
constexpr int QUEEN  = 4;
constexpr int KING   = 5;
constexpr int NONE   = -1;

// Colors
constexpr int WHITE  = 0;
constexpr int BLACK  = 1;

[[nodiscard]] inline constexpr int GetOtherSide(int currentSide) noexcept {
    return 1 - currentSide;
}
```

### 3.3 Move Type Flags

Moves use a bitfield for type information:

```cpp
constexpr uint8_t NORMAL_MOVE  = 0;
constexpr uint8_t CAPTURE      = 1;   // 0x01
constexpr uint8_t CASTLE       = 2;   // 0x02
constexpr uint8_t EN_PASSANT   = 4;   // 0x04
constexpr uint8_t TWO_SQUARES  = 8;   // 0x08 - Pawn double advance
constexpr uint8_t PAWN_MOVE    = 16;  // 0x10
constexpr uint8_t PROMOTION    = 32;  // 0x20
```

### 3.4 Castling Permissions

```cpp
constexpr uint8_t WHITE_KING_SIDE  = 1;   // 0x01
constexpr uint8_t WHITE_QUEEN_SIDE = 2;   // 0x02
constexpr uint8_t BLACK_KING_SIDE  = 4;   // 0x04
constexpr uint8_t BLACK_QUEEN_SIDE = 8;   // 0x08
```

### 3.5 Core Data Structures

#### 3.5.1 MoveStruct

Compact 4-byte move representation:

```cpp
struct MoveStruct {
    int8_t Source;     // Source square (0-63, -1 for invalid)
    int8_t Target;     // Target square (0-63, -1 for invalid)
    uint8_t Type;      // Move flags (bitfield)
    uint8_t Promote;   // Promotion piece (0-4)
};
```

#### 3.5.2 GameState

Complete board state for move undo and repetition detection:

```cpp
struct GameState {
    int8_t Colour[BOARD_SQUARES];      // WHITE=0, BLACK=1, NONE=-1 per square
    int8_t Piece[BOARD_SQUARES];       // PAWN=0..KING=5, NONE=-1 per square
    uint8_t CastlePerm;                // Castling permissions (4-bit bitfield)
    int8_t EnPass;                     // En passant target or NO_EN_PASSANT (-1)
    int FiftyCounter;                  // Half-moves since capture/pawn move
    int KingSquare[2];                 // King positions [WHITE, BLACK]
    unsigned InCheck : 1;              // Side to move is in check
    unsigned IsDraw : 1;               // Position is drawn
    HashKey Key;                       // Zobrist hash of position
};
```

#### 3.5.3 MoveList

Container for generated moves with fixed-size array (kept as C-style for performance):

```cpp
struct MoveList {
    MoveStruct Move[MOVELIST_ARRAY_SIZE];  // Array of moves (max 2000)
    int NumMoves;                          // Actual number of moves
};
```

**Note:** MoveList uses a fixed-size C-style array for stack allocation performance during search. The limit (2000) is a hard maximum; generation stops at this limit with a warning.

#### 3.5.4 SquareData

Critical structure for sliding piece move generation:

```cpp
struct SquareData {
    int TestSquare;      // Square to test
    SquareData* Skip;    // Pointer to next ray after blocking piece
};
```

This enables efficient "skip to next ray" when a blocking piece is encountered.

### 3.6 Zobrist Hashing

64-bit hash keys for position identification:

```cpp
// Hash components (initialized with fixed seed for reproducibility)
HashKey g_hashCode[2][6][64];      // [Color][Piece][Square]
HashKey g_epHashCode[64];          // En passant square
HashKey g_castleHashCode[16];      // All 16 castle permission combinations
HashKey g_sideHashCode;            // Side to move

// Incremental update during move making
// Key ^= g_hashCode[color][piece][source];  // Remove from source
// Key ^= g_hashCode[color][piece][target];  // Add to target
```

---

## 4. Chess Engine Module - Move Generation

### 4.1 Overview

The engine uses a **pseudo-legal then filter** approach:
1. Generate all moves that look legal (ignoring check)
2. Make each move, test if it leaves king in check
3. If legal, keep it; otherwise discard

### 4.2 Main Generation Functions

```cpp
void GenLegalMoves(MoveList& Moves);   // Fully legal moves only
void GenMoves(MoveList& Moves);        // Pseudo-legal moves
void GenCaptures(MoveList& Moves);     // Captures only (for quiescence)
void GenPush(MoveList& Moves, int Source, int Target, int Type);
```

### 4.3 Pawn Move Generation

Pawn moves are hardcoded for performance:

**For WHITE pawns:**
- Capture left: `I-9` (if file != 0 and target has BLACK piece)
- Capture right: `I-7` (if file != 7 and target has BLACK piece)
- Push: `I-8` (if empty)
- Double push: `I-16` (if on rank 2, both I-8 and I-16 empty)

**For BLACK pawns (mirrored):**
- Capture left: `I+7`
- Capture right: `I+9`
- Push: `I+8`
- Double push: `I+16`

### 4.4 Sliding/Nonsliding Piece Generation

Uses the `g_posData` lookup table with skip pointers:

```cpp
SquareData* P = g_posData[g_currentPiece[I]][I];
do {
    if (g_currentColour[P->TestSquare] == NONE) {
        GenPush(Moves, I, P->TestSquare, NORMAL_MOVE);
        P++;  // Continue along ray
    } else {
        if (g_currentColour[P->TestSquare] == GetOtherSide(g_currentSide))
            GenPush(Moves, I, P->TestSquare, CAPTURE);
        P = P->Skip;  // Jump to next ray
    }
} while (P->TestSquare != END_OF_LOOKUP);
```

### 4.5 Castling Generation

Castling is generated only if:
1. Not in check (`!g_currentState->InCheck`)
2. King and rook haven't moved (castle permissions)
3. Squares between king and rook are empty
4. King doesn't pass through check (full attack check deferred to `MakeMove()`)

### 4.6 En Passant Generation

Generated when:
1. En passant square is set (not `NO_EN_PASSANT`)
2. Own pawn is positioned to capture
3. Generate capture moves with `EN_PASSANT|CAPTURE` flags

### 4.7 Promotion Handling

When a pawn reaches the final rank:

```cpp
if (isPromotion) {
    for (int I = KNIGHT; I <= QUEEN; I++) {
        // Add 4 promotion moves (N, B, R, Q)
        Move.Type = PAWN_MOVE | PROMOTION;
        Move.Promote = I;
        // Add to move list
    }
}
```

---

## 5. Chess Engine Module - Game State Management

### 5.1 Initialization

```cpp
void InitAll(void);
```

**Algorithm:**
1. Thread-safe one-time initialization of lookup tables via `std::call_once`
2. Set up starting position:
   - Rank 0: rnbqkbnr (BLACK)
   - Rank 1: pppppppp (BLACK)
   - Ranks 2-5: empty
   - Rank 6: PPPPPPPP (WHITE)
   - Rank 7: RNBQKBNR (WHITE)
3. Initialize state variables:
   - `CastlePerm = 15` (all castling allowed)
   - `EnPass = NO_EN_PASSANT`
   - `FiftyCounter = 0`
   - `KingSquare[WHITE] = 60` (E1)
   - `KingSquare[BLACK] = 4` (E8)
4. Set current side to WHITE, move number to 0
5. Compute initial Zobrist key

### 5.2 MakeMove Algorithm

```cpp
bool MakeMove(MoveStruct& MoveToMake);
```

**Returns:** `true` if legal, `false` if illegal

**14-Step Algorithm:**

1. **Save State:** `*(g_currentState+1) = *g_currentState` (copy for undo)

2. **Handle Castling:**
   - Check king doesn't pass through attacked square
   - Move rook to final position
   - Track `ExtraExposedSquare` (rook's new square for check detection)

3. **Update King Square:** If king moved, update `KingSquare[]`

4. **Update Castle Permissions:**
   - King move: clear both sides for that color
   - Rook move or rook captured: clear specific side

5. **Update En Passant:**
   - If two-square pawn move: set `EnPass` to passed square
   - Otherwise: `NO_EN_PASSANT`

6. **Update Fifty Counter:**
   - Reset to 0 if pawn move or capture
   - Increment otherwise

7. **Handle Captures:**
   - Regular capture: XOR out captured piece from hash
   - En passant: Remove pawn from different square, update hash

8. **Move Piece:**
   - Set destination square
   - If promotion: use `Promote` piece type
   - Clear source square
   - Update hash with XOR operations

9. **Switch Sides:** `g_currentSide = GetOtherSide(g_currentSide)`

10. **Test Legality:**
    - If was in check or captured king's adjacent square: full `Attack()` test
    - Else: `TestExposure()` on source square only

11. **If Illegal:** Call `TakeMoveBack()`, return false

12. **Test for Draw:**
    - Fifty move rule (>= 50)
    - Insufficient material
    - Threefold repetition
    - If draw: set `IsDraw = true`, `InCheck = false`

13. **Test for Check (if not draw):**
    - Castling: `TestExposure()` on rook square
    - En passant: `TestExposure()` on captured pawn square
    - Otherwise: `SingleAttack()` on moved piece + `TestExposure()` on source

14. **Save Move:** Store in `g_movesMade[]`

15. **Return true**

### 5.3 TakeMoveBack Algorithm

```cpp
void TakeMoveBack(void);
```

**Algorithm:**
1. `g_currentSide = GetOtherSide(g_currentSide)` (swap back)
2. `g_moveNum--`
3. `g_currentState--` (restore previous state)
4. Update `g_currentColour` and `g_currentPiece` pointers

### 5.4 Attack Detection

#### 5.4.1 Attack() - General Attack Test

```cpp
[[nodiscard]] bool Attack(int Square, int SideAttacking);
```

**Algorithm (Optimized Order - Most Likely First):**
1. **Rooks/Queens (Straight Lines):** Check 4 directions using `g_straightMoves`
2. **Bishops/Queens (Diagonals):** Check 4 directions using `g_diagonalMoves`
3. **Knights:** Check all knight jumps using `g_knightMoves`
4. **Pawns:** Direction depends on attacking side
   - WHITE: check `Square+7` and `Square+9` (pawns move up board)
   - BLACK: check `Square-7` and `Square-9` (pawns move down board)
5. **King:** Check all adjacent squares using `g_kingMoves`

#### 5.4.2 SingleAttack() - Specific Piece Attack

```cpp
[[nodiscard]] bool SingleAttack(int TargetSquare, int NewEnemySquare);
```

**Purpose:** Test if specific piece at `NewEnemySquare` attacks `TargetSquare`

**Used for:** Fast check detection after king non-moves

**Algorithm:**
1. Use `g_exposedAttackTable[TargetSquare][NewEnemySquare]` to get direction
   - -1 = not on same line (no attack possible)
   - 0-3 = straight line direction index
   - 4-7 = diagonal direction index
2. If rook/queen and straight line: verify no blockers
3. If bishop/queen and diagonal: verify no blockers
4. If knight: check `g_knightAttackTable`
5. If pawn: verify rank/file relationship

#### 5.4.3 TestExposure() - Check for Discovered Attack

```cpp
[[nodiscard]] bool TestExposure(int TargetSquare, int EvacuatedSquare, int SideAttacking);
```

**Purpose:** Check if moving piece from `EvacuatedSquare` exposes `TargetSquare` to attack

**Algorithm:**
1. Use `g_exposedAttackTable` to see if squares are aligned
2. If aligned on straight line (0-3): scan for rook/queen
3. If aligned on diagonal (4-7): scan for bishop/queen

### 5.5 Draw Detection

#### 5.5.1 TestRepetition() - Threefold Repetition

```cpp
[[nodiscard]] bool TestRepetition(void);
```

**Algorithm:**
1. Build current position key including:
   - Base Zobrist key
   - Castle permissions via `g_castleHashCode`
   - En passant square via `g_epHashCode` (if valid)
2. Only check if `FiftyCounter >= 8` (need at least 4 moves each)
3. Iterate backwards through history every 2 plies (same side to move)
4. Compare adjusted keys
5. Return true if 2 previous matches found (3 total occurrences)

#### 5.5.2 TestNotEnoughMaterial() - Insufficient Material

```cpp
[[nodiscard]] bool TestNotEnoughMaterial(void);
```

**Algorithm:**
1. Scan all squares, count bishops and knights per side
2. If any pawn, rook, or queen exists: return false
3. Check if both sides have material that cannot force checkmate:
   - 0-0 (K vs K)
   - 0-1 (K vs K+N or K+B)
   - 1-0 (K+N/B vs K)
   - 0-2 (K vs K+N+N)
   - 1-1 (K+N/B vs K+N/B)
   - 2-0 or 0-2 (K+N+N vs K or K vs K+N+N)
   - 2-1, 1-2, 2-2 combinations

---

## 6. Search Engine Module - Alpha-Beta Search

### 6.1 Overview

The search uses **negamax** with **fail-soft alpha-beta pruning** and numerous enhancements.

### 6.2 SearchConfig - Runtime Configuration

**Modernization:** Search limits are now runtime-configurable via the `SearchConfig` struct:

```cpp
struct SearchConfig {
    // Default values (match original compile-time constants)
    static constexpr size_t DEFAULT_MAX_PLYS_PER_GAME = 1000;
    static constexpr size_t DEFAULT_MAX_QUIESCE_DEPTH = 500;
    static constexpr size_t DEFAULT_HASH_SIZE_MB = 512;  // ~16M entries
    
    // Maximum allowed values for validation
    static constexpr size_t MAX_PLYS_PER_GAME_LIMIT = 10000;
    static constexpr size_t MAX_QUIESCE_DEPTH_LIMIT = 10000;
    static constexpr size_t MAX_HASH_SIZE_MB = 1024 * 1024;  // 1TB
    
    // Configurable parameters
    size_t maxPlysPerGame = DEFAULT_MAX_PLYS_PER_GAME;   // Game history size
    size_t maxQuiesceDepth = DEFAULT_MAX_QUIESCE_DEPTH;  // Quiescence array size
    size_t hashSizeMB = DEFAULT_HASH_SIZE_MB;            // Hash table memory
    
    // Computed from hashSizeMB
    size_t numHashSlots = 0;    // Actual number of entries (power of 2)
    size_t hashPow2 = 0;        // Power of 2 for table size
    
    // Runtime flags (formerly compile-time defines)
    bool enableSearchDiagnostics = false;  // Enable search diagnostics output
    
    // Note: USE_NULL_MOVE and USE_HASH_TABLE_IN_QUIESCE are always enabled
    
    // Methods
    void computeHashSize();     // Calculates hashPow2 from hashSizeMB
    [[nodiscard]] bool validate() const;
    [[nodiscard]] size_t getHashMemoryBytes() const;
    [[nodiscard]] size_t getHashMemoryMB() const;
};

// Global instance
extern SearchConfig g_searchConfig;

// Hash key folding function (uses dynamic hashPow2)
[[nodiscard]] inline size_t foldHashKey(HashKey key) {
    return foldHashKey(key, g_searchConfig.hashPow2);
}
```

### 6.2.1 EvaluationConfig - Runtime Evaluation Configuration

**Modernization:** Evaluation features are now runtime-configurable via the `EvaluationConfig` struct:

```cpp
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
};

// Global instance
extern EvaluationConfig g_evaluationConfig;
```

**Static Members in EvaluationParameters:**
The configuration is accessed via static members in the `EvaluationParameters` class for performance:

```cpp
class EvaluationParameters {
    // ... other members ...
    
    // Runtime configuration flags (static - shared across all instances)
    static bool UseLinearTraining;
    static bool UseKingDistanceFeatures;
    static bool UseEmptySquareFeatures;
    static bool UseSuperFastEval;
};
```

These static members are initialized from `g_evaluationConfig` at program startup. Programs can modify `g_evaluationConfig` before creating any `EvaluationParameters` objects to change the default behavior.

**Former Compile-Time Flags:**
The following flags were previously compile-time defines and are now runtime configurable:
- `USE_LINEAR_TRAINING` → `useLinearTraining`
- `USE_KING_DISTANCE_FEATURES` → `useKingDistanceFeatures`
- `USE_EMPTY_SQUARE_FEATURES` → `useEmptySquareFeatures`
- `USE_SUPER_FAST_EVAL` → `useSuperFastEval`

### 6.3 SearchData Structure

Complete search state container:

```cpp
struct SearchData : RunningMaterial {
    EvaluationParameters EP;          // Eval parameters for this search
    
    // Statistics counters
    int TotalNodesSearched;
    int NumAlphaCutOffs, NumBetaCutOffs, NumNullCutOffs;
    int NumMaterialEvals, NumTrueEvals;
    int TotalMoveGens;
    int NumHashCollisions;
    int TotalPutHashCount, TotalGetHashCount, NumHashSuccesses;
    int NumCheckExtensions, NumMateExtensions;
    
    // Iterative deepening
    int IterDepth;
    
    // Timing
    ClockTime StartTime, StopTime;
    double WallClockStart, CPUStart;
    
    // Evaluation window optimization
    int MinPositionEval[MAX_QUIESCE_DEPTH];
    int MaxPositionEval[MAX_QUIESCE_DEPTH];
    int MaxPositionalDiff;
    
    // Aspiration windows
    int RootAlpha, RootBeta;
    
    // Transposition table (dynamically sized)
    std::vector<HashRecord> HashTable;
    
    // Move ordering data
    MoveStruct HashMoves[MAX_QUIESCE_DEPTH];
    int MoveHistory[64][64];
    MoveStruct KillerMovesOld[MAX_QUIESCE_DEPTH];
    MoveStruct KillerMovesNew[MAX_QUIESCE_DEPTH];
    
    // Search result
    MoveStruct ComputersMove;
    int ComputersMoveScore;
};
```

**Key change:** `HashTable` is now a `std::vector<HashRecord>` sized at runtime based on `SearchConfig::numHashSlots`, instead of a fixed C-style array.

### 6.4 Key Constants

```cpp
// Hash table is now dynamic - no compile-time size constant
// constexpr int HASH_POW_2 = 24;  // REMOVED - now in SearchConfig::hashPow2

// These remain as limits used for array sizing
constexpr int MAX_QUIESCE_DEPTH = 500;    // For Min/MaxPositionEval arrays

// Search depth is UNLIMITED - no MAX_SEARCH_DEPTH constant
// Depth limited by quiescence search and time constraints only

constexpr double EVAL_WINDOW = 0.99;      // Lazy evaluation window
constexpr double ASPIRATION_WINDOW = 0.5; // Root aspiration window (pawns)
constexpr int WIN_SCORE = 10000000;       // Mate score base value
constexpr int DRAW_CONTEMPT = 0;          // Draw evaluation
constexpr int PIECE_VALUE[6] = {10000, 30000, 30000, 50000, 90000, 0};
```

### 6.5 Think() - Iterative Deepening Controller

**Purpose:** Main entry point for computer move calculation

**Algorithm:**

```
FUNCTION Think(SearchDepth, MaxTimeSeconds, ShowOutput, ShowThinking, RandomSwing, EP):

  1. INITIALIZE SearchData SD
     - Clear all statistics counters
     - Copy evaluation parameters
     - Mutate EP with RandomSwing for randomization
     - Clear hash table, hash moves, move history, killer moves
     - Initialize Min/MaxPositionEval arrays
     - Allocate HashTable vector with g_searchConfig.numHashSlots entries
  
  2. SETUP TIMING
     - If ShowThinking: record StartTime, WallClockStart, CPUStart
     - If SearchDepth is INFINITE: set StopTime = Now + MaxTimeSeconds
     - Else: set StopTime to maximum (no time limit)
  
  3. PRINT THINKING HEADER
  
  4. INITIALIZE MATERIAL EVALUATION
     - Calculate initial PieceMatValue and PawnMatValue from board
  
  5. INITIALIZE ASPIRATION WINDOW
     - RootAlpha = -WIN_SCORE
     - RootBeta = +WIN_SCORE
  
  6. ITERATIVE DEEPENING LOOP (for SD.IterDepth = 1 to ...):
     
     a. ASPIRATION SEARCH LOOP:
        - Call Search(SD, 0, RootAlpha, RootBeta, IterDepth, false)
        - Store result in LastScore
        - IF timeout: break
        
        - IF LastScore <= RootAlpha:  // Fail low
            RootAlpha = -WIN_SCORE
            Continue aspiration loop
            
        - ELSE IF LastScore >= RootBeta:  // Fail high
            RootBeta = +WIN_SCORE
            Continue aspiration loop
            
        - ELSE: Break aspiration loop (success)
     
     b. UPDATE ASPIRATION WINDOW for next iteration:
        - RootAlpha = LastScore - ASPIRATION_WINDOW * PAWN_VALUE
        - RootBeta = LastScore + ASPIRATION_WINDOW * PAWN_VALUE
     
     c. PRINT PROGRESS (if ShowThinking):
        - Print PV line with current best move and score
        - Use '%' if timed out, '.' if complete
     
     d. CHECK TERMINATION CONDITIONS:
        - Depth reached AND no time limit
        - Mate score found
        - Timeout occurred
  
  7. PRINT FINAL STATISTICS
  
  8. RETURN SD.ComputersMove
```

**Key change:** Iterative deepening now continues indefinitely until stopped by time or a mate score is found. There is no `MAX_SEARCH_DEPTH` limit.

### 6.6 Search() - Main Alpha-Beta Algorithm

```cpp
int Search(SearchData& SD, int CurrentPly, int Alpha, int Beta, 
           int Depth, bool NullMove);
```

**Algorithm:**

```
FUNCTION Search(SD, CurrentPly, Alpha, Beta, Depth, NullMove):

  1. TIMEOUT CHECK
     - If shouldTimeOut(): return 0
  
  2. CHECK EXTENSION
     - If CurrentPly < MAX_QUIESCE_DEPTH AND InCheck:
         Depth++
         SD.NumCheckExtensions++
  
  3. QUIESCENCE BOUNDARY
     - If Depth <= 0:
         X = QuiesceSearch(SD, CurrentPly, Alpha, Beta, NullMove)
         If isMateScore(X):
             SD.NumMateExtensions++
             Depth++
         Else:
             Return X
  
  4. NODE INITIALIZATION
     - SD.TotalNodesSearched++
     - Propagate Min/MaxPositionEval to next ply
     - BestMove = empty
     - SaveAlpha = Alpha
     - Best = -WIN_SCORE
  
  5. DRAW CHECK
     - If IsDraw: Best = 0; goto LeaveSearch
  
  6. TRANSPOSITION TABLE PROBE
     - Flags = TTGet(SD, CurrentPly, Depth, X, HashMove, NextKey)
     - IF EXACTSCORE: return X
     - IF UPPERBOUND: Beta = min(Beta, X); if Beta <= Alpha: return X
     - IF LOWERBOUND: if X >= Beta: return X; else Alpha = X
  
  7. NULL MOVE PRUNING
     Conditions:
     - CurrentPly > 1
     - Not already in null move
     - Depth > 1
     - Not in check
     - MaterialEval + PAWN > Beta (we're ahead)
     - Beta > -(WIN_SCORE-1) + CurrentPly (not in forced mate)
     - Our piece material > BISHOP (avoid zugzwang)
     - Opponent has some material
     
     If all met:
     - Make null move (switch sides, clear en passant)
     - X = -Search(SD, CurrentPly, -Beta, -Beta+1, Depth-3, true)
     - Undo null move
     - If X >= Beta: SD.NumNullCutOffs++; Best = X; goto LeaveSearch
  
  8. MOVE GENERATION AND ORDERING
     - GenMoves(Moves)
     - SD.TotalMoveGens++
     - ScoreMoves(SD, CurrentPly, Moves, MoveScores, NullMove)
  
  9. MAIN SEARCH LOOP (for each move):
     
     a. Sort to get best unscored move
     b. Try MakeMove(), skip illegal
     c. UpdateMaterialEvaluation()
     
     d. IF first move:
            Found = true
            X = -Search(SD, CurrentPly+1, -Beta, -Alpha, Depth-1, NullMove)
        
        ELSE (PVS):
            Alpha = max(Best, Alpha)  // Fail-soft
            X = -Search(SD, CurrentPly+1, -Alpha-1, -Alpha, Depth-1, NullMove)
            
            If X > Best AND X > Alpha AND X < Beta:
                X = -Search(SD, CurrentPly+1, -Beta, -X, Depth-1, NullMove)
     
     e. Get NextHashKey = g_currentState->Key
     f. TakeMoveBack()
     g. Timeout check
     
     h. IF X > Best:
            BestMove = current move
            BestHashKey = NextHashKey
            Best = X
            
            IF CurrentPly == 0:
                IF Best <= RootAlpha: PrintLine with '-'; return Best
                IF Best >= RootBeta: PrintLine with '+'; return Best
            
            IF Best >= Beta:
                SD.NumBetaCutOffs++
                goto LeaveSearch
            
            IF CurrentPly == 0 AND Best > RootAlpha AND Best < RootBeta:
                SD.ComputersMove = BestMove
                SD.ComputersMoveScore = Best
                PrintLine with '&'
     
     i. Shortest mate detection:
        IF Best + CurrentPly == WIN_SCORE - 1: goto LeaveSearch
  
  10. CHECKMATE/STALEMATE
      If no legal moves:
          IF InCheck: Best = -WIN_SCORE + CurrentPly
          ELSE: Best = 0
  
  11. LEAVESEARCH
      TTPut(SD, CurrentPly, Depth, SaveAlpha, Beta, Best, BestMove, BestHashKey)
      
      IF Best > SaveAlpha AND BestMove is valid:
          SD.NumAlphaCutOffs++
          SD.MoveHistory[source][target] |= (1 << Depth)
          
          IF not capture AND not promotion:
              IF KillerMovesOld[CurrentPly] is empty:
                  KillerMovesOld[CurrentPly] = BestMove
              ELSE IF different from Old:
                  KillerMovesNew[CurrentPly] = BestMove
      
      Return Best
```

### 6.7 Key Heuristics Summary

| Heuristic | Implementation |
|-----------|----------------|
| Negamax | With alpha-beta pruning |
| Fail-Soft | Alpha = max(Best, Alpha) |
| Aspiration Windows | Narrow window at root, expand on fail |
| Iterative Deepening | Progressive depth increase, unlimited depth |
| Transposition Table | Stores exact/lower/upper bounds, dynamic size |
| Null Move Pruning | R=3 reduction with conditions |
| Check Extensions | Depth++ when in check |
| Mate Extensions | Depth++ when mate score from quiescence |
| PVS | Zero-window search, re-search if promising |
| Killer Moves | Two slots per ply + ply-2 killers |
| History Heuristic | Incremented by (1 << Depth) on alpha improvement |
| Shortest Mate | Cut search at WIN_SCORE-1 |

**Key change:** No `MAX_SEARCH_DEPTH` - search depth is now practically unlimited.

---

## 7. Search Engine Module - Quiescence Search

### 7.1 Overview

Quiescence search explores only forcing lines (captures, promotions, checks) to reach tactically quiet positions for evaluation.

### 7.2 QuiesceSearch Algorithm

```cpp
int QuiesceSearch(SearchData& SD, int CurrentPly, int Alpha, int Beta, bool NullMove);
```

**Algorithm:**

```
FUNCTION QuiesceSearch(SD, CurrentPly, Alpha, Beta, NullMove):

  1. TIMEOUT CHECK
     - If shouldTimeOut(): return 0
  
  2. NODE INITIALIZATION
     - SD.TotalNodesSearched++
     - Propagate Min/MaxPositionEval
     - BestMove = empty
     - SaveAlpha = Alpha
  
  3. DRAW CHECK
     - If IsDraw: Best = 0; goto LeaveQSearch
  
  4. TRANSPOSITION TABLE PROBE
     - Flags = TTGet(SD, CurrentPly, 0, X, HashMove, NextKey)
     - Handle EXACTSCORE, UPPERBOUND, LOWERBOUND as in main search
  
  5. STAND PAT (Initial Evaluation)
     MEval = MaterialEval(CurrentPly)
     
     IF CurrentPly > 0 AND 
        (MEval - MinPositionEval[ply-1] + EVAL_WINDOW*PAWN < Alpha OR
         MEval - MaxPositionEval[ply-1] - EVAL_WINDOW*PAWN > Beta):
         SD.NumMaterialEvals++
         Best = MEval  // Lazy evaluation
     ELSE:
         SD.NumTrueEvals++
         PEval = SD.EP.Eval()
         Best = MEval + PEval
         
         MinPositionEval[CurrentPly] = min(MinPositionEval[CurrentPly], PEval)
         MaxPositionEval[CurrentPly] = max(MaxPositionEval[CurrentPly], PEval)
  
  6. MOVE GENERATION
     IF InCheck:
         GenMoves(Moves)  // All moves when in check
     ELSE:
         IF Best >= Beta: goto LeaveQSearch
         GenCaptures(Moves)  // Only captures/promotions
  
  7. SCORE AND SORT MOVES
     - SD.TotalMoveGens++
     - ScoreMoves(...)
     - Alpha = max(Best, Alpha)  // Fail-soft
  
  8. SEARCH LOOP
     FOR each move:
         Sort to get best move
         IF !MakeMove(): continue
         UpdateMaterialEvaluation()
         Found = true
         
         X = -QuiesceSearch(SD, CurrentPly+1, -Beta, -Alpha, NullMove)
         
         TakeMoveBack()
         Timeout check
         
         IF X > Best:
             BestMove = current move
             BestHashKey = NextHashKey
             Best = X
             
             IF Best >= Beta:
                 SD.NumBetaCutOffs++
                 goto LeaveQSearch
             
             IF Best > Alpha:
                 Alpha = Best
         
         ELSE IF X >= Beta:
             SD.NumBetaCutOffs++
             Best = X
             goto LeaveQSearch
         
         IF Best + CurrentPly == WIN_SCORE - 1:
             goto LeaveQSearch
  
  9. CHECKMATE DETECTION
     IF no legal moves AND InCheck:
         Best = -WIN_SCORE + CurrentPly
  
   10. LEAVEQSEARCH
       - Store in TT
       - Update statistics
       - Return Best
```

### 7.3 Quiescence-Specific Features

- **Stand Pat:** Current position score is lower bound
- **Delta Pruning:** Lazy evaluation with EVAL_WINDOW = 0.99 pawn
- **Check Handling:** Generate all moves when in check
- **Capture-Only:** Only captures/promotions when not in check
- **Same Heuristics:** Uses hash, killers, history from main search

---

## 8. Search Engine Module - Heuristics

### 8.1 Move Ordering Priority

Priority order (highest to lowest):

| Priority | Condition | Score Formula |
|----------|-----------|---------------|
| 1 | Hash move (from TT) | PV_SORT_SCORE (1,000,000,000) |
| 2 | Promotion | PROMOTION_SORT_SCORE + PromotePiece*10 |
| 3 | Capture | CAPTURE_SORT_SCORE + (Victim*10 - Attacker) |
| 4 | Killer (same ply, old) | KILLER_SORT_SCORE + (History << 3) |
| 5 | Killer (same ply, new) | KILLER_SORT_SCORE + (History << 3) |
| 6 | Killer (ply-2, old) | KILLER_SORT_SCORE/2 + (History << 3) |
| 7 | Killer (ply-2, new) | KILLER_SORT_SCORE/2 + (History << 3) |
| 8 | Castling | (History << 3) \| 7 |
| 9 | King move (non-castle) | (History << 3) |
| 10 | Other moves | (History << 3) + (PieceType + 1) |

### 8.2 MVV-LVA Capture Scoring

```
MoveScores[I] = CAPTURE_SORT_SCORE + (capturedPieceValue * 10) - movingPieceValue
```

Bonus for capturing last moved piece:
```cpp
IF g_moveNum > 1 AND NOT nullMove AND
   (piece on target changed from 2 moves ago OR color on target changed):
    MoveScores[I]++
```

### 8.3 Killer Moves

Two slots per ply plus ply-2 killers:

```cpp
MoveStruct KillerMovesOld[MAX_QUIESCE_DEPTH];  // Primary killers
MoveStruct KillerMovesNew[MAX_QUIESCE_DEPTH];  // Secondary killers
```

Updated on alpha cutoff (non-captures, non-promotions only):
```cpp
IF not capture AND not promotion:
    IF KillerMovesOld[CurrentPly] is empty:
        KillerMovesOld[CurrentPly] = BestMove
    ELSE IF different from Old:
        KillerMovesNew[CurrentPly] = BestMove
```

### 8.4 History Heuristic

64x64 table indexed by [source][target]:

```cpp
int MoveHistory[64][64];
```

Updated on alpha improvement:
```cpp
SD.MoveHistory[source][target] |= (1 << Depth);
```

Used in move scoring:
```cpp
MoveScores[I] += (MoveHistory[source][target] << 3);
```

### 8.5 Selection Sort

Simple but effective for small move lists:

```cpp
void Sort(MoveList& Moves, int MoveScores[MOVELIST_ARRAY_SIZE], int Source) {
    int BestScore = INT_MIN;
    int BestIndex = Source;
    
    for (int I = Source; I < Moves.NumMoves; I++) {
        if (MoveScores[I] > BestScore) {
            BestScore = MoveScores[I];
            BestIndex = I;
        }
    }
    
    // Swap moves and scores
    std::swap(Moves.Move[Source], Moves.Move[BestIndex]);
    std::swap(MoveScores[Source], MoveScores[BestIndex]);
}
```

---

## 9. Search Engine Module - Transposition Table

### 9.1 HashRecord Structure

```cpp
struct HashRecord {
    MoveStruct Move;       // Best move found at this position
    int Score;             // Score of the position
    uint8_t Flags;         // Node type flags
    uint8_t Depth;         // Depth searched
    HashKey Key;           // Full hash key for verification
    HashKey NextKey;       // Hash key after best move
};
```

**Entry size:** ~32 bytes  
**Table size:** Dynamically computed from `SearchConfig::hashSizeMB` (default 512MB → ~16M entries)

### 9.2 Flags

```cpp
constexpr uint8_t LOWERBOUND  = 1;  // Score is lower bound
constexpr uint8_t UPPERBOUND  = 2;  // Score is upper bound
constexpr uint8_t EXACTSCORE  = 4;  // Exact score
constexpr uint8_t QUIESCENT   = 8;  // Quiescent node
```

### 9.3 Key Folding

Maps 64-bit hash to table index using dynamic power-of-2 size:

```cpp
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
```

### 9.4 TTPut - Store Position

```cpp
void TTPut(SearchData& SD, int CurrentPly, int Depth, int Alpha, int Beta,
           int Score, MoveStruct Move, HashKey NextKey);
```

**Algorithm:**
1. Compute full hash key including castle and en passant
2. Get table index via `foldHashKey()` (using `g_searchConfig.hashPow2`)
3. **Replacement policy:** Don't replace deeper entries (unless mate score)
4. Count collisions if entry already exists
5. Store entry with mate score adjustment
6. Determine flag based on score vs Alpha/Beta

### 9.5 TTGet - Retrieve Position

```cpp
[[nodiscard]] uint8_t TTGet(SearchData& SD, int CurrentPly, int Depth, int& Score,
               MoveStruct& Move, HashKey& NextKey);
```

**Algorithm:**
1. Compute full hash key
2. Get table index using `foldHashKey()`
3. Verify entry matches key (full 64-bit comparison)
4. Check depth is sufficient (unless mate score)
5. Adjust mate scores by ply
6. Return flags (EXACTSCORE, LOWERBOUND, UPPERBOUND, or 0 if not found)

### 9.6 Mate Score Adjustment

Mate scores are stored relative to root, adjusted by ply:

```cpp
// Store
IF isMateScore(Score):
    Hash->Score = Score + (Score > 0 ? CurrentPly : -CurrentPly);

// Retrieve
IF isMateScore(Score):
    Score -= (Score > 0 ? CurrentPly : -CurrentPly);
```

---

## 10. Evaluation System Architecture

### 10.1 Overview

The evaluation uses a **3-stage system** with **2,796 trainable parameters**:

- **Opening** (>10 pieces): Development, center control
- **Middlegame** (7-10 pieces): Tactical play
- **Endgame** (≤6 pieces): Pawn promotion focus

### 10.2 Stage Determination

```cpp
[[nodiscard]] int GetStage() const {
    int NumPieces = 0;
    for (int I = 0; I < BOARD_SQUARES; I++) {
        if (g_currentPiece[I] != NONE && 
            g_currentPiece[I] != PAWN && 
            g_currentPiece[I] != KING)
            NumPieces++;
    }
    
    if (NumPieces > MIDDLE_GAME_PIECES)      // > 10
        return OPENING;
    else if (NumPieces > END_GAME_PIECES)    // 7-10
        return MIDDLE_GAME;
    else                                      // <= 6
        return END_GAME;
}
```

### 10.3 EvaluationParameters Class

```cpp
class EvaluationParameters {
    int Stage;
    double Offset;  // Training offset
    
    // Pawn structure tracking
    int PawnCount[2][10];
    int PawnRank[2][10];
    
    // Minor piece tracking
    bool HasKnights[2];
    bool HasWhiteSquareBishop[2];
    bool HasBlackSquareBishop[2];
    
    // PIECE-SQUARE TABLES: [STAGE][PIECE_TYPE][SQUARE]
    // Piece indices: 0-5 = White Pawn-King
    //                 6-11 = Black Pawn-King
    //                 12 = Empty square
    double PSValues[NUM_STAGES][13][64];
    
    // King distance features (optional)
    double KingDistanceOwn[NUM_STAGES][12];
    double KingDistanceOther[NUM_STAGES][12];
    
    // SINGULAR WEIGHTS: [STAGE][WEIGHT_INDEX][ASYMMETRY]
    // Asymmetry: 0 = our feature, 1 = opponent's feature
    double Weights[NUM_STAGES][NUM_WEIGHTS][2];
};
```

### 10.4 The 38 Singular Weights

| Index | Constant Name | Description |
|-------|---------------|-------------|
| 0 | CASTLE_BONUS | Bonus for castling |
| 1 | CASTLE_MISSING_PAWN | Penalty for missing pawn shield |
| 2 | CASTLE_FIENCHETTO | Bonus for fianchetto defense |
| 3 | CASTLE_KNIGHT_PROT | Bonus for knight protecting king |
| 4 | ROOK_NO_MOVE | Bonus for not moving rook before castling |
| 5 | FOREPOST_BONUS | Standard forepost bonus (Knight) |
| 6 | FOREPOST_BONUS | Standard forepost bonus (Bishop) |
| 7 | PROTECTED_FOREPOST | Protected by pawn (Knight) |
| 8 | PROTECTED_FOREPOST | Protected by pawn (Bishop) |
| 9 | ABSOLUTE_FOREPOST | Can't be attacked by enemy minor (Knight) |
| 10 | ABSOLUTE_FOREPOST | Can't be attacked by enemy minor (Bishop) |
| 11 | PAWN_INFRONT_FOREPOST | Enemy pawn directly in front (Knight) |
| 12 | PAWN_INFRONT_FOREPOST | Enemy pawn directly in front (Bishop) |
| 13 | ROOK_OPEN_FILE | Rook on open file |
| 14 | ROOK_SEMI_OPEN_FILE | Rook on semi-open file |
| 15 | QUEEN_OPEN_FILE | Queen on open file |
| 16 | QUEEN_SEMI_OPEN_FILE | Queen on semi-open file |
| 17 | KING_OPEN_FILE | King on open file (penalty) |
| 18 | KING_SEMI_OPEN_FILE | King on semi-open file (penalty) |
| 19 | KING_OPEN_FILE_SIDE | Open file to side of king |
| 20 | KING_SEMI_OPEN_FILE_SIDE | Semi-open file to side of king |
| 21 | KING_ROOK_XRAY | Opposing rook x-ray on king |
| 22 | KING_QUEEN_XRAY | Opposing queen x-ray on king |
| 23 | PAWN_STORM | Enemy pawn storming king position |
| 24 | BATTERY_BONUS | Battery (2+ major pieces aligned) |
| 25 | FIENCHETTO | Fianchettoed bishop bonus |
| 26 | NO_BLOCK_KNIGHT | Knight at c3/f3 with pawn above, not blocked |
| 27 | DOUBLED_PAWN | Doubled pawn penalty |
| 28 | ISOLATED_PAWN | Isolated pawn penalty |
| 29 | BACKWARD_PAWN | Backward pawn penalty |
| 30 | SEMI_BACKWARD_PAWN | Semi-backward pawn penalty |
| 31 | BACKWARD_ATTACK | Backward pawn square attacked |
| 32 | ADJACENT_PAWNS | Adjacent pawns bonus |
| 33 | PAWN_CHAIN | Pawn chain (protected by pawn) bonus |
| 34 | PASSED_PAWN | Passed pawn bonus (scaled by rank) |
| 35 | PROT_PASSED_PAWN | Protected passed pawn bonus |
| 36 | BLOCKED_PASSED_PAWN | Blocked passed pawn penalty |
| 37 | PAWN_MAJORITY | Local pawn majority bonus |

### 10.5 Evaluation Algorithm

**Two-pass evaluation:**

**Pass 1: Setup Pawn Structure**
```cpp
// Initialize pawn tracking
for (int I = 0; I < 10; I++) {
    PawnCount[WHITE][I] = 0;  PawnCount[BLACK][I] = 0;
    PawnRank[WHITE][I] = -1;  PawnRank[BLACK][I] = 8;
}

// Reset minor piece flags
HasKnights[0] = HasKnights[1] = false;
HasWhiteSquareBishop[0] = HasWhiteSquareBishop[1] = false;
HasBlackSquareBishop[0] = HasBlackSquareBishop[1] = false;

// Scan board
for (int I = 0; I < 64; I++) {
    if (g_currentPiece[I] == PAWN) {
        PawnCount[color][file+1]++;
        PawnRank[color][file+1] = max(PawnRank[color][file+1], rank);
    }
    else if (g_currentPiece[I] == KNIGHT) {
        HasKnights[color] = true;
    }
    else if (g_currentPiece[I] == BISHOP) {
        Determine square color and set flag;
    }
}
```

**Pass 2: Evaluate Each Square**
```cpp
for (int I = 0; I < BOARD_SQUARES; I++) {
    // OUR PIECES
    if (g_currentColour[I] == g_currentSide) {
        // Piece-square value (flip for black perspective)
        Score += AW(PSValues[Stage][piece][FlipSquare(I)]);
        
        // King distance features (if enabled)
        Score += KingDistanceOwn * distance_to_own_king;
        Score += KingDistanceOther * distance_to_enemy_king;
    }
    // OPPONENT'S PIECES
    else if (g_currentColour[I] == GetOtherSide(g_currentSide)) {
        Score += AW(PSValues[Stage][piece+6][FlipSquare(I)]);
        Score += KingDistanceOwn * distance_to_own_king;
        Score += KingDistanceOther * distance_to_enemy_king;
    }
    // EMPTY SQUARES (if enabled)
    else {
        Score += AW(PSValues[Stage][12][FlipSquare(I)]);
    }
    
    // Piece-specific evaluation
    Score += EvalPawn(I) / EvalKnight(I) / EvalBishop(I) / 
             EvalRook(I) / EvalQueen(I) / EvalKing(I);
}
```

### 10.6 Training Weight Access Macros

```cpp
#ifdef TRAINING
  #define AW(W) ((W)+=Offset)                    // Add offset during training
  #define AW_Scale(W,SF) ((W+=(Offset*(SF)))*(SF))
  #define AW_Sing(W,S) ((W[S])+=Offset)
  #define AW_Sing_Scale(W,S,SF) (((W[S])+=(Offset*(SF)))*(SF))
#else
  #define AW(W) (W)                              // Just read value
  #define AW_Scale(W,SF) ((W)*(SF))
  #define AW_Sing(W,S) (W[S])
  #define AW_Sing_Scale(W,S,SF) ((W[S])*(SF))
#endif
```

### 10.7 Piece-Specific Evaluation

#### Pawn Evaluation
```cpp
[[nodiscard]] double EvalPawn(int Square) {
    // 1. DOUBLED PAWNS
    if (pawn_behind_on_same_file)
        Score += Weights[DOUBLED_PAWN][S];
    
    // 2. ISOLATED PAWNS
    if (no_friendly_pawns_on_adjacent_files)
        Score += Weights[ISOLATED_PAWN][S];
    
    // 3. BACKWARD/SEMI-BACKWARD PAWNS
    else if (neighbors_advanced_further) {
        if (no_hostile_pawn_on_this_file)
            Score += Weights[BACKWARD_PAWN][S];
        else
            Score += Weights[SEMI_BACKWARD_PAWN][S];
        
        if (attacked_by_enemy_pawns)
            Score += Weights[BACKWARD_ATTACK][S];
    }
    
    // 4. ADJACENT PAWNS
    if (friendly_pawn_to_left OR friendly_pawn_to_right)
        Score += Weights[ADJACENT_PAWNS][S];
    
    // 5. PAWN CHAIN
    if (protected_by_pawn_from_behind) {
        Score += Weights[PAWN_CHAIN][S];
        ProtectedBy++;
    }
    
    // 6. PASSED PAWNS
    if (passed_pawn) {
        Score += Weights[PASSED_PAWN][S] * advancement_factor;
        
        if (ProtectedBy > 0)
            Score += Weights[PROT_PASSED_PAWN][S] * ProtectedBy;
        
        if (blocked_by_piece_in_front)
            Score += Weights[BLOCKED_PASSED_PAWN][S];
    }
    
    // 7. PAWN MAJORITY
    if (our_pawns_in_window > their_pawns_in_window)
        Score += Weights[PAWN_MAJORITY][S];
    
    return Score;
}
```

#### Knight/Bishop Forepost Evaluation
```cpp
[[nodiscard]] double ForepostBonus(int Square) {
    // FOREPOST: Advanced outpost protected by pawns
    if (no_enemy_pawns_can_attack &&
        GetRank(Square) > 0 (for white) / < 7 (for black)) {
        
        Score += Weights[FOREPOST_BONUS + B][S];
        
        if (pawn_protects_from_left)
            Score += Weights[PROTECTED_FOREPOST + B][S];
        if (pawn_protects_from_right)
            Score += Weights[PROTECTED_FOREPOST + B][S];
        
        if (enemy_pawn_in_front)
            Score += Weights[PAWN_INFRONT_FOREPOST + B][S];
    }
    
    // ABSOLUTE FOREPOST
    if (enemy_has_no_knights) {
        if (square_color_matches && enemy_has_no_bishop_of_that_color)
            Score += Weights[ABSOLUTE_FOREPOST + B][S];
    }
    
    return Score;
}
```

#### King Safety Evaluation
```cpp
[[nodiscard]] double EvalKing(int Square) {
    // PAWN STORM detection
    if (king_on_queenside_castled_position) {
        Check enemy pawns at A2, B2, C2, A3, B3, C3;
        Add Weights[PAWN_STORM][S] for each;
    }
    else if (king_on_kingside_castled_position) {
        Check enemy pawns at F2, G2, H2, F3, G3, H3;
        Add Weights[PAWN_STORM][S] for each;
    }
    
    // CASTLING BONUS
    if (king_on_castled_position) {
        Score += Weights[CASTLE_BONUS][S];
        
        if (missing_pawn_shield)
            Score += Weights[CASTLE_MISSING_PAWN][S];
        
        if (fianchetto_compensation)
            Score += Weights[CASTLE_FIENCHETTO][S];
        
        if (knight_at_C3_or_F3)
            Score += Weights[CASTLE_KNIGHT_PROT][S];
    }
    
    // Open/Semi-open file penalties
    if (no_own_pawns_on_king_file) {
        if (no_enemy_pawns)
            Score += Weights[KING_OPEN_FILE][S];
        else
            Score += Weights[KING_SEMI_OPEN_FILE][S];
    }
    
    // X-ray attacks
    for (all_squares) {
        if (same_file_or_rank_as_king) {
            if (enemy_rook) Score += Weights[KING_ROOK_XRAY][S];
            if (enemy_queen) Score += Weights[KING_QUEEN_XRAY][S];
        }
    }
    
    return Score;
}
```

---

## 11. Evaluation System Training Algorithm

### 11.1 Overview

The training uses **temporal difference learning** with batch updates.

### 11.2 Training Function

```cpp
[[nodiscard]] double Train(double DesiredOutput, double LearningRate, double& Output);
```

**Algorithm:**
```
1. Forward pass: get current evaluation
   Output = EvalAndLearn(0.0);

2. Calculate error
   Error = (DesiredOutput - Activation(Output));

3. Calculate offset using gradient
   Offset = LearningRate * Error * Gradient(Output);

4. Backward pass: update weights
   Output = Activation(EvalAndLearn(Offset));

5. Return squared error
   return Error * Error;
```

### 11.3 Activation Functions

**Linear (default):**
```cpp
Activation(Value) = Value;
Gradient(Value) = 1.0;
```

**Sigmoid (alternative):**
```cpp
Activation(Value) = (1.0 - exp(-Value)) / (1.0 + exp(-Value));
Gradient(Value) = (2.0 * exp(-Value)) / ((1.0 + exp(-Value))^2);
```

### 11.4 Batch Updates

Key insight: Learning rate should be **exactly 1/N** where N = number of positions.

**Logic:** If all positions want a value to change by X, with LR=1/N it changes by exactly X. Larger causes overstepping, smaller causes slower convergence.

### 11.5 Quiescent Training

Only train on quiet positions:
- Position marked as quiescent in training data
- No captures, checks, or promotions pending
- Stored in binary format to save computation

### 11.6 Training Data Format

**From game outcomes:**
- Win: Target = +1.0 (or 0.95 with sigmoid)
- Loss: Target = -1.0 (or -0.95)
- Draw: Target = 0.0

**1.5 pawn critical value:** Positions with material difference > 1.5 pawns are excluded from training (search should handle these).

---

## 12. Interface Module - UCI and PGN

### 12.1 UCI Protocol Commands

**GUI to Engine:**

| Command | Description |
|---------|-------------|
| `uci` | Initialize UCI mode |
| `debug [on\|off]` | Toggle debug mode |
| `isready` | Synchronization |
| `setoption name <id> [value <x>]` | Set engine parameter |
| `ucinewgame` | New game starting |
| `position [fen <fen>\|startpos] moves <m1>...` | Set position |
| `go [subcommands]` | Start search |
| `stop` | Stop searching |
| `ponderhit` | User played expected move |
| `quit` | Exit program |

**Go Subcommands:**
- `searchmoves <moves>` - Restrict search
- `ponder` - Pondering mode
- `wtime <x>`, `btime <x>` - Time remaining (ms)
- `winc <x>`, `binc <x>` - Increment (ms)
- `movestogo <x>` - Moves to next time control
- `depth <x>` - Search x plies only
- `nodes <x>` - Search x nodes only
- `mate <x>` - Search for mate in x
- `movetime <x>` - Search exactly x ms
- `infinite` - Search until `stop`

**Engine to GUI:**

| Command | Description |
|---------|-------------|
| `id name <x>` | Engine name |
| `id author <x>` | Author name |
| `uciok` | UCI mode ready |
| `readyok` | Ready for commands |
| `bestmove <move> [ponder <move>]` | Search result |
| `info [subcommands]` | Search information |
| `option name <x> type <t> ...` | Available options |

**Info Subcommands:**
- `depth <x>`, `seldepth <x>` - Search depth
- `time <x>`, `nodes <x>`, `nps <x>` - Statistics
- `pv <moves>` - Principal variation
- `score cp <x>\|mate <y>` - Evaluation score
- `currmove <m>`, `currmovenumber <x>` - Current search
- `hashfull <x>` - TT fill (per mille)
- `string <str>` - Free text

**Move Format:** Long algebraic notation (e.g., `e2e4`, `e1g1`, `e7e8q`)

### 12.2 PGN Parsing

**SAN Move Parsing Algorithm:**

```cpp
[[nodiscard]] bool ConvertFromSAN(const char* SAN_Move, MoveStruct& Alg_Move);
```

**Algorithm:**
1. Generate all legal moves for current position
2. **Castling Detection:**
   - `O-O-O` or `0-0-0` (queenside): Source=60/4, Target=58/2
   - `O-O` or `0-0` (kingside): Source=60/4, Target=62/6
3. **Piece Move (K, Q, R, B, N):**
   - Work backwards to find target square
   - Check for disambiguation (file/rank specifiers)
   - Search generated moves to find matching piece
4. **Pawn Move:**
   - Simple push: Calculate source from target
   - Capture: Find target, calculate source ±8 ±1
5. **Validation:** Search legal moves, verify with MakeMove

**Supported Formats:**
- Castling: `O-O`, `O-O-O`, `0-0`, `0-0-0`
- Piece moves: `Nf3`, `Bxe5`, `Rad1`, `R1d2`
- Pawn moves: `e4`, `exd5`, `exd5=Q`, `exd5Q`
- Annotations: `!`, `?`, `+`, `#` ignored

### 12.3 Game Loop

**PlayGame() Algorithm:**
```
1. Set ComputerSide based on ModeOfPlay
2. Initialize game: InitAll(), GenMoves()
3. Infinite loop:
   a. Print board (orientation depends on ComputerSide)
   b. Check draw conditions
   c. Check for legal moves
   d. If no legal moves: checkmate or stalemate
   e. If computer's turn:
      - Call Think() to get best move
      - Print move in algebraic notation
      - MakeMove(ComputersMove)
   f. If human's turn:
      - Read input string
      - Handle commands (q=quit, t=takeback, ?=help)
      - Parse move in coordinate notation
      - Find matching legal move
      - MakeMove if valid
```

**Move Notation:**
- Input: Coordinate notation (e.g., `e2e4`, `e7e8q` for promotion)
- Output: Extended format with capture indicators
  - `e2-e4` for normal move
  - `e2xe4` for capture
  - `e1*g1` for castling
  - `e7-e8=Q` for promotion

### 12.4 Board Display

**PrintBoard():**
- Two orientations: WHITE up or BLACK up
- Empty squares: `.` (light) and `#` (dark) for checkerboard
- White pieces: uppercase (P, N, B, R, Q, K)
- Black pieces: lowercase (p, n, b, r, q, k)
- Coordinates shown (a-h, 1-8)

---

## 13. Programs and Entry Points

### 13.1 ChessTest

**Purpose:** Run test positions and verify correct moves

**Usage:**
```bash
./ChessTest [OPTIONS] <test_file> <eval_set>
  -t, --time <seconds>    Search time per position (default: 10.0)
      --cpu-time          Use CPU time instead of wall clock
      --hash-size <MB>    Hash table size in MB (default: 512)
```

**Algorithm:**
1. Load evaluation set
2. Parse .fin test file (FEN-like format)
3. Initialize globals with SearchConfig
4. For each position:
   - Set up position
   - Run search for specified time
   - Compare best move against desired move(s)
   - Record correct/incorrect
5. Print statistics

### 13.2 PlayChess

**Purpose:** Main chess engine for playing games

**Usage:**
```bash
./PlayChess [OPTIONS]
  -m, --mode <mode>          Game mode: computer-black, computer-white, 
                              two-humans, two-computers
  -w, --white-set <file>     White evaluation set
  -b, --black-set <file>     Black evaluation set
  -d, --depth <plies>        Search depth, 0=use time (default: unlimited)
  -t, --time <seconds>       Search time per move (default: 1.0)
  -n, --games <n>            Number of games to play
  -r, --random-swing <value> Random evaluation swing
      --thinking             Show thinking output
  -q, --quiet                Minimal output
      --bell                 Beep after computer moves
      --cpu-time             Use CPU time
      --hash-size <MB>       Hash table size in MB (default: 512)
```

**Algorithm:**
1. Parse command-line arguments
2. Load evaluation set(s)
3. Initialize globals with SearchConfig
4. Call PlayGame() with specified mode
5. Run game loop until completion
6. Print result and statistics

### 13.3 TrainEval

**Purpose:** Train evaluation weights from game database

**Usage:**
```bash
./TrainEval <database> <eval_set>
  database    Training database file (.min)
  eval_set    Evaluation set file (.set)
```

**Algorithm:**
1. Load evaluation set (or create new)
2. Open training database
3. Initialize globals with SearchConfig
4. For each game in database:
   - Parse moves
   - For each position:
     - Calculate target based on game outcome
     - Call Train() to update weights
5. Save updated evaluation set
6. Print training statistics

### 13.4 Utility Programs

**convert_from_pgn:**
```bash
./convert_from_pgn <input.pgn> <output.bin>
```
Converts PGN files to binary format with detailed move encoding.

**normalize_eval_set:**
```bash
./normalize_eval_set <input.set> <output.set> <scale_factor>
```
Scales evaluation weights by specified factor.

**randomize_games:**
```bash
./randomize_games <input.bin> <output.bin>
```
Randomizes game order in training database.

---

## 14. File Formats

### 14.1 Evaluation Set (.set)

Binary format containing trained weights:
- Header: Magic number + version
- PSValues: [NUM_STAGES][13][64] doubles (piece-square tables)
- Weights: [NUM_STAGES][NUM_WEIGHTS][2] doubles (singular weights)
- KingDistance: Optional king distance weights

### 14.2 Test Position (.fin)

FEN-like text format:
```
# Comment line
rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1
ExpectedMove1 ExpectedMove2 ...
```

### 14.3 Training Database (.min/.bin)

Binary format for training:

**Header (2 bytes):**
- Bits 1-15: Number of moves in game (unsigned)
- Bit 0: Result (1=White win, 0=Draw, interpreted as -1 for Black win)

**Moves (3 bytes each):**
- Bits 0-3: Move type flags
- Bits 4-9: Target square (0-63)
- Bits 10-15: Source square (0-63)
- Bit 16: Quiescent flag (position is quiet)

**Game Separator:**
- Null byte `\0` between games (for sorting)

---

## 15. Build System

### 15.1 Makefile Structure

The build system uses separate compilation with automatic dependency generation:

```makefile
# Library source files
CHESS_ENGINE_SRCS = src/chess_engine/*.cpp
SEARCH_ENGINE_SRCS = src/search_engine/*.cpp
INTERFACE_SRCS = src/interface/*.cpp
CORE_SRCS = src/core/*.cpp

# Object files compiled to obj/
# Automatic dependency tracking (.d files)
```

### 15.2 Build Commands

```bash
make all              # Build all executables
make ChessTest        # Test suite
make TrainEval        # Training tool
make PlayChess        # Main engine
make debug            # Debug build
make clean            # Clean artifacts
make convert_from_pgn # PGN converter
make normalize_eval_set
make randomize_games
```

### 15.3 Compiler Flags

**Production:**
```
-std=c++20 -O3 -Wall -Wextra -Wno-char-subscripts -Wno-register -march=native
```

**Debug:**
```
-std=c++20 -O0 -g -Wall -Wextra
```

### 15.4 Modern C++ Features Used

1. **`#pragma once`** - Modern include guards
2. **`constexpr`** - Compile-time constants
3. **`[[nodiscard]]`** - Enforce return value usage
4. **`noexcept`** - Non-throwing function contracts
5. **`std::unique_ptr<>`** - Dynamic array ownership
6. **`std::vector<>`** - Dynamic hash table storage
7. **`std::min/max`** - Type-safe min/max (replacing macros)
8. **`std::swap`** - Standard swap utility
9. **`static_cast<>`** - Explicit type conversions
10. **`std::call_once`** - Thread-safe one-time initialization
11. **`std::chrono`** - High-resolution timing

---

## 16. Appendices

### 16.1 SearchConfig Reference

```cpp
struct SearchConfig {
    // Static defaults
    static constexpr size_t DEFAULT_MAX_PLYS_PER_GAME = 1000;
    static constexpr size_t DEFAULT_MAX_QUIESCE_DEPTH = 500;
    static constexpr size_t DEFAULT_HASH_SIZE_MB = 512;
    
    // Limits
    static constexpr size_t MAX_PLYS_PER_GAME_LIMIT = 10000;
    static constexpr size_t MAX_QUIESCE_DEPTH_LIMIT = 10000;
    static constexpr size_t MAX_HASH_SIZE_MB = 1024 * 1024;  // 1TB
    
    // Runtime values
    size_t maxPlysPerGame;    // Game history array size
    size_t maxQuiesceDepth;   // Quiescence array size
    size_t hashSizeMB;        // Requested hash memory
    bool enableSearchDiagnostics;  // Enable search diagnostics output (default: false)
    size_t numHashSlots;      // Computed: actual entries
    
    // Note: USE_NULL_MOVE and USE_HASH_TABLE_IN_QUIESCE are always enabled (not configurable)
    size_t hashPow2;          // Computed: power of 2
    
    // Methods
    void computeHashSize();           // Calculate hashPow2 from hashSizeMB
    [[nodiscard]] bool validate() const;
    [[nodiscard]] size_t getHashMemoryBytes() const;
    [[nodiscard]] size_t getHashMemoryMB() const;
};
```

### 16.2 EvaluationConfig Reference

Runtime configuration for evaluation features:

```cpp
struct EvaluationConfig {
    // King distance features - bonus/penalty based on distance to kings
    // Default: false (disabled as it may hurt playing strength)
    bool useKingDistanceFeatures = false;
    
    // Empty square features - piece-square values for empty squares
    // Default: true (enabled, appears to help)
    bool useEmptySquareFeatures = true;
    
    // Super fast eval - use only piece-square features
    // Default: false (full evaluation)
    bool useSuperFastEval = false;
    
    // Linear training - use simple linear training instead of sigmoid
    // Default: true (must match training method used for eval set)
    bool useLinearTraining = true;
};
```

**Note:** These flags were previously compile-time defines (USE_KING_DISTANCE_FEATURES, USE_EMPTY_SQUARE_FEATURES, USE_SUPER_FAST_EVAL, USE_LINEAR_TRAINING) and are now runtime configurable.

**Usage:**
```cpp
// Set before initializing EvaluationParameters
g_evaluationConfig.useLinearTraining = false;
g_evaluationConfig.useEmptySquareFeatures = false;

// Now create evaluation parameters with new settings
EvaluationParameters EP;
```

### 16.3 Constants Summary

| Constant | Value | Description |
|----------|-------|-------------|
| BOARD_SQUARES | 64 | Squares on board |
| MOVELIST_ARRAY_SIZE | 2000 | Max moves per position |
| MAX_QUIESCE_DEPTH | 500 | Array sizing limit |
| WIN_SCORE | 10000000 | Mate score base |
| PIECE_VALUE[PAWN] | 10000 | Pawn value in centipawns |
| PIECE_VALUE[KNIGHT] | 30000 | Knight value |
| PIECE_VALUE[BISHOP] | 30000 | Bishop value |
| PIECE_VALUE[ROOK] | 50000 | Rook value |
| PIECE_VALUE[QUEEN] | 90000 | Queen value |

### 16.4 Type Aliases

```cpp
using HashKey = uint64_t;     // Zobrist hash keys
using ClockTime = uint64_t;   // Timing measurements
```

### 16.5 Modernization Checklist

- ✅ C++20 standard
- ✅ `#pragma once` include guards
- ✅ `constexpr` instead of `#define`
- ✅ `[[nodiscard]]` for important returns
- ✅ `noexcept` for non-throwing functions
- ✅ Dynamic allocation for game history
- ✅ Dynamic hash table sizing
- ✅ Removed MAX_SEARCH_DEPTH limit
- ✅ `std::unique_ptr` for array ownership
- ✅ `std::vector` for dynamic containers
- ✅ `static_cast` instead of C casts
- ✅ `std::min/max` instead of macros
- ✅ `std::swap` standard utility
- ✅ `std::call_once` for initialization
- ✅ Standard C++ headers (`<cstdint>`, etc.)

---

**End of Document**
