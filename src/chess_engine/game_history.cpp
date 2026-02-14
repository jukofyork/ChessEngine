// ****************************************************************************
// *                        GAME HISTORY UPDATE FUNCTIONS                     *
// ****************************************************************************

#include "chess_engine.h"
#include "globals.h"
#include <mutex>

// ==========================================================================
// Internal function to initialize lookup tables - called once via std::call_once
static void CreateLookupTables(void) {
  // First cearte the lookup tables from own program (to make hybrid)
  generateMoveTables();                // Create the move lookup tables.

  // Generate the exposed attack table.
  generateExposedAttackTable();        // MUST BE DONE AFTER MOVE TABLES.

  // Generate the g_posData tables.
  initPosData();

  // Init the hash codes.
  initHashCodes();
} // End CreateLookupTables.

void initAll(void)
{ // Sets the board to the initial game state (Starts new game).

  // Thread-safe one-time initialization of lookup tables.
  static std::once_flag lookupInitFlag;
  std::call_once(lookupInitFlag, CreateLookupTables);

  // Set up the pieces for each square:
  g_gameHistory[0].piece[0]=ROOK;        // Black Left Rook.
  g_gameHistory[0].piece[1]=KNIGHT;      // Black Left Knight.
  g_gameHistory[0].piece[2]=BISHOP;      // Black Left Bishop.
  g_gameHistory[0].piece[3]=QUEEN;       // Black Queen.
  g_gameHistory[0].piece[4]=KING;        // Black King.
  g_gameHistory[0].piece[5]=BISHOP;      // Black Right Bishop.
  g_gameHistory[0].piece[6]=KNIGHT;      // Black Right Knight.
  g_gameHistory[0].piece[7]=ROOK;        // Black Right Rook.
  for (int i=8;i<16;i++)
    g_gameHistory[0].piece[i]=PAWN;      // Black Pawns.
  for (int i=16;i<48;i++)
    g_gameHistory[0].piece[i]=NONE;      // Empty Squares.
  for (int i=48;i<56;i++)
    g_gameHistory[0].piece[i]=PAWN;      // White Pawns.
  g_gameHistory[0].piece[56]=ROOK;       // White Left Rook.
  g_gameHistory[0].piece[57]=KNIGHT;     // White Left Knight.
  g_gameHistory[0].piece[58]=BISHOP;     // White Left Bishop.
  g_gameHistory[0].piece[59]=QUEEN;      // White Queen.
  g_gameHistory[0].piece[60]=KING;       // White King.
  g_gameHistory[0].piece[61]=BISHOP;     // White Right Bishop.
  g_gameHistory[0].piece[62]=KNIGHT;     // White Right Knight.
  g_gameHistory[0].piece[63]=ROOK;       // White Right Rook.

  // Set up the Owner of each square.
  for (int i=0;i<16;i++)
    g_gameHistory[0].colour[i]=BLACK;    // Black's Pieces.
  for (int i=16;i<48;i++)
    g_gameHistory[0].colour[i]=NONE;     // Empty Squares.
  for (int i=48;i<64;i++)
    g_gameHistory[0].colour[i]=WHITE;    // White's Pieces.

  // Reset the other variables to a new game.
  g_gameHistory[0].castlePerm=15;        // All can castle.
  g_gameHistory[0].enPass=NO_EN_PASSANT; // En-Pasent is NOT allowed yet.
  g_gameHistory[0].fiftyCounter=0;       // Reset the first-move-rule counter.
  g_gameHistory[0].kingSquare[WHITE]=60; // Set to start position.
  g_gameHistory[0].kingSquare[BLACK]=4;  // Set to start position.

  // Set up the side to play to be WHITE.
  g_currentSide=WHITE;                   // side is the side to play next.

  // Start on move 0.
  g_moveNum=0;                           // Now on move 0.

  // Flag the fact that we are NOT in check at the start of the game.
  g_gameHistory[0].inCheck=false;

  // Can't be a draw on the first move.
  g_gameHistory[0].isDraw=false;

  // Set the pointer to the first state.
  g_currentState=&g_gameHistory[0];

  // Set up the pointer to the current board.
  g_currentColour=g_currentState->colour;
  g_currentPiece=g_currentState->piece;

  // Init the 64bit Hash Key for this (starting) state.
  // Update on the fly the rest of the time.
  g_gameHistory[0].key=currentKey();

} // End initAll.

// =============================================================================

