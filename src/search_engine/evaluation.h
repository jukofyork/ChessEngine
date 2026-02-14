// ****************************************************************************
// *                            EVALUATION HEADER FILE                        *
// ****************************************************************************

#pragma once

#include <algorithm>
#include <array>
#include <iomanip>
#include "../chess_engine/types.h"
#include "../chess_engine/chess_engine.h"
#include "../chess_engine/globals.h"
#include "evaluation_config.h"

// =============================================================================

// Note: Feature flags are now runtime configurable via EvaluationConfig.
// See evaluation_config.h for the runtime configuration options.
// The default values match the previous compile-time defaults.

// =============================================================================

// CONSTANTS:

// The name of each stage.
constexpr int OPENING = 0;
constexpr int MIDDLE_GAME = 1;
constexpr int END_GAME = 2;

// The number of stages we have.
constexpr int NUM_STAGES = 3;

// These control what stage (Opening, Middle Game) we are in.
constexpr int MIDDLE_GAME_PIECES = 10; // If Total No. of Pieces <= N: Middle game.
constexpr int END_GAME_PIECES = 6;     // If Total No. of Pieces <= N: Ending game.

// -----------------------------------------------------------------------------

// *** These are the 'singular' weight indexes ***

// These are the bonuses for castling (op/mid).
constexpr int CASTLE_BONUS = 0;

// These are the penalties for not having pawns infron of the castled king.
constexpr int CASTLE_MISSING_PAWN = 1;

// These are the bonuses for having a fienchetto defence infront of castle king.
constexpr int CASTLE_FIENCHETTO = 2;

// These are the bonuses for having a knight at c3 or f3 to protect king.
constexpr int CASTLE_KNIGHT_PROT = 3; /* ???? - strange larning value? */

// Used to give a bonus to *not* moving a rook before king has moved/castled.
constexpr int ROOK_NO_MOVE = 4;

// These are the forepost bonuses.
// NOTE: There *MUST* be two features for each (1st=Knight, 2nd=Bishop).
constexpr int FOREPOST_BONUS = 5;        // Standard forepost.
constexpr int PROTECTED_FOREPOST = 7;    // Protected by 1/2 own pawns (x2 for 2).
constexpr int ABSOLUTE_FOREPOST = 9;     // Cant' be attacked by enemy minor peice.
constexpr int PAWN_INFRONT_FOREPOST = 11; // Enemy pawn on square in-front.
constexpr int BLANK_FEATURE_BISHOP = 12; // <- Just so it doesn't get lost!!!!

// These are the bonuses/penalties for the king, queen or rook being on an
// open file (ie: no pawns at all), or semi-open file (ie: no own pawns).
constexpr int ROOK_OPEN_FILE = 13;
constexpr int ROOK_SEMI_OPEN_FILE = 14;
constexpr int QUEEN_OPEN_FILE = 15;
constexpr int QUEEN_SEMI_OPEN_FILE = 16;
constexpr int KING_OPEN_FILE = 17;
constexpr int KING_SEMI_OPEN_FILE = 18;

// These two king safety functions check if a king has open/semi-open files to
// either of its sides (op/mid/end).
constexpr int KING_OPEN_FILE_SIDE = 19;
constexpr int KING_SEMI_OPEN_FILE_SIDE = 20;

// These are to test if we have an opposing rook or queen on the same file or
// rank as the king.
constexpr int KING_ROOK_XRAY = 21;
constexpr int KING_QUEEN_XRAY = 22;

// These are used to tell if a castled king is being pawn stormed (op/mid).
constexpr int PAWN_STORM = 23;

// These are the bonuses for getting a file or rank battery with majour pieces.
constexpr int BATTERY_BONUS = 24;  // NOTE: x2 if there are 3 in a row!

// These are the bonuses for fienchotting a bishop to the left or right.
// NOTE: Must have pawn to edge and above...
constexpr int FIENCHETTO = 25;

// These are used to see if it's a f3/c3 knight with pawn above it and not 
// below/blocked in.
constexpr int NO_BLOCK_KNIGHT = 26;

