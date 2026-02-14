// **************************************************************************
// *                          MOVE ORDERING FUNCTIONS                       *
// **************************************************************************

#include "search_engine.h"

// ==========================================================================

void scoreMoves(SearchData &searchData,int currentPly,MoveList &moves,
                int moveScores[MOVELIST_ARRAY_SIZE],bool nullMove)
{ // This gives a score to each move in the move list.

  // Rank catures and promotions the highest, then try a killer move if one
  // exists, else use move history value for a normal move.
  // 1. Hash move.
  // 2. Capture of last moved piece.
  // 3. Other captures (MVV-LVA).
  // 4. Killer moves (old/new/1st/2nd).
  // 5. King moves get as score of -1, if not a castle move.
  // 6. Move History.

  // 2003_v5: Save a local copy to try to speed the code up here.
  static int8_t source,target;
  static uint8_t type;

  // For each move.
  for (int i=0;i<moves.numMoves;i++) {

    // 1. Hash table move.
    if (moves.moves[i].source == searchData.hashMoves[currentPly].source
        && moves.moves[i].target == searchData.hashMoves[currentPly].target
        && moves.moves[i].type == searchData.hashMoves[currentPly].type
        && moves.moves[i].promote == searchData.hashMoves[currentPly].promote) {
      moveScores[i]=PV_SORT_SCORE;
      continue;
    }

    // 2003_v5: Save a local copy to try to speed the code up here.
    source=moves.moves[i].source;
    target=moves.moves[i].target;
    type=moves.moves[i].type;

    // 2. If it's a promotion, rank it high!
    if (type&PROMOTION) {
      moveScores[i]=PROMOTION_SORT_SCORE+(moves.moves[i].promote*10);

      // Add an even higher score if we capture on the promotion!
      if (type&CAPTURE)
        moveScores[i]+=g_currentPiece[target]*10;
    }

    // 3. Is it a capture, capture the last piece moved first.
    else if (type&CAPTURE) {
      moveScores[i]=CAPTURE_SORT_SCORE+((g_currentPiece[target]*10)
                                        -g_currentPiece[source]);

      // If we are capturing the last piece moved by opponent, make it higher.
      if (g_moveNum>1 && nullMove==false
          && (g_gameHistory[g_moveNum-1].piece[target]
              !=g_gameHistory[g_moveNum-2].piece[target]
              || g_gameHistory[g_moveNum-1].colour[target]
                 !=g_gameHistory[g_moveNum-2].colour[target])) {
        moveScores[i]++;
      }
    }

    // 4. Killer moves.
    else if (source==searchData.killerMovesOld[currentPly].source
             && target==searchData.killerMovesOld[currentPly].target) {
      moveScores[i]=KILLER_SORT_SCORE+(searchData.moveHistory[source][target]<<3);
    }
    else if (source==searchData.killerMovesNew[currentPly].source
             && target==searchData.killerMovesNew[currentPly].target) {
      moveScores[i]=KILLER_SORT_SCORE+(searchData.moveHistory[source][target]<<3);
    }
    else if (currentPly>1
             && source==searchData.killerMovesOld[currentPly-2].source
             && target==searchData.killerMovesOld[currentPly-2].target) {
      moveScores[i]=(KILLER_SORT_SCORE/2)+(searchData.moveHistory[source][target]<<3);
    }
    else if (currentPly>1
             && source==searchData.killerMovesNew[currentPly-2].source
             && target==searchData.killerMovesNew[currentPly-2].target) {
      moveScores[i]=(KILLER_SORT_SCORE/2)+(searchData.moveHistory[source][target]<<3);
    }

    // 5. Castling is generally good.
    else if (type&CASTLE) {
      moveScores[i]=(searchData.moveHistory[source][target]<<3)|7;
    }

    // 5. King moves score lower, as are more expensive to test if legal.
    else if (g_currentPiece[source]==KING) {
      moveScores[i]=(searchData.moveHistory[source][target]<<3);
    }

    // 6. Move history score + add to it the piece moveing (not KINGS!).
    else {
      moveScores[i]=(searchData.moveHistory[source][target]<<3)
                    +(g_currentPiece[source]+1);
    }

  } // End for each move.

} // End scoreMoves.

// ==========================================================================

void sortMoves(MoveList &moves,int moveScores[MOVELIST_ARRAY_SIZE],int source)
{ // This function searches the current ply's move list from 'Source' to the 
  // end to find the move with the highest score. Then it swaps that move 
  // and the 'from' move so the move with the highest score gets searched 
  // next, and hopefully produces a cutoff.
  // NOTE: For this to work correctly, it is expected that CAPTURES and 
  //       PROMOTIONS have a higher MoveScore than other moves!
  //       This is so we can use the same Sort()/GenMoves() for Quiesce search.

  int bestScore;                // Best score.
  int bestIndex = source;       // Best Index - init to safe value.
  MoveStruct tempMove;
  int tempScore;

  // Find the best.
  bestScore=-0x7fffffff;      // Init to impossible.
  for (int i=source;i<moves.numMoves;i++) {
    if (moveScores[i]>bestScore) {
      bestScore=moveScores[i];
      bestIndex=i;
    }
  }

  // Swap the move.
  tempMove=moves.moves[source];
  moves.moves[source]=moves.moves[bestIndex];
  moves.moves[bestIndex]=tempMove;

  // Swap the score (NOT NEEDED?).
  tempScore=moveScores[source];
  moveScores[source]=moveScores[bestIndex];
  moveScores[bestIndex]=tempScore;

} // End sortMoves.

// ==========================================================================

