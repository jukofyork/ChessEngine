// ****************************************************************************
// *                         DRAW TESTING FUNCTIONS                           *
// ****************************************************************************

#include "chess_engine.h"
#include "globals.h"

#include <array>

// ============================================================================

bool testRepetition(void)
{ // Returns true if we have repeaded the same position 3 times.
  // start_again2: Have now speeded this up loads by doing (in order of speed!):
  //               1: Using FiftyCounter being greater than 8 (not movenum!).
  //               2: Starting to look for duplicate states *4* states back from
  //                  current, as state -1/-2/-3 can't be the same as current.
  //               3: Realizing that any before a FiftyCounter=0 can't be the
  //                  same as the state is inacessable after a pawn move or
  //                  capture (hardly any speed increase from this?).
  // start_again5: Have now realized that repeats (ie: Cycles) may only happen
  //               every 4 moves, this is OK because we may not forfeit our
  //               move as in games such as Othello, in general we need only
  //               look back every 2*NumPlayers moves to check for a repetition.
  //               This makes very little difference as already very fast here!
  // start_again8: Now uses the hash key only to search (is this safe?).
  // clean_up14: Now uses proper hash key (with castle permisions etc).

  int numSame;                 // If gets to 3, then draw due to repetition.

  // These are used to make the true key, using Side/Castle permision etc.
  HashKey currentKey;
  HashKey oldKey;

  // Get the current key.
  currentKey=g_currentState->key;

  // Add the castle permisions and the en-passent info etc.
  currentKey^=g_castleHashCode[g_currentState->castlePerm];
  if (g_currentState->enPass!=NO_EN_PASSANT)
    currentKey^=g_enPassantHashCode[g_currentState->enPass];

  // Check the position history, to see if we have had the same position
  // three time. (Only check >=8 as these are possible onwards).
  if (g_currentState->fiftyCounter>=8) {

    // None the same yet.
    numSame=0;

    // Test with all previously stored moves (with the same player to move).
    for (int i=g_moveNum-4;(g_currentState->fiftyCounter-(g_moveNum-i))>=0;i-=2) {

      // Get the old (stored) key.
      oldKey=g_gameHistory[i].key;

      // Add the castle permisions and the en-passent info etc.
      oldKey^=g_castleHashCode[g_gameHistory[i].castlePerm];
      if (g_gameHistory[i].enPass!=NO_EN_PASSANT)
        oldKey^=g_enPassantHashCode[g_gameHistory[i].enPass];

      // See if the same (ie: Same position!).
      if (currentKey==oldKey) {
        numSame++;                     // One more same.

        // Check if We have two the same, if so 3 identilcle have been seen.
        if (numSame==2)
          return true;    // Draw, due to repitition.
      }

    }

  }

  // Not so, so return false.
  return false;

} // End testRepetition.

// =========================================================================

bool testSingleRepetition(int minMoveNum)
{ // Returns true if we have repeaded the same position one before.
  // NOTE: Not for draws, but for spotting hash cycles when printing the PV
  //       from the hash. 
  //       Set min movenum to the move before we started searching.

  // Check the position history, to see if we have had the same position
  // before. (Only check >=4 as these are possible onwards).
  if (g_currentState->fiftyCounter>=4) {

    // Test with all previously stored moves.
    for (int i=g_moveNum-4;(g_currentState->fiftyCounter-(g_moveNum-i))>=0
                         && i>minMoveNum;i-=4) {

      // If state not made by the null move, compare it.
      //if (g_gameHistory[i].NullMove==false) {
        if (g_gameHistory[i].key==g_currentState->key) {
            return true;          // Same found.
        }
      //}

    }

  }

  // Not so, so return false.
  return false;

} // End testSingleRepetition.

// ==========================================================================

bool testNotEnoughMaterial(void)
{ // This function detects a draw due to not having enough material to win.
  // - King vs King.
  // - King vs King and Knight.
  // - King vs King and Bishop.
  // - King vs King and two Knights.
  // - King and Knight vs King and Knight.
  // - King and Knight vs King and Bishop.
  // - King and Knight vs King and two Knights.
  // - King and bishop vs King and Knight.
  // - King and bishop vs King and Bishop.
  // - King and bishop vs King and two Knights.
  // - King and two Knights vs King and Knight.
  // - King and two Knights vs King and Bishop.
  // - King and two Knights vs King and two Knights.

  std::array<int, 2> numBishops = {0, 0};  // For each side.
  std::array<int, 2> numKnights = {0, 0};  // For each side.

  // Scan through the pieces on the board, testing if any are pawns or major
  // pieces - in which case stop.
  for (int i=0;i<BOARD_SQUARES;i++) {
    if (g_currentPiece[i]==PAWN
        || g_currentPiece[i]==ROOK
        || g_currentPiece[i]==QUEEN) {
      return false;                     // Not an Material Draw.
    }

    // count up each sides minor peices.
    if (g_currentPiece[i]==BISHOP)
      numBishops[g_currentColour[i]]++;
    else if (g_currentPiece[i]==KNIGHT)
      numKnights[g_currentColour[i]]++;
  }

  // Test to see if any of the draw situations have been reached.
  // First see if White Can't Mate with his material, then test blacks
  // material.
  if (((numBishops[WHITE]==0 && numKnights[WHITE]==0)
       || (numBishops[WHITE]==1 && numKnights[WHITE]==0)
       || (numBishops[WHITE]==0 && numKnights[WHITE]==1)
       || (numBishops[WHITE]==0 && numKnights[WHITE]==2))
      && ((numBishops[BLACK]==0 && numKnights[BLACK]==0)
          || (numBishops[BLACK]==1 && numKnights[BLACK]==0)
          || (numBishops[BLACK]==0 && numKnights[BLACK]==1)
          || (numBishops[BLACK]==0 && numKnights[BLACK]==2))) {
    return true;                       // Can't possibly mate now.
  }

  // One side must still be able to possibly mate.
  return false;

} // End testNotEnoughMaterial.

// ============================================================================

