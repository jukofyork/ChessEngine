# Chess Engine Makefile - Modern C++20 Build System
# Uses separate compilation with individual .cpp files as compilation units

CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -march=native -MMD -MP
CXXFLAGS_DEBUG = -std=c++20 -O0 -g -Wall -Wextra -MMD -MP

# Directories
SRCDIR = src
OBJDIR = obj

# Include paths  
INCLUDES = -I$(SRCDIR)/chess_engine -I$(SRCDIR)/search_engine -I$(SRCDIR)/interface -I$(SRCDIR)/core

# Source files by module
CHESS_ENGINE_SRCS = $(SRCDIR)/chess_engine/globals.cpp \
                    $(SRCDIR)/chess_engine/lookup_tables.cpp \
                    $(SRCDIR)/chess_engine/hash_key_codes.cpp \
                    $(SRCDIR)/chess_engine/game_history.cpp \
                    $(SRCDIR)/chess_engine/move_generation.cpp \
                    $(SRCDIR)/chess_engine/attack_tests.cpp \
                    $(SRCDIR)/chess_engine/draw_tests.cpp

SEARCH_ENGINE_SRCS = $(SRCDIR)/search_engine/think.cpp \
                     $(SRCDIR)/search_engine/evaluation.cpp \
                     $(SRCDIR)/search_engine/evaluation_config.cpp \
                     $(SRCDIR)/search_engine/material_evaluation.cpp \
                     $(SRCDIR)/search_engine/search.cpp \
                     $(SRCDIR)/search_engine/quiescent_search.cpp \
                     $(SRCDIR)/search_engine/move_ordering.cpp \
                     $(SRCDIR)/search_engine/quick_search.cpp \
                     $(SRCDIR)/search_engine/transposition_table.cpp \
                     $(SRCDIR)/search_engine/search_config.cpp

INTERFACE_SRCS = $(SRCDIR)/interface/interface.cpp \
                 $(SRCDIR)/interface/parse_pgn.cpp

# All library source files (excluding main programs)
LIB_SRCS = $(CHESS_ENGINE_SRCS) $(SEARCH_ENGINE_SRCS) $(INTERFACE_SRCS)

# Object files
LIB_OBJS = $(LIB_SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Main targets
TARGETS = ChessTest TrainEval PlayChess

.PHONY: all clean debug dirs

# Default target
all: dirs $(TARGETS)

# Create object directories
dirs:
	@mkdir -p $(OBJDIR)/chess_engine $(OBJDIR)/search_engine $(OBJDIR)/interface $(OBJDIR)/programs

# Pattern rule for compiling source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ChessTest executable
ChessTest: $(OBJDIR)/programs/chess_test.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# TrainEval executable
TrainEval: $(OBJDIR)/programs/train_eval.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# PlayChess executable
PlayChess: $(OBJDIR)/programs/play_game.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Utility programs
convert_from_pgn: $(OBJDIR)/programs/convert_from_pgn.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

normalize_eval_set: $(OBJDIR)/programs/normalize_eval_set.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

randomize_games: $(OBJDIR)/programs/randomize_games.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Debug build
debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: clean all

# Clean build artifacts
clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGETS) convert_from_pgn normalize_eval_set randomize_games

# Prevent make from deleting intermediate files
.SECONDARY: $(LIB_OBJS) $(OBJDIR)/programs/*.o

# Dependencies are handled automatically by the pattern rule
# Add explicit dependencies for headers that affect many files
$(OBJDIR)/chess_engine/%.o: $(SRCDIR)/chess_engine/chess_engine.h
$(OBJDIR)/search_engine/%.o: $(SRCDIR)/search_engine/search_engine.h $(SRCDIR)/search_engine/evaluation.h
$(OBJDIR)/interface/%.o: $(SRCDIR)/interface/interface.h

$(OBJDIR)/programs/%.o: $(SRCDIR)/chess_engine/chess_engine.h $(SRCDIR)/search_engine/search_engine.h $(SRCDIR)/interface/interface.h

# Include auto-generated dependency files
# These are created by the -MMD flag and track header dependencies
-include $(LIB_OBJS:.o=.d)
-include $(OBJDIR)/programs/*.d

# End of Makefile
