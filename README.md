# Chess Engine - Modernized C++20 Version

This is a modernized version of a chess engine originally developed between 2001-2012, now updated to compile with C++20 on Linux using g++ and a simple Makefile build system.

## Summary of Changes

### Build System
- **Removed**: Visual Studio project files (.sln, .vcproj, .vcxproj)
- **Removed**: Windows-specific build artifacts and binaries
- **Added**: Simple Makefile for g++ on Linux
- **Compiler**: C++20 standard with g++

### Code Modernization
1. **Headers**: Updated from C headers (stdio.h, stdlib.h) to C++ equivalents (cstdio, cstdlib, cstdint, etc.)
2. **Constants**: Converted #define macros to constexpr where appropriate
3. **Types**: Changed from `byte` typedef to standard types (removed conflict with C++20 std::byte)
4. **Functions**: Added `inline` to header-defined functions to prevent multiple definition errors
5. **Globals**: Moved global variable definitions to globals.cc to avoid linker errors
6. **Includes**: Fixed all include dependencies and circular references
7. **Random**: Replaced custom random.h with direct C++ `<random>` library usage
8. **Timing**: Replaced clock64.h with dual-mode timing system (wall clock vs CPU time)

### Project Structure
```
src/
├── chess_engine/     # Core chess engine (move generation, board representation)
├── search_engine/    # AI search algorithms (alpha-beta, quiescence search)
├── interface/        # UCI protocol and PGN parsing
├── core/             # Utility functions (timing, error handling)
├── programs/         # Main executables entry points
data/
├── evaluation_sets/  # Trained evaluation weights (.set files)
data/test_positions/  # Test position files (.fin format)
```

## Building

### Requirements
- g++ (version 11 or higher recommended for C++20 support)
- Linux operating system

### Build Commands

```bash
# Build all main executables
make all

# Build specific targets
make ChessTest      # Test suite
make TrainEval      # Evaluation training tool
make PlayChess      # Main chess engine

# Debug build
make debug

# Clean build artifacts
make clean
```

## Executables

### ChessTest
Test suite that runs the engine on test positions.
```bash
./ChessTest data/test_positions/larsen1.fin data/evaluation_sets/best_so_far.set [search_time_seconds]

# Use CPU time instead of wall clock (useful with nice)
./ChessTest -t 10 --cpu-time data/test_positions/larsen1.fin data/evaluation_sets/best_so_far.set
```

### PlayChess
Main chess engine that can play games via UCI protocol or against itself.
```bash
./PlayChess

# Use CPU time for time control
./PlayChess -t 5 --cpu-time

# Show thinking output
./PlayChess --thinking
```

### TrainEval
Training tool for the evaluation function using machine learning.
```bash
./TrainEval data/training/all_random.dat data/evaluation_sets/my.set
```

## Dual Timing System

The engine now supports two timing modes for search time control:

- **Wall Clock** (default): Uses `std::chrono::steady_clock` for real-world time. Best for actual play where you need moves within a fixed time limit.
- **CPU Time**: Uses `std::clock()` for process CPU time. Useful when running with `nice` or when you want to measure actual computation time regardless of system load.

### Output
After each search, both wall clock and CPU times are reported:
```
Wall Clock Time                 : 0.50002 seconds
CPU Time                        : 0.50001 seconds
Total Nodes Searched            : 1207301
Nodes Per Second (wall clock)   : 2414505
Nodes Per Second (CPU)          : 2414553
```

### Usage
Use the `--cpu-time` flag with ChessTest or PlayChess to use CPU time for time control:
```bash
# Normal mode (wall clock time)
./ChessTest -t 10 positions.set eval.set

# CPU time mode
./ChessTest -t 10 --cpu-time positions.set eval.set
```

## Training Data

The `all_random.dat` file (702MB) contains training positions in binary format. It was created from PGN game databases using the `convert_from_pgn` utility.

### Binary Format
- **Header**: 2 bytes = (NumMoves << 1) | (Result & 1)
  - Result: 1=White win, -1=Black win, 0=Draw
- **Moves**: 3 bytes each = (Source << 10) | (Target << 4) | (Type & 0xF) | (Quiescent flag)
- **Terminator**: '\0' between games (for sorting)

### To Regenerate Training Data
```bash
# Convert PGN files to binary format
./convert_from_phn games.pgn positions.bin

# Combine multiple files
cat *.bin > combined.bin

# Randomize
./randomize_games combined.bin all_random.dat
```

## Test Files

Test positions in `.fin` format (FEN-like) are in the `data/test_positions/` directory:
- `larsen1.fin`, `larsen2.fin`, `larsen3.fin` - Larsen test positions
- `hardmid.fin` - Hard middle game positions
- `openings.fin` - Opening positions
- `positional.fin` - Positional play tests
- And many more...

## Technical Details

### Engine Features
- **Search**: Alpha-beta pruning with quiescence search, unlimited depth
- **Evaluation**: Trainable linear weighted sum with piece-square tables, runtime-configurable features
- **Hash Table**: Runtime-configurable transposition table (512MB default, adjustable)
- **Move Ordering**: History heuristic and killer moves
- **Time Management**: Configurable thinking time with wall clock or CPU time
- **Memory**: Dynamic allocation for game history and hash table
- **Configuration**: Runtime configuration for search diagnostics and evaluation features

### Performance
On a modern CPU, the engine achieves:
- ~2,000,000+ nodes per second (wall clock)
- ~500,000+ nodes per second (CPU time with --cpu-time)
- Searches 4-5 million nodes in 10 seconds

## Original Development History

From the original readme:
- **2001-2003**: Initial development with neural network evaluation
- **2003_v8**: Slim-lined evaluation with staged features (opening/middlegame/endgame)
- **2008**: Return to development, linear training approach
- **2012**: Final updates before hiatus
- **2026**: Modernized to C++20, Makefile build system, dual timing system
- **2026 v2**: Unlimited search depth, dynamic memory allocation, SearchConfig runtime configuration
- **2026 v3**: Removed conditional compilation flags, runtime evaluation configuration

## Known Issues

1. **Warnings**: Some compiler warnings about uninitialized variables and sprintf buffer sizes (these exist in the original code)
2. **Platform**: Currently Linux-only (should work on macOS with minor changes)
3. **Data Files**: The 702MB training file is not included in git (add to .gitignore)

## Credits

Original engine developed by the author over many years. This modernization preserves all the original algorithms and evaluation logic while making the code compile with modern C++ standards.

## License

See original source files for licensing terms.

---

**Build Date**: February 2026  
**Compiler**: g++ with C++20  
**Platform**: Linux

For more information about the original engine, see `docs/readme.txt` and `docs/features.txt`.

## Quick Start

```bash
# Clone/navigate to the repository
cd /home/juk/temp/Work/Chess/v1

# Build everything
make all

# Run a test
./ChessTest data/test_positions/larsen1.fin data/evaluation_sets/best_so_far.set

# Run a test with CPU time mode
./ChessTest -t 10 --cpu-time data/test_positions/larsen1.fin data/evaluation_sets/best_so_far.set

# Play a game
./PlayChess

# Play a game with CPU time
./PlayChess -t 5 --cpu-time --thinking
```

Enjoy playing with this classic chess engine!
