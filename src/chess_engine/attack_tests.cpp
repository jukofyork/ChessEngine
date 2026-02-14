// *****************************************************************************
// *                           ATTACK TESTING FUNCTIONS                        *
// *****************************************************************************

#include "chess_engine.h"
#include "globals.h"

// ============================================================================

bool isAttacked(int square,int sideAttacking)
{ // This function returns true if side 1-sideAttacking is in Check now.
  // It basically tries all moves *FROM* the square and sees if there is one
  // of the oponent's peices on the square (Must be the same as the move we
  // are trieing).
  // Has been ordered (most likely to be true first) to try to make quicker.
  // VERY OPTOMIZED - NEARLY AS FAST AS (Old N=64) InCheck() function!!!

  int* MovePtr;                             // To iterate through Move Tables.
  int tempSquare;                           // Holds square read from lookup.
  int dirIndex;                             // To iterate through move dirs.

  // Do both in the same iterator for speed.
  for (dirIndex=0;dirIndex<4;dirIndex++) {

    // Test For Oponment's Rooks and Queens (Straight moves).
    MovePtr=g_straightMoves[square][dirIndex];
    while ((tempSquare=(*(MovePtr++)))!=END_OF_LOOKUP){
      if (g_currentColour[tempSquare]==sideAttacking
          && (g_currentPiece[tempSquare]==ROOK
              || g_currentPiece[tempSquare]==QUEEN)) {
        return true;                         // square under attack.
      }
      if (g_currentColour[tempSquare]!=NONE)
        break;
    }

    // Test For Oponment's Bishops and Queens (Diagonal moves ).
    MovePtr=g_diagonalMoves[square][dirIndex];
    while ((tempSquare=(*(MovePtr++)))!=END_OF_LOOKUP){
      if (g_currentColour[tempSquare]==sideAttacking
          && (g_currentPiece[tempSquare]==BISHOP
              || g_currentPiece[tempSquare]==QUEEN)) {
        return true;                         // square under attack.
      }
      if (g_currentColour[tempSquare]!=NONE)
        break;
    }
  }

  // Test For Oponment's Knights.
  MovePtr=g_knightMoves[square];
  while ((tempSquare=(*(MovePtr++)))!=END_OF_LOOKUP) {
    if (g_currentPiece[tempSquare]==KNIGHT
        && g_currentColour[tempSquare]==sideAttacking) {
      return true;                         // square under attack.
    }
  }

  // Test for Oponment's Pawn Captures.
  // This now is corrected and works with the new board orientation.
  if (sideAttacking==WHITE && square<=47) { //(BUG: 47 not 40!)
    if (getFile(square)!=0 && g_currentPiece[square+7]==PAWN
        && g_currentColour[square+7]==WHITE) {
      return true;                         // square under attack.
    }
    if (getFile(square)!=7 && g_currentPiece[square+9]==PAWN
        && g_currentColour[square+9]==WHITE) {
      return true;                         // square under attack.
    }
  }
  else if (sideAttacking==BLACK && square>=16) {
    if (getFile(square)!=0 && g_currentPiece[square-9]==PAWN
        && g_currentColour[square-9]==BLACK) {
      return true;                         // square under attack.
    }
    if (getFile(square)!=7 && g_currentPiece[square-7]==PAWN
        && g_currentColour[square-7]==BLACK) {
      return true;                         // square under attack.
    }
  }

  // Test For Oponment's King.
  MovePtr=g_kingMoves[square];
  while ((tempSquare=(*(MovePtr++)))!=END_OF_LOOKUP) {
    if (g_currentPiece[tempSquare]==KING
        && g_currentColour[tempSquare]==sideAttacking) {
      return true;                         // square under attack.
    }
  }

  // square not under attack.
  return false;

} // End isAttacked.

// =============================================================================