bool makeMove(MoveStruct &moveToMake)
{ // This function makes a move. If the move is illegal, it undoes whatever
  // it did and returns false. Otherwise, it returns true.

  // This is -1 if no extra exposed square to check for putting oponent in
  // check. This is only when we castle (ie. to the rooks square) or when we
  // do the enpassent move (your old piece's square - as it could of blocked
  // a check to you).
  int extraExposedSquare=-1;

  // Save the game state on the g_gameHistory.
  // This is used for both rep check and for user take-back of move.
  // BUG: This was in the wrong place before and catles had rook moved
  //      priror to the state being saved!
  *(g_currentState+1) = *g_currentState;

  // This points to the current state (for speed) - NOTE: +1 now!.
  g_currentState++;

  // Set up the pointer to the current board.
  g_currentColour=g_currentState->colour;
  g_currentPiece=g_currentState->piece;

  // First test to see if a castle move is legal and move the rook (the king
  // is moved with the usual move code later).
  if (moveToMake.type&CASTLE) {
    if (moveToMake.target==62) {
      if (isAttacked(61,getOtherSide(g_currentSide)) || isAttacked(62,getOtherSide(g_currentSide))) {
        g_currentState--;
        g_currentColour=g_currentState->colour;
        g_currentPiece=g_currentState->piece;
        return false;
      }
      g_currentColour[61]=WHITE;
      g_currentPiece[61]=ROOK;
      g_currentColour[63]=NONE;
      g_currentPiece[63]=NONE;
      extraExposedSquare=61;
      g_currentState->key^=g_hashCode[WHITE][ROOK][63];
      g_currentState->key^=g_hashCode[WHITE][ROOK][61];
    }
    else if (moveToMake.target==58) {
      if (isAttacked(58,getOtherSide(g_currentSide)) || isAttacked(59,getOtherSide(g_currentSide))) {
        g_currentState--;
        g_currentColour=g_currentState->colour;
        g_currentPiece=g_currentState->piece;
        return false;
      }
      g_currentColour[59]=WHITE;
      g_currentPiece[59]=ROOK;
      g_currentColour[56]=NONE;
      g_currentPiece[56]=NONE;
      extraExposedSquare=59;
      g_currentState->key^=g_hashCode[WHITE][ROOK][56];
      g_currentState->key^=g_hashCode[WHITE][ROOK][59];
    }
    else if (moveToMake.target==6) {
      if (isAttacked(5,getOtherSide(g_currentSide)) || isAttacked(6,getOtherSide(g_currentSide))) {
        g_currentState--;
        g_currentColour=g_currentState->colour;
        g_currentPiece=g_currentState->piece;
        return false;
      }
      g_currentColour[5]=BLACK;
      g_currentPiece[5]=ROOK;
      g_currentColour[7]=NONE;
      g_currentPiece[7]=NONE;
      extraExposedSquare=5;
      g_currentState->key^=g_hashCode[BLACK][ROOK][7];
      g_currentState->key^=g_hashCode[BLACK][ROOK][5];
    }
    else if (moveToMake.target==2) {
      if (isAttacked(2,getOtherSide(g_currentSide)) || isAttacked(3,getOtherSide(g_currentSide))) {
        g_currentState--;
        g_currentColour=g_currentState->colour;
        g_currentPiece=g_currentState->piece;
        return false;
      }
      g_currentColour[3]=BLACK;
      g_currentPiece[3]=ROOK;
      g_currentColour[0]=NONE;
      g_currentPiece[0]=NONE;
      extraExposedSquare=3;
      g_currentState->key^=g_hashCode[BLACK][ROOK][0];
      g_currentState->key^=g_hashCode[BLACK][ROOK][3];
    }

  }

  // Alter the King's Square if needed.
  if (moveToMake.source==g_currentState->kingSquare[g_currentSide])
    g_currentState->kingSquare[g_currentSide]=moveToMake.target;

  // Update the castleing permisions. (Note: Targets must be used also!)
  if (moveToMake.source==60) {
    g_currentState->castlePerm&=~(WHITE_KING_SIDE|WHITE_QUEEN_SIDE);
  }
  else if (moveToMake.source==4) {
    g_currentState->castlePerm&=~(BLACK_KING_SIDE|BLACK_QUEEN_SIDE);
  }
  else if (moveToMake.source==0 || moveToMake.target==0) {
    g_currentState->castlePerm&=~BLACK_QUEEN_SIDE;
  }
  else if (moveToMake.source==7 || moveToMake.target==7) {
    g_currentState->castlePerm&=~BLACK_KING_SIDE;
  }
  else if (moveToMake.source==56 || moveToMake.target==56) {
    g_currentState->castlePerm&=~WHITE_QUEEN_SIDE;
  }
  else if (moveToMake.source==63 || moveToMake.target==63) {
    g_currentState->castlePerm&=~WHITE_KING_SIDE;
  }

  // Update en passant info.
  if (moveToMake.type&TWO_SQUARES) {
    if (g_currentSide==WHITE) {
      g_currentState->enPass=moveToMake.target+8;
    }
    else {
      g_currentState->enPass=moveToMake.target-8;
    }
  }
  else {
    g_currentState->enPass=NO_EN_PASSANT;
  }

  // Update the fifty-move-draw counter.
  if (moveToMake.type&(PAWN_MOVE|CAPTURE))
    g_currentState->fiftyCounter=0;
  else
    g_currentState->fiftyCounter++;

  // Alter the Peice or pawn material scores, if a capture.
  // MEGA FAT BUG THAT REALY PISSED ME OFF:
  // Treat en-passent as a special capture... Spent hours on this one :(
  // The cause of the phantom queen/knight sacrifice when playing gnu chess!
  if (moveToMake.type&CAPTURE) {
    if (moveToMake.type&EN_PASSANT) {
      if (g_currentSide==WHITE) {
        g_currentColour[moveToMake.target+8]=NONE;
        g_currentPiece[moveToMake.target+8]=NONE;
        extraExposedSquare=moveToMake.target+8;
        g_currentState->key^=g_hashCode[getOtherSide(g_currentSide)][PAWN][moveToMake.target+8];
      }
      else {
        g_currentColour[moveToMake.target-8]=NONE;
        g_currentPiece[moveToMake.target-8]=NONE;
        extraExposedSquare=moveToMake.target-8;
        g_currentState->key^=g_hashCode[getOtherSide(g_currentSide)][PAWN][moveToMake.target-8];
      }
    }
    else {
      g_currentState->key^=g_hashCode[getOtherSide(g_currentSide)][g_currentPiece[moveToMake.target]]
                                 [moveToMake.target];
    }
  }

  // Move the piece.
  g_currentColour[moveToMake.target]=g_currentSide;
  if (moveToMake.type&PROMOTION) {
    g_currentState->key^=g_hashCode[g_currentSide][moveToMake.promote]
                               [moveToMake.target];
    g_currentPiece[moveToMake.target]=moveToMake.promote;

  }
  else {
    g_currentPiece[moveToMake.target]=g_currentPiece[moveToMake.source];
    g_currentState->key^=g_hashCode[g_currentSide][g_currentPiece[moveToMake.target]]
                               [moveToMake.target];
  }
  g_currentState->key^=g_hashCode[g_currentSide][g_currentPiece[moveToMake.source]]  
                             [moveToMake.source];  
  g_currentColour[moveToMake.source]=NONE;
  g_currentPiece[moveToMake.source]=NONE;

  // Switch sides and test for legality (if we can capture the other guy's 
  // king, it's an illegal position and we need to take the move back).
  // start_againg6: Try to do less work with extra lookups (see other text)!
  //                Don't bother if a castling move as checked for legality
  //                already.
  g_currentSide=getOtherSide(g_currentSide);             // Swap sides.
  g_moveNum++;                         // One more move done.
  if (!(moveToMake.type&CASTLE)) {
    if (g_gameHistory[g_moveNum-1].inCheck 
        || moveToMake.target==g_currentState->kingSquare[getOtherSide(g_currentSide)]) {
      if (isAttacked(g_currentState->kingSquare[getOtherSide(g_currentSide)],g_currentSide)) {
        takeMoveBack();
        return false;
      }
    }
    else {

      // One more cheap attack (as before this required a full isAttacked() call!).

      // Only test the exposed squares (cheaper!).
      if (testExposure(g_currentState->kingSquare[getOtherSide(g_currentSide)],moveToMake.source,
                       g_currentSide)) {
        takeMoveBack();
        return false;
      }
    }
  }

  // See if the state is a draw due to material, repetition or fifty move rule.
  // NOTE: Don't bother checking for check if so, as it doen't matter!
  if (g_currentState->fiftyCounter>=50 || testNotEnoughMaterial()
      || testRepetition()) {
    g_currentState->isDraw=true;             // Is a draw.
    g_currentState->inCheck=false;           // NOT LOOKED AT ANYWAY!
  }
  // Else, see if we put the opponent in check.
  else {

    g_currentState->isDraw=false;            // Is not a draw.

    // If castling, just check the rook for exposed check (cheap!).
    if (moveToMake.type&CASTLE
        && testExposure(g_currentState->kingSquare[g_currentSide],
                        extraExposedSquare,getOtherSide(g_currentSide))==true) {
      g_currentState->inCheck=true;
    }
    else {

      // Enpassant move exposes an extra square to be checked (cheap!).
      if (moveToMake.type&EN_PASSANT
          && testExposure(g_currentState->kingSquare[g_currentSide],
                        extraExposedSquare,getOtherSide(g_currentSide))==true) {
        g_currentState->inCheck=true;
      }

      // Check the exposed square and the piece that moved to see if it's check.
      else if (singleAttack(g_currentState->kingSquare[g_currentSide],
                            moveToMake.target)==true
               || testExposure(g_currentState->kingSquare[g_currentSide],
                               moveToMake.source,getOtherSide(g_currentSide))==true) {
        g_currentState->inCheck=true;
      }

      // Not in check then!
      else {
        g_currentState->inCheck=false;
      }

    }

  }

  // Save the move that we made into the g_movesMade list.
  g_movesMade[g_moveNum-1]=moveToMake;

  // Return true as made a legal move.
  return true;

} // End makeMove.

// ==========================================================================

void takeMoveBack(void)
{ // This function takes back a single move by restoring a previously saved
  // state.
  // NOTE: No need to restore state now.

  // Swap and decrement flags/counters.
  g_currentSide=getOtherSide(g_currentSide);                                // Swap sides.
  g_moveNum--;                                            // One less move.

  // This points to the current state (for speed) - NOTE: -1 now!.
  g_currentState--;

  // Set up the pointer to the current board.
  g_currentColour=g_currentState->colour;
  g_currentPiece=g_currentState->piece;

} // End takeMoveBack.

// ==========================================================================