// These are all the pawn features.
constexpr int DOUBLED_PAWN = 27;
constexpr int ISOLATED_PAWN = 28;
constexpr int BACKWARD_PAWN = 29;
constexpr int SEMI_BACKWARD_PAWN = 30;
constexpr int BACKWARD_ATTACK = 31;
constexpr int ADJACENT_PAWNS = 32;
constexpr int PAWN_CHAIN = 33;
constexpr int PASSED_PAWN = 34;
constexpr int PROT_PASSED_PAWN = 35; /* ???? - strange larning value? */
constexpr int BLOCKED_PASSED_PAWN = 36;
constexpr int PAWN_MAJORITY = 37;

// The number of 'singular' weight indexes (see above).
constexpr int NUM_WEIGHTS = 38;

// =============================================================================

// MODERN INLINE FUNCTIONS:

// The flipSquare function is used to calculate the piece/square values for DARK
// pieces. The piece/square value of a WHITE pawn is PAWN_VALUE_MASK[sq] and
// the value of a BLACK pawn is PAWN_VALUE_MASK[flipSquare(Square)].
// This makes sure the board is reflected about the middle.
// Black king moves badly if not done!
// no_class10: This function replaces the old 64 element 'flip' vector.
[[nodiscard]] inline int flipSquare(int square) noexcept {
  return (56 + square) - (16 * (square / 8));
}

// -----------------------------------------------------------------------------

// This returns the distance from one square to another (Manhattan 0-14).
[[nodiscard]] inline int getManhattanDistance(int sq1, int sq2) noexcept {
  return (std::abs(getFile(sq1) - getFile(sq2)) + std::abs(getRank(sq1) - getRank(sq2)));
}

// This returns the distance from one square to another (Straight 0-7).
[[nodiscard]] inline int getStraightDistance(int sq1, int sq2) noexcept {
  return std::max(std::abs(getFile(sq1) - getFile(sq2)), std::abs(getRank(sq1) - getRank(sq2)));
}

// This returns the distance from one square to another (Min 0-7).
[[nodiscard]] inline int getMinDistance(int sq1, int sq2) noexcept {
  return std::min(std::abs(getFile(sq1) - getFile(sq2)), std::abs(getRank(sq1) - getRank(sq2)));
}

// These are used to return distances to own and other king.
[[nodiscard]] inline int getDistanceToOwnKingManhattan(int square) noexcept {
  return getManhattanDistance(square, g_currentState->kingSquare[g_currentColour[square]]);
}

[[nodiscard]] inline int getDistanceToOwnKingStraight(int square) noexcept {
  return getStraightDistance(square, g_currentState->kingSquare[g_currentColour[square]]);
}

[[nodiscard]] inline int getDistanceToOwnKingMin(int square) noexcept {
  return getMinDistance(square, g_currentState->kingSquare[g_currentColour[square]]);
}

[[nodiscard]] inline int getDistanceToOtherKingManhattan(int square) noexcept {
  return getManhattanDistance(square, g_currentState->kingSquare[1 - g_currentColour[square]]);
}

[[nodiscard]] inline int getDistanceToOtherKingStraight(int square) noexcept {
  return getStraightDistance(square, g_currentState->kingSquare[1 - g_currentColour[square]]);
}

[[nodiscard]] inline int getDistanceToOtherKingMin(int square) noexcept {
  return getMinDistance(square, g_currentState->kingSquare[1 - g_currentColour[square]]);
}

// #############################################################################
// #                   'EvaluationParameters' CLASS DEFINITION                 #
// #############################################################################

class EvaluationParameters
{ // This is used to hold, train and use all the feature values.

  // PRIVATE CLASS DATA:

  // These are saved outside of the eval function so that others can see it.
  int    stage;
  double offset;

  // Note: these two arrays have extra files to simplify checking A/H pawns.
  // Note: If a file has no pawns... pawnRank is as though off edge off board.
  std::array<std::array<int, 10>, 2> pawnCount;              // Number of [Colour][File] pawns.
  std::array<std::array<int, 10>, 2> pawnRank;               // Least advanced pawn for [Colour][File].

