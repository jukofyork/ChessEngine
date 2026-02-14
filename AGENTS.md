# Agent Guidelines for Chess Engine

C++20 chess engine with alpha-beta search, trainable evaluation, and UCI protocol support.

## Build Commands

```bash
make all              # Build all executables (ChessTest, TrainEval, PlayChess)
make ChessTest        # Test suite
make TrainEval        # Evaluation training tool
make PlayChess        # Main chess engine
make debug            # Debug build (make clean first recommended)
make clean            # Clean build artifacts
make convert_from_pgn # PGN to binary converter
make normalize_eval_set
make randomize_games
```

## Recent Architecture Changes (February 2026)

### Removed MAX_SEARCH_DEPTH Limit
- Search depth is now unlimited (iterative deepening continues until time limit or mate)
- Search is practically limited by quiescence search and time constraints only

### Dynamic Memory Management
- `g_gameHistory` and `g_movesMade` are now `std::unique_ptr<[]>` (dynamically allocated)
- Hash table is `std::vector<HashRecord>` sized at runtime via `SearchConfig`
- Call `initGlobals(config)` before using the engine to allocate arrays

### SearchConfig Runtime Configuration
The `SearchConfig` struct provides runtime-configurable search parameters:

```cpp
SearchConfig config;
config.hashSizeMB = 512;        // Hash table memory (default: 512MB)
config.maxPlysPerGame = 1000;   // Game history size (default: 1000)
config.maxQuiesceDepth = 500;   // Quiescence array size (default: 500)
config.enableSearchDiagnostics = false;  // Enable search diagnostics output
initGlobals(config);            // Must call before engine use
```

See `src/search_engine/search_config.h` for full details.

### Removed Conditional Compilation Flags
The following compile-time flags have been removed and replaced with runtime configuration or always-enabled features:

**Always Enabled (no longer configurable):**
- `USE_NULL_MOVE` - Null move heuristic is always active
- `USE_HASH_TABLE_IN_QUIESCE` - Hash table is always used in quiescence search
- `USE_HASH_MOVE_ONLY` - Full hash table features always enabled

**Converted to Runtime Configuration:**
- `ENABLE_SEARCH_DIAGNOSTICS` → `SearchConfig.enableSearchDiagnostics` (default: false)
- Evaluation flags moved to `EvaluationConfig`:
  - `USE_LINEAR_TRAINING` → `useLinearTraining` (default: true)
  - `USE_KING_DISTANCE_FEATURES` → `useKingDistanceFeatures` (default: false)
  - `USE_EMPTY_SQUARE_FEATURES` → `useEmptySquareFeatures` (default: true)
  - `USE_SUPER_FAST_EVAL` → `useSuperFastEval` (default: false)

See `src/search_engine/evaluation_config.h` for evaluation configuration details.

## Test Commands

```bash
# Run single test file
./ChessTest -t 10 data/test_positions/larsen1.fin data/evaluation_sets/best_so_far.set

# Run all tests
./scripts/run_all_tests.sh

# Train evaluation
./scripts/train_evaluation.sh

# Get help
./ChessTest --help
./PlayChess --help
./TrainEval --help
```

Test files are in `data/test_positions/*.fin` format (FEN-like).

## Code Style

### Language Standard
- **C++20** with g++ 11+
- Use modern C++ features; keep C-style arrays for performance-critical code

### Naming Conventions (Updated February 2026)
- **Constants**: `constexpr` UPPER_CASE (e.g., `constexpr int MAX_DEPTH = 30`)
- **Types**: PascalCase (e.g., `MoveStruct`, `SearchData`)
- **Functions**: camelCase (e.g., `makeMove`, `think`, `search`)
- **Variables**: camelCase (e.g., `moveNum`, `currentPly`, `bestScore`)
- **Global variables**: `g_` prefix + camelCase (e.g., `g_moveNum`, `g_currentSide`)
- **Loop variables**: lowercase single letters (e.g., `i`, `j`, `k`)
- **Macros**: UPPER_CASE, prefer `constexpr`

