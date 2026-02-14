// **************************************************************************
// *                           QUICK SEARCH FUNCTIONS                       *
// **************************************************************************

#include "search_engine.h"

#include <algorithm>

using namespace std;

// 2003: Have now made the Quick quiescent search time out.
//       This was done to stop 'silly' exibition games from causing problems.

// =========================================================================

// This is used to stop the search from doing daft stuff and taking ages when
// we find a 'silly' exibition game...
constexpr int MAscore_NODES_TO_TRY = 1000000;

// This holds the time we want to stop the search at, if still going.
int g_qsNumNodesSearched;

// =========================================================================

int isQuiescent(void)
{ // This function simply finds if the current position is quiescent.
  // It does this by compareing the reults of GetQuiescentScore, with the
  // current material eval, if they are the same, then the position IS
  // quiescent, otherwise it is not.
  // 2003: If the search 'times out', we return -1...

  int currentPly=0;

  int quiescentScore;                           // The score returned.

  // Running material (for speed).
  RunningMaterial sd;

  // Set up the material evaluations for this state.
  sd.pieceMatValue[0][WHITE]=0;
  sd.pieceMatValue[0][BLACK]=0;
  sd.pawnMatValue[0][WHITE]=0;
  sd.pawnMatValue[0][BLACK]=0;
  for (int i=0;i<64;i++) {

    // Don't bother if theres no piece on the square.
    if (g_currentPiece[i]==NONE)
      continue;

    // See if it's a pawn or a piece.
    if (g_currentPiece[i]==PAWN)
      sd.pawnMatValue[0][g_currentColour[i]]+=PIECE_VALUE[PAWN];
    else
      sd.pieceMatValue[0][g_currentColour[i]]+=PIECE_VALUE[g_currentPiece[i]];

  }

  // Set up a sensible limit on the number of search nodes...
  g_qsNumNodesSearched=0;

  // See if quiescent or not.
  quiescentScore=getQuiescentScore(sd);

  // 2003: Has the search 'timed-out'?
  if (g_qsNumNodesSearched>MAscore_NODES_TO_TRY)
    return -1;                                    // Failed.

  // Return true or false.
  if (quiescentScore==getMaterialEval(sd, g_currentSide, getOtherSide(g_currentSide), currentPly))
    return true;
  else
    return false;

} // End isQuiescent.

// ==========================================================================

int getQuiescentScore(RunningMaterial &sd)
{ // This function returns the quiescent *MATERIAL* score for a position.
  // It calls QuickQuiesceSearch() to do the work.

  // Return the value.
  return quickQuiesceSearch(sd,0,-WIN_SCORE,WIN_SCORE);

} // End getQuiescentScore.

// ============================================================================

int quickQuiesceSearch(RunningMaterial &sd,int currentPly,int alpha,int beta)
{ // This function simply finds the quiescent *MATERIAL* score for a position.
  // The function calls itself recusively, but uses the minimum code (unlike
  // QuiesceSearch, which has lots of book keeping code).
  // THIS IS THE FAIL-SOFT VERSION: MAKES NO SPEED DIFF, BUT MAY HAVE A BUG IN
  //                                IT???
  // clean_up13: Have fixed the slowdown bug, it was the result of not sorting
  //             the moves (MVV-LVA).

  int score;                     // Returned from recursicve call.
  bool found=false;              // true if at least 1 legal move found.

  // Fail-Soft stuff.
  int best;                      // Best score so far.

  // This is the list of Moves/Captures generated from this state.
  MoveList moves;
  int      moveScores[MOVELIST_ARRAY_SIZE];

  // One more node tried now.
  g_qsNumNodesSearched++;

  // Check to see if timed out (To avoid 'silly' exibition games...).
  if (g_qsNumNodesSearched>MAscore_NODES_TO_TRY)
    return 0;                    // Search invalid now , leaving recusion.

  // Test here so we can cut off the search as pointless to search further.
  if (g_currentState->isDraw)
    return getDrawScore();          // Draw situtaion.

  // MATERIAL ONLY!
  best=getMaterialEval(sd, g_currentSide, getOtherSide(g_currentSide), currentPly);       // Get material eval.

  // Generate all moves if in check, else just gen captures (if no cut!).
  if (g_currentState->inCheck) {
    genMoves(moves);
  }
  else {

    // See if we can cut off search here.
    if (best>=beta)
      return best;

    // Generate only captures and promotions.
    genCaptures(moves);

  }

  // Sort the moves (MVV-LVA).
  for (int i=0;i<moves.numMoves;i++) {
    if (moves.moves[i].type&PROMOTION) {
      moveScores[i]=PROMOTION_SORT_SCORE+(moves.moves[i].promote*10);

      // Add an even higher score if we capture on the promotion!
      if (moves.moves[i].type&CAPTURE)
        moveScores[i]+=g_currentPiece[moves.moves[i].target]*10;
    }
    else if (moves.moves[i].type&CAPTURE) {
      moveScores[i]=CAPTURE_SORT_SCORE+((g_currentPiece[moves.moves[i].target]*10)
                                        -g_currentPiece[moves.moves[i].source]);

      // If we are capturing the last piece moved by opponent, make it higher.
      if (g_moveNum>1
          && (g_gameHistory[g_moveNum-1].piece[moves.moves[i].target]
              !=g_gameHistory[g_moveNum-2].piece[moves.moves[i].target]
              || g_gameHistory[g_moveNum-1].colour[moves.moves[i].target]
                 !=g_gameHistory[g_moveNum-2].colour[moves.moves[i].target])) {
        moveScores[i]++;
      }
    }
    else {
      moveScores[i]=0;
    }
  }

  // Init the fail-soft stuff.
  alpha=std::max(best,alpha);

  // Loop through the moves.
  for (int i=0;i<moves.numMoves;i++) {

    // Sort the moves.
    sortMoves(moves,moveScores,i);

    if (!makeMove(moves.moves[i]))
      continue;
    found=true;                    // We have found a legal move.

    // Update the material evaluation.
    updateMaterialEvaluation(sd,currentPly,moves.moves[i]);

    // Search the next ply.
    score=-quickQuiesceSearch(sd,currentPly+1,-beta,-alpha); // Call self.

    takeMoveBack();                // Take the move back.

    // Check to see if timed out (To avoid 'silly' exibition games...).
    if (g_qsNumNodesSearched>MAscore_NODES_TO_TRY)
      return 0;                    // Search invalid now , leaving recusion.

    // See if score is better than old best.
    if (score>best) {

      // Save for later.
      best=score;                      // Save the new best score.

      // See if it's a beta-cuttoff.
      if (best>=beta)
        return best;

      // Fail soft condition.
      if (best>alpha)
        alpha=best;

    }

    // See if it's a beta-cuttoff (so we can prune here after avoiding check).
    else if (score>=beta)
      return score;

    // Have we found a (SHORT AS POSIBLE!) mate, if so don't search any more.
    // NOTE: Don't make this >WIN_SCORE-100 or we'll miss shorter mates!
    if ((best+currentPly)==(WIN_SCORE-1))
      return best;
  }

  // If we're in check and there aren't any legal moves, well, we lost.
  // NOTE: This is not definitely forced and should be extended in Search() to
  //       make sure it realy is a forced mate.
  if ((!found) && g_currentState->inCheck)
    best=(-WIN_SCORE)+currentPly; // <*** N moves to go can be worked out!

  // Return the best found.
  return best;

} // End quickQuiesceSearch.

// ==========================================================================

