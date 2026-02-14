// **************************************************************************
// *                         QUIESCENT SEARCH FUNCTION                      *
// **************************************************************************

#include "search_engine.h"

#include <algorithm>
#include <iostream>

using namespace std;

// ==========================================================================

int quiesceSearch(SearchData &searchData,int currentPly,int alpha,int beta,
                  bool nullMove)
{ // This function  is a recursive minimax search function with alpha-beta 
  // cutoffs. In other words, negamax. It basically only searches capture 
  // sequences and allows the evaluation function to cut the search off 
  // (and set alpha). The idea is to find a position where there isn't a 
  // lot going on so the static evaluation function will work.

  int score;                     // Returned from Eval() and recursicve call.
  bool found=false;              // true if at least 1 legal move found.

  // For the TTable.
  uint8_t flags;                 // Flags returned from TTGet() (node params).
  int upperbound=WIN_SCORE;      // Best should not end up exceeding this.

  // Fail-Soft stuff.
  MoveStruct bestMove;           // The best move fount (may be emty!).
  int best;                      // Best score so far.
  int saveAlpha=alpha;           // Value of alpha on entry.

  // The hash key of the move we just made.
  HashKey nextHashKey;

  // This is the best Next hash key we got.
  HashKey bestHashKey=0;

  // Used to speed up.
  int mEval,pEval;

  // This is the list of moves/Captures generated from this state.
  MoveList moves;
  int      moveScores[MOVELIST_ARRAY_SIZE];

  // Check to see if timed out (Time is huge if no time limit!).
  if (shouldTimeOut(searchData)==true)
    return 0;                    // Search invalid now , leaving recusion.

  // One more node searched.
  searchData.totalNodesSearched++;

  // Always push up these values if needed (in case we only have 1 move!).
  if (searchData.minPositionEval[currentPly]<searchData.minPositionEval[currentPly+1])
    searchData.minPositionEval[currentPly+1]=searchData.minPositionEval[currentPly];
  if (searchData.maxPositionEval[currentPly]>searchData.maxPositionEval[currentPly+1])
    searchData.maxPositionEval[currentPly+1]=searchData.maxPositionEval[currentPly];

  // Init the best move to empty.
  bestMove = MoveStruct{NONE, NONE, NORMAL_MOVE, NO_PROMOTION};

  // Test here so we can cut off the search as pointless to search further.
  if (g_currentState->isDraw) {
    best=getDrawScore();               // Draw situtaion.
    goto LeaveQSearch;               // To save in TTable ect.
  }

  // Probe the transposition table for a score and a move.
  // If the score is an upperbound, then we can use it to improve the value
  // of beta.  If a lowerbound, we improve alpha.  If it is an exact score,
  // if we now get a cut-off due to the new alpha/beta, return the score.

  // See if the states in the hash already.
  flags=ttGet(searchData,currentPly,0,score,searchData.hashMoves[currentPly],nextHashKey);
  searchData.totalGetHashCount++;
  if (flags!=0)
    searchData.numHashSuccesses++;                      // One more success.

  // Is it an exact score, if so leave with it.
  if (flags==EXACTSCORE) {
    return score;                       // Was saved with alpha==beta to be exact.
  }

  // Is it a new Upper Bound?
  else if (flags==UPPERBOUND) {
     beta=std::min(beta,score);              // Set beta.
     if (beta<=alpha)
        return score;                   // Cut the node.
     upperbound=score;                  // For testing at end (for errors).
      //BanNullForThisCall=true;       // Don't allow null move this call (why?).
  }

  // Is it a new lower bound.
  else if (flags==LOWERBOUND) {
     if (score>=beta)
        return score;                   // Cut the node.
     alpha=score;                       // Set alpha to score.
  }

  // Set the initial value of best to the evaluation.
  // Try to use a cheap estimate, if possible (taking 1 pawn as max pos value!).
  mEval=getMaterialEval(searchData, g_currentSide, getOtherSide(g_currentSide), currentPly);
  if (currentPly>0
      && (((mEval-searchData.minPositionEval[currentPly-1])
           +static_cast<int>(EVAL_WINDOW*static_cast<double>(PIECE_VALUE[PAWN])))<alpha
          || ((mEval-searchData.maxPositionEval[currentPly-1])
              -static_cast<int>(EVAL_WINDOW*static_cast<double>(PIECE_VALUE[PAWN])))>beta)) {
    searchData.numMaterialEvals++;

    // Set best to a pesimistic value rather than just the material, in case
    // we decide to use htis - in which case it want's re-doing.
    //best=(MATERIAL_EVAL(searchData)+MinPositionEval[currentPly-1])-EVAL_WINDOW;
    best=mEval;                // Use esitimate.
  }
  else {
    searchData.numTrueEvals++;

    pEval=searchData.evalParams.eval();       // Use real evaulation.
    best=mEval+pEval; // Use real evaulation.

    // Is it as new maximum positional evaluation score for this depth?
    if (pEval<searchData.minPositionEval[currentPly])
      searchData.minPositionEval[currentPly]=pEval;
    if (pEval>searchData.maxPositionEval[currentPly])
      searchData.maxPositionEval[currentPly]=pEval;

  }

  // FOR NN (clean_up17):
  //best=evalParams.Eval();  // Use real evaulation.

  // Generate all moves if in check, else just gen captures (if no cut!).
  if (g_currentState->inCheck) {
    genMoves(moves);
    searchData.totalMoveGens++;
    scoreMoves(searchData,currentPly,moves,moveScores,nullMove);
  }
  else {

    // See if we can cut off search here.
    if (best>=beta) {
      searchData.numBetaCutOffs++;
      goto LeaveQSearch;
    }

    // Generate only captures and promotions.
    genCaptures(moves);
    searchData.totalMoveGens++;
    scoreMoves(searchData,currentPly,moves,moveScores,nullMove);

  }

  // Init the fail-soft stuff.
  alpha=std::max(best,alpha);

  // Loop through the moves.
  for (int i=0;i<moves.numMoves;i++) {

    sortMoves(moves,moveScores,i);

    if (!makeMove(moves.moves[i]))
      continue;

    // Update the material evaluation.
    updateMaterialEvaluation(searchData,currentPly,moves.moves[i]);

    found=true;                    // We have found a legal move.

    // Search the next ply.
    score=-quiesceSearch(searchData,currentPly+1,-beta,-alpha,nullMove);// Call self.

    // Get the hash key before we take the move back.
    nextHashKey=g_currentState->key;

    takeMoveBack();                // Take the move back.

    // Check to see if timed out (Time is huge if no time limit!).
    if (shouldTimeOut(searchData)==true)
      return 0;                    // Search invalid now , leaving recusion.

    // See if score is better than old best.
    if (score>best) {

      bestMove=moves.moves[i];

      bestHashKey=nextHashKey;

      // Save for later.
      best=score;                      // Save the new best score.

      // See if it's a beta-cuttoff.
      if (best>=beta) {
        searchData.numBetaCutOffs++;
        goto LeaveQSearch;         // For update ect.
      }

      // Fail soft condition.
      if (best>alpha)
        alpha=best;

    }

    // See if it's a beta-cuttoff (so we can prune here after avoiding check).
    else if (score>=beta) {
      searchData.numBetaCutOffs++;
      best=score;                        // Cuttoff?
      goto LeaveQSearch;             // For update ect.
    }

    // Have we found a (SHORT AS POSIBLE!) mate, if so don't search any more.
    // NOTE: Don't make this >WIN_SCORE-100 or we'll miss shorter mates!
    if ((best+currentPly)==(WIN_SCORE-1))
      goto LeaveQSearch;             // For update ect.

  }

  // If we're in check and there aren't any legal moves, well, we lost.
  // NOTE: This is not definitely forced and should be extended in Search() to 
  //       make sure it realy is a forced mate.
  if ((!found) && g_currentState->inCheck)
    best=(-WIN_SCORE)+currentPly; // <*** N moves to go can be worked out!

  // Jump here when a draw if found at the top of function.
  LeaveQSearch:

  // Update the TTable here.
  if (g_searchConfig.enableSearchDiagnostics && upperbound<best)
    std::cout << "Inconsistencies UB:" << upperbound << " best:" << best << std::endl;
  ttPut(searchData,currentPly,0,saveAlpha,beta,best,bestMove,bestHashKey);

  // Update the cuttoff totals.
  // NOTE: Don't alter the move history here (ends up slower! - more nodes).
  if (best>saveAlpha && bestMove.source!=-1) {
    searchData.numAlphaCutOffs++;

    // Move history.
    searchData.moveHistory[bestMove.source][bestMove.target]|=1;

    // Killer moves.
    if (!(bestMove.type&CAPTURE
          || bestMove.type&PROMOTION)) {
      if (searchData.killerMovesOld[currentPly].source==-1) {
        searchData.killerMovesOld[currentPly]=bestMove; // First.
      }
      else if (searchData.killerMovesOld[currentPly].source
               !=bestMove.source
               || searchData.killerMovesOld[currentPly].target
                  !=bestMove.target) {
        searchData.killerMovesNew[currentPly]=bestMove; // Last.
      }
    }

  }

  // Return the best found.
  return best;

} // End quiesceSearch.

// ==========================================================================