  // Used to find what minor peices (Knight, w-sq/bsq B-sg) a player has.
  std::array<bool, 2> hasKnights;
  std::array<bool, 2> hasWhiteSquareBishop;
  std::array<bool, 2> hasBlackSquareBishop;

  // These are the piece-square values for each piece, on each square.
  double psValues[NUM_STAGES][13][64];

  // These are the king-distance values for each piece.
  std::array<std::array<double, 12>, NUM_STAGES> kingDistanceOwn;
  std::array<std::array<double, 12>, NUM_STAGES> kingDistanceOther;

  // These are the 'singular' weights (x NUM_STAGES, x2=for asymetry).
  double weights[NUM_STAGES][NUM_WEIGHTS][2];

  // Runtime configuration flags (static - shared across all instances)
  static bool useLinearTraining;
  static bool useKingDistanceFeatures;
  static bool useEmptySquareFeatures;
  static bool useSuperFastEval;

  public:

  // STANDARD MEMEBER FUNCTION PROTOTYPES:
  EvaluationParameters() {};                     // Default constructor.

  // PUBLIC (USER) MEMEBER FUNCTION PROTOTYPES:
  void randomize(double maxInit);                // Random init of values.
  void mutate(const double randomSwing);         // Alter each weight...
  [[nodiscard]] bool load(const char* fileName); // Load the values.
  [[nodiscard]] bool save(const char* fileName); // Save the values.
  void normalize(void);                          // Normalize the values.
  void scale(double scaleFactor);                // Scale the values.
  double train(double desiredOutput,double learningRate,double &output);
  double evalPrecise(void);                      // Get float eval.
  int eval(void);                                // Get (scaled) INT eval.

  private:

  // PRIVATE (CLASS) MEMEBER FUNCTION PROTOTYPES:
  [[nodiscard]] int getStage(void) noexcept;                            // Get stage of game we are on.
  [[nodiscard]] double activation(double value) noexcept; // Get bipolar-sigmoid act.
  [[nodiscard]] double gradient(double value) noexcept;   // Get bipolar-sigmoid grad.
  double evalAndLearn(double OffsetValue);       // Eval and/or update weights.
  double evalPawn(int Square);                   // Eval the pawn at square.
  double evalKnight(int Square);                 // Eval the knight at square.
  double evalBishop(int Square);                 // Eval the bishiop at square.
  double evalRook(int Square);                   // Eval the rook at square.
  double evalQueen(int Square);                  // Eval the queen at square.
  double evalKing(int Square);                   // Eval the king at square.
  double forepostBonus(int Square);              // Add forepost bonuse(s)...

  // Inline helper functions for weight access during training
  // NOTE: When offset is 0 (normal evaluation), these just return the weight value
  // without modification. Only during training (offset != 0) do they update weights.
  inline double addWeight(double& weight) { 
    if (offset != 0.0) weight += offset;
    return weight; 
  }
  inline double addWeightScaled(double& weight, double scaleFactor) {
    if (offset != 0.0) weight += offset * scaleFactor;
    return weight * scaleFactor;
  }
  inline double addWeightSingular(double* weight, int idx) { 
    if (offset != 0.0) weight[idx] += offset;
    return weight[idx]; 
  }
  inline double addWeightSingularScaled(double* weight, int idx, double scaleFactor) {
    if (offset != 0.0) weight[idx] += offset * scaleFactor;
    return weight[idx] * scaleFactor;
  }
  inline int flipIfNeeded(bool flipFlag, int square) {
    return flipFlag ? flipSquare(square) : square;
  }

}; // End EvaluationParameters class.

// #############################################################################
// #############################################################################
// #############################################################################

// GLOBAL FUNCTION PROTOTYPES:

[[nodiscard]] int basicMaterialEval(void) noexcept;    // This just returns a materialistic evaluation.
[[nodiscard]] bool materialExactlyEven(void) noexcept; // Returns true if both have same pieces...