bool singleAttack(int targetSquare,int newEnemySquare)
{ // Works like Attack(), but only tests if the enemy piece can attack the
  // designated square, rather than any enemy piece like in Attack().
  // NOTE: No need to check for enemy king checking us as this should of been
  //       filtered out as an illegal move in MakeMove() before calling this
  //       function.

  int* MovePtr;                            // To iterate through Move Tables.
  int tempSquare;                          // Holds square read from lookup.
  int lookupIndex;                         // What direction to check in lookup.

  // Get the lookup index.
  lookupIndex=g_exposedAttackTable[targetSquare][newEnemySquare];

  // Enemy Rooks or Queen? (Straight move).
  // NOTE: Castling move must use a proper Attack()!
  if ((g_currentPiece[newEnemySquare]==ROOK
       || g_currentPiece[newEnemySquare]==QUEEN)
      && lookupIndex>=0 && lookupIndex<=3) {

    // Test all the squares inbetween to see if we are unobstructed.
    MovePtr=g_straightMoves[targetSquare][lookupIndex];
    tempSquare=(*MovePtr);
    while (tempSquare!=newEnemySquare) {
      if (g_currentColour[tempSquare]!=NONE)
        return false;                         // Attack blocked.
      tempSquare=(*(++MovePtr));
    }
    return true;

  }

  // Enemy Bishop or Queen? (Diagonal move).
  else if ((g_currentPiece[newEnemySquare]==BISHOP
            || g_currentPiece[newEnemySquare]==QUEEN)
           && lookupIndex>=4) {

    // Test all the squares inbetween to see if we are unobstructed.
    MovePtr=g_diagonalMoves[targetSquare][lookupIndex-4];
    tempSquare=(*MovePtr);
    while (tempSquare!=newEnemySquare) {
      if (g_currentColour[tempSquare]!=NONE)
        return false;                         // Attack blocked.
      tempSquare=(*(++MovePtr));
    }
    return true;

  }

  // Enemy Knight? (Knight move).
  else if (g_currentPiece[newEnemySquare]==KNIGHT) {

    // dirIndexust use the Knight attack lookup table.
    return g_knightAttackTable[targetSquare][newEnemySquare];

  }

  // Enemy Pawn? (Pawn move).
  // BUG: Enpassent move causes an extra diagonal to need checking and hence
  //      just use attack!.
  else if (g_currentPiece[newEnemySquare]==PAWN) {
    if (g_currentColour[newEnemySquare]==WHITE && targetSquare<=39
        && (getRank(newEnemySquare)-1)==getRank(targetSquare)
        && labs(getFile(targetSquare)-getFile(newEnemySquare))==1) {
      return true;                         // square under attack.
    }
    else if (g_currentColour[newEnemySquare]==BLACK && targetSquare>=24
             && (getRank(newEnemySquare)+1)==getRank(targetSquare)
             && labs(getFile(targetSquare)-getFile(newEnemySquare))==1) {
      return true;                         // square under attack.
    }
  }

  // square not under attack.
  return false;

} // End singleAttack.

// =============================================================================

bool testExposure(int targetSquare,int evacuatedSquare,int sideAttacking)
{ // This function tests if the targetSquare has been exposed to an attack
  // because of a piece (NOT KING!) moveing out of the line of your king.

  int* MovePtr;                            // To iterate through Move Tables.
  int tempSquare;                          // Holds square read from lookup.
  int lookupIndex;                         // What direction to check in lookup.

  // Get the lookup index.
  lookupIndex=g_exposedAttackTable[targetSquare][evacuatedSquare];

  // See if it will make nay difference first.
  if (lookupIndex==-1)
    return false;

  // Else see if it's straight or diagonal.
  else if (lookupIndex<4) {

    // Test For Oponment's Rooks and Queens (Straight moves).
    MovePtr=g_straightMoves[targetSquare][lookupIndex];
    while ((tempSquare=(*(MovePtr++)))!=END_OF_LOOKUP){
      if (g_currentColour[tempSquare]==sideAttacking
          && (g_currentPiece[tempSquare]==ROOK
              || g_currentPiece[tempSquare]==QUEEN)) {
        return true;                         // square under attack.
      }
      if (g_currentColour[tempSquare]!=NONE)
        break;
    }
  }

  // It must be a diagonal move then.
  else {

    // Test For Oponment's Bishops and Queens (Diagonal moves ).
    MovePtr=g_diagonalMoves[targetSquare][lookupIndex-4];
    while ((tempSquare=(*(MovePtr++)))!=END_OF_LOOKUP){
      if (g_currentColour[tempSquare]==sideAttacking
          && (g_currentPiece[tempSquare]==BISHOP
              || g_currentPiece[tempSquare]==QUEEN)) {
        return true;                         // square under attack.
      }
      if (g_currentColour[tempSquare]!=NONE)
        break;
    }

  }

  // No exposed attack found.
  return false;

} // End testExposure.

// ============================================================================


