// ****************************************************************************
// *                        MOVE GENERATION FUNCTIONS                         *
// ****************************************************************************

#include "chess_engine.h"
#include "globals.h"

// ============================================================================

void genLegalMoves(MoveList &moves)
{ // This function returns a list of all legal moves from to position.

  // This is the list of psuedo-legal moves.
  MoveList psuedoLegalMoves;

  // Generate all psuedo-legal moves.
  genMoves(psuedoLegalMoves);

  // No moves in output yet.
  moves.numMoves=0;

  // Attempt to make each move, and copy to output if legal.
  for (int i=0;i<psuedoLegalMoves.numMoves;i++) {

    // See if the move is realy legal.
    if (makeMove(psuedoLegalMoves.moves[i])) {

      // Take the move back.
      takeMoveBack();

      // Copy it to the output.
      moves.moves[moves.numMoves++] = psuedoLegalMoves.moves[i];

    }

  }

} // End GenLegalMoves. 

// ============================================================================

void genMoves(MoveList &moves)
{ // This function generates (pseudo-legal) moves for the current position.
  // It scans the board to find friendly pieces and then determines what 
  // squares they attack. When it finds a piece/square combination, it calls 
  // genPush to put the move on the "move stack."
  // NOTE: All movement lookup tables are generated here *ONCE ONLY* and then
  //       saved in statics for later use (saving on all the X/Y Bounds stuff).

  SquareData* squareData;

  // So far, we have no moves for the current ply.
  moves.numMoves=0;
  for (int i=0;i<BOARD_SQUARES;i++) {
    if (g_currentColour[i]==g_currentSide) {
      if (g_currentPiece[i]==PAWN) {
        if (g_currentSide==WHITE) {
          if (getFile(i)!=0 && g_currentColour[i-9]==BLACK)
            genPush(moves,i,i-9,PAWN_MOVE|CAPTURE);
          if (getFile(i)!=7 && g_currentColour[i-7]==BLACK)
            genPush(moves,i,i-7,PAWN_MOVE|CAPTURE);
          if (g_currentColour[i-8]==NONE) {
            genPush(moves,i,i-8,PAWN_MOVE);
            if (i>=48 && g_currentColour[i-16]==NONE)
              genPush(moves,i,i-16,PAWN_MOVE|TWO_SQUARES);
          }
        }
        else {
          if (getFile(i)!=0 && g_currentColour[i+7]==WHITE)
            genPush(moves,i,i+7,PAWN_MOVE|CAPTURE);
          if (getFile(i)!=7 && g_currentColour[i+9]==WHITE)
            genPush(moves,i,i+9,PAWN_MOVE|CAPTURE);
          if (g_currentColour[i+8]==NONE) {
            genPush(moves,i,i+8,PAWN_MOVE);
            if (i<=15 && g_currentColour[i+16]==NONE)
              genPush(moves,i,i+16,PAWN_MOVE|TWO_SQUARES);
          }
        }

      }
      
      // Do other pieces.
      else {
        squareData=g_posData[g_currentPiece[i]][i];
        do {
          if (g_currentColour[squareData->testSquare]==NONE) { 
            genPush(moves,i,squareData->testSquare,NORMAL_MOVE);
            squareData++;
          }
          else {
            if (g_currentColour[squareData->testSquare]==getOtherSide(g_currentSide))
              genPush(moves,i,squareData->testSquare,CAPTURE);
            squareData=squareData->skip;
          }
        } while (squareData->testSquare!=END_OF_LOOKUP);
      }

    }

  } // End for all squares.

  // Generate castle moves.
  // start_again6: All but the check for Attack here to save time. Attack check 
  //               in MakeMove() in case this move is prunned and not looked at.
  if (!g_currentState->inCheck) {
    if (g_currentSide==WHITE) {
      if (g_currentState->castlePerm&WHITE_KING_SIDE
          && g_currentColour[61]==NONE
          && g_currentColour[62]==NONE) {
        genPush(moves,60,62,CASTLE);
      }
      if (g_currentState->castlePerm&WHITE_QUEEN_SIDE
          && g_currentColour[57]==NONE
          && g_currentColour[58]==NONE
          && g_currentColour[59]==NONE) {
        genPush(moves,60,58,CASTLE);
      }
    }
    else {
      if (g_currentState->castlePerm&BLACK_KING_SIDE
          && g_currentColour[5]==NONE
          && g_currentColour[6]==NONE) {
        genPush(moves,4,6,CASTLE);
      }
      if (g_currentState->castlePerm&BLACK_QUEEN_SIDE
          && g_currentColour[1]==NONE
          && g_currentColour[2]==NONE
          && g_currentColour[3]==NONE) {
        genPush(moves,4,2,CASTLE);
      }
    }
  }

  // Generate en passant moves.
  if (g_currentState->enPass!=NO_EN_PASSANT) {
    if (g_currentSide==WHITE) {
      if (getFile(g_currentState->enPass)!=0
          && g_currentColour[g_currentState->enPass+7]==WHITE 
          && g_currentPiece[g_currentState->enPass+7]==PAWN) {
        genPush(moves,g_currentState->enPass+7,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
      if (getFile(g_currentState->enPass)!=7
          && g_currentColour[g_currentState->enPass+9]==WHITE 
          && g_currentPiece[g_currentState->enPass+9]==PAWN) {
        genPush(moves,g_currentState->enPass+9,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
    }
    else {
      if (getFile(g_currentState->enPass)!=0
          && g_currentColour[g_currentState->enPass-9]==BLACK 
          && g_currentPiece[g_currentState->enPass-9]==PAWN) {
        genPush(moves,g_currentState->enPass-9,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
      if (getFile(g_currentState->enPass)!=7
          && g_currentColour[g_currentState->enPass-7]==BLACK 
          && g_currentPiece[g_currentState->enPass-7]==PAWN) {
        genPush(moves,g_currentState->enPass-7,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
    }
  }

} // End genMoves.

// ==========================================================================

void genCaptures(MoveList &moves)
{ // This function generates (pseudo-legal) capture moves for the current 
  // position for use in Q. search.

  SquareData* squareData;

  // So far, we have no moves for the current ply.
  moves.numMoves=0;
  for (int i=0;i<BOARD_SQUARES;i++) {
    if (g_currentColour[i]==g_currentSide) {
      if (g_currentPiece[i]==PAWN) {
        if (g_currentSide==WHITE) {
          if (getFile(i)!=0 && g_currentColour[i-9]==BLACK)
            genPush(moves,i,i-9,PAWN_MOVE|CAPTURE);
          if (getFile(i)!=7 && g_currentColour[i-7]==BLACK)
            genPush(moves,i,i-7,PAWN_MOVE|CAPTURE);
          if (i<=15 && g_currentColour[i-8]==NONE)
            genPush(moves,i,i-8,PAWN_MOVE);
        }
        else {
          if (getFile(i)!=0 && g_currentColour[i+7]==WHITE)
            genPush(moves,i,i+7,PAWN_MOVE|CAPTURE);
          if (getFile(i)!=7 && g_currentColour[i+9]==WHITE)
            genPush(moves,i,i+9,PAWN_MOVE|CAPTURE);
          if (i>=48 && g_currentColour[i+8]==NONE)
            genPush(moves,i,i+8,PAWN_MOVE);
        }

      }

      // Do other pieces.
      else {
        squareData=g_posData[g_currentPiece[i]][i];
        do {
          if (g_currentColour[squareData->testSquare]==NONE) {
            squareData++;
          }
          else {
            if (g_currentColour[squareData->testSquare]==getOtherSide(g_currentSide))
              genPush(moves,i,squareData->testSquare,CAPTURE);
            squareData=squareData->skip;
          }
        } while (squareData->testSquare!=END_OF_LOOKUP);
      }

    }

  } // End for all squares.

  // Generate en passant moves.
  if (g_currentState->enPass!=NO_EN_PASSANT) {
    if (g_currentSide==WHITE) {
      if (getFile(g_currentState->enPass)!=0
          && g_currentColour[g_currentState->enPass+7]==WHITE
          && g_currentPiece[g_currentState->enPass+7]==PAWN) {
        genPush(moves,g_currentState->enPass+7,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
      if (getFile(g_currentState->enPass)!=7
          && g_currentColour[g_currentState->enPass+9]==WHITE
          && g_currentPiece[g_currentState->enPass+9]==PAWN) {
        genPush(moves,g_currentState->enPass+9,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
    }
    else {
      if (getFile(g_currentState->enPass)!=0
          && g_currentColour[g_currentState->enPass-9]==BLACK
          && g_currentPiece[g_currentState->enPass-9]==PAWN) {
        genPush(moves,g_currentState->enPass-9,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
      if (getFile(g_currentState->enPass)!=7
          && g_currentColour[g_currentState->enPass-7]==BLACK
          && g_currentPiece[g_currentState->enPass-7]==PAWN) {
        genPush(moves,g_currentState->enPass-7,
                g_currentState->enPass,PAWN_MOVE|EN_PASSANT|CAPTURE);
      }
    }
  }

} // End genCaptures.

// ==========================================================================

void genPush(MoveList &moves,int Source,int Target,int Type)
{ // This function adds a move to the MoveList given.
  // If it is a Pawn Promotion, it adds 4 moves.

  // For pawn promotions, add all four of the possible promotion moves.
  if (Type&PAWN_MOVE 
      && ((g_currentSide==WHITE && Target<=7)
          || (g_currentSide==BLACK && Target>=56))) {

    // Add the four different types (ie: target peices) of promotion.
    for (int i=KNIGHT;i<=QUEEN;i++) {
      moves.moves[moves.numMoves].source=Source;
      moves.moves[moves.numMoves].target=Target;
      moves.moves[moves.numMoves].promote=i;
      moves.moves[moves.numMoves].type=Type|PROMOTION;
      moves.numMoves++;
    }

    // Return as all promotion moves added now.
    return;

  }

  // Add the move.
  moves.moves[moves.numMoves].source=Source;
  moves.moves[moves.numMoves].target=Target;
  moves.moves[moves.numMoves].promote=0;
  moves.moves[moves.numMoves].type=Type;
  moves.numMoves++;

} // End genPush.

// ============================================================================