### Header Organization
```cpp
#pragma once  // Use pragma once for include guards

// 1. Standard C++ headers
#include <cstdint>
#include <iostream>

// 2. Project headers with relative paths
#include "../chess_engine/chess_engine.h"

// 3. Constants, types, function prototypes
constexpr int MAX_MOVES = 2000;
struct MyStruct { ... };
void MyFunction();
```

### Code Formatting
- **Indentation**: 2 spaces (no tabs)
- **Braces**: Opening brace on same line
- **Line length**: ~100 characters max
- **Comments**: Use `//` (avoid `/* */` except file headers)

### Function Style
```cpp
void functionName(int param) {
  if (condition) {
    doSomething();
  }
}
```

### Modernization Guidelines
1. Use C++ headers (`<cstdint>` not `<stdint.h>`)
2. Use `constexpr` instead of `#define`
3. Use `inline` for functions in headers
4. Declare globals as `extern` in headers, define in .cpp files
5. Use `[[nodiscard]]` for important return values
6. Use `noexcept` for non-throwing functions
7. Use C++ `<random>` directly
8. Use `src/core/timing.h` for time measurement

### Error Handling
```cpp
// Use macros from error_handling.h
FATAL_ERROR("Error message");     // Exits program
LOG_WARNING("Warning message");   // Continues execution
ASSERT(condition, "Message");     // Assert with fatal error
```

### Performance
- Use C-style arrays for lookup tables
- Mark `inline` for functions in headers
- Prefer stack allocation over heap

## Project Structure

```
src/
  chess_engine/    # Board representation, move generation
  search_engine/   # Alpha-beta search, evaluation
  interface/       # UCI protocol, PGN parsing, game loop
  core/            # Utilities (timing, error_handling, cli_parser)
  programs/        # Main entry points

data/
  test_positions/  # .fin test files
  evaluation_sets/ # .set weight files
```

## Compiler Flags

- **Production**: `-std=c++20 -O3 -Wall -Wextra -Wno-char-subscripts -Wno-register -march=native`
- **Debug**: `-std=c++20 -O0 -g -Wall -Wextra`

## Build Pattern

The Makefile uses separate compilation:
- Source files compile to `obj/*.o`
- Don't add new source files to Makefile directly—add to appropriate LIB_SRCS variable

## Git

Do not commit:
- Build artifacts (ChessTest, TrainEval, PlayChess, obj/)
- Large data files (>10MB, like all_random.dat)
- IDE files

## CLI Interface

All programs use standardized Unix-style CLI via `src/core/cli_parser.h`:
- **Help**: `-h` or `--help`
- **Options**: POSIX-style (`-t 5`) or GNU-style (`--time=5`)
- **Boolean flags**: No value needed (`--thinking`, `--quiet`)

### Timing Modes
- **Wall clock** (default): `std::chrono::steady_clock`
- **CPU time**: Use `--cpu-time` flag (useful with `nice`)

### Program Usage

**ChessTest**:
```bash
./ChessTest [OPTIONS] <test_file> <eval_set>
  -t, --time <seconds>    Search time per position (default: 10.0)
      --cpu-time          Use CPU time instead of wall clock
```

**PlayChess**:
```bash
./PlayChess [OPTIONS]
  -m, --mode <mode>          Game mode (default: computer-black)
  -w, --white-set <file>     White evaluation set
  -b, --black-set <file>     Black evaluation set
  -d, --depth <plies>        Search depth (0=use time)
  -t, --time <seconds>       Search time per move (default: 1.0)
  -n, --games <n>            Number of games
  -r, --random-swing <val>   Random evaluation swing
      --thinking             Show thinking output
  -q, --quiet                Minimal output
      --bell                 Beep after computer moves
      --cpu-time             Use CPU time
```

**TrainEval**:
```bash
./TrainEval <database> <eval_set>
  database    Training database file (.min)
  eval_set    Evaluation set file (.set)
```

**Utility Programs**:
```bash
./convert_from_pgn <input.pgn> <output.bin>
./normalize_eval_set <input.set> <output.set> <scale_factor>
./randomize_games <input.bin> <output.bin>
```
