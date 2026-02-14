// **************************************************************************
// *                             SEARCH FUNCTION                            *
// **************************************************************************

#include "search_engine.h"
#include "../interface/interface.h"

#include <algorithm>
#include <iostream>

using namespace std;

// ==========================================================================

int search(SearchData &searchData,int currentPly,int alpha,int beta,int depth,
           bool nullMove)
{ // This function searches useing negamax algorithm.
  // Now uses the null move heuristic.
  // NOTE: If StartTime=0, then won't show thinking!

  int score;                     // Returned from recursicve call.
   bool found=false;              // true if at least 1 legal move found.

   // Fail-Soft stuff.
   MoveStruct bestMove;           // The best move fount (may be emty!).
   int best=-WIN_SCORE;           // best score so far.
    int saveAlpha=alpha;           // Value of alpha on entry.

   // For the TTable.
   uint8_t flags;                 // flags returned from TTGet() (node params).
   int upperbound=WIN_SCORE;      // best should not end up exceeding this.

   // This is so the TTable can stop us useing the null move (THIS CALL ONLY!).
   bool banNullForThisCall=false;
   // This is used to store the enpassent/fifty counter info before a null move.
   int oldEnPass,oldFiftyCounter;

   // The hash key of the move we just made.
   HashKey nextHashKey;

   // This is the best Next hash key we got.
   HashKey bestHashKey=0;

   // This is the list of moves/Captures generated from this state.
   MoveList moves;
   int      moveScores[MOVELIST_ARRAY_SIZE];

  // Check to see if timed out (Time is huge if no time limit!).
  if (shouldTimeOut(searchData)==true)
    return 0;                    // Search invalid now , leaving recusion.

  // Are we in check? If so, we want to search deeper so that the effective
  // search depth is not reduced by the opponent checking us again and again.
  // NOTE: Done here - Before Quiesce test!
  // Note: No MAX_SEARCH_DEPTH limit - check extensions allowed at any depth.
  if (g_currentState->inCheck) {
    searchData.numCheckExtensions++;
    depth++;          // Extend depth.
  }

  // If as deep as we want to be; call QuiesceSearch() to get a reasonable 
  // score and return it.
  // BUG: Don't increase current ply here!
  // BUG: Don't attempt to jump to end of function here like on draw ect!
  // NEW: If we get a mate back from quiesce, then extend to make sure.
  if (depth<=0) {
    score=quiesceSearch(searchData,currentPly,alpha,beta,nullMove);
    if (isMateScore(score)) {
      searchData.numMateExtensions++;
      depth++;                    // Extend.
    }
    else {
      return score;                   // Use it as score.
    }
  }

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
    goto LeaveSearch;                // To save in TTable ect.
  }

  // Probe the transposition table for a score and a move.
  // If the score is an upperbound, then we can use it to improve the value
  // of beta.  If a lowerbound, we improve alpha.  If it is an exact score,
  // if we now get a cut-off due to the new alpha/beta, return the score.

  // See if the states in the hash already.
   flags=ttGet(searchData,currentPly,depth,score,searchData.hashMoves[currentPly],
              nextHashKey);
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
     banNullForThisCall=true;       // Don't allow null move this call (why?).
  }

  // Is it a new lower bound.
  else if (flags==LOWERBOUND) {
     if (score>=beta)
        return score;                   // Cut the node.
      //alpha=std::max(alpha,score);          // Not sure, but worth a try later?
     alpha=score;                       // Set alpha (why not like above?).
  }

  // Attempt to cut off the search with the (deep) null move heuristic.
  // Not done if any of the following are so:
  // 1. If the current ply is 0, as this always has beta==WIN_SCORE at this
  //    point in the function (ie: In the initial recursive call).
  //    Also it seems to help stop promlems if > 1.
  // 2. Last move was a null move also (Would just swap back!).
  // 3. The search depth is 2 or greater (depth 1 not work and causes bugs!).
  // 4. We are following the Primary Variation (not needed, but saves a call!).
  // 5. If we are in check (as swapping sides would make impossible position!).
  // 6. beta indicates we will lose on the next ply (need to search then as may
  //    be a way to get out of the mate down this line).
  // 7. The side to move has less than N as his peice material. This is to
  //    reduce the possibility of Zugzwang (ie: All own moves being 
  //    detremental to self, with smaller evaluations than before the move).
  //    This generally happens in the end-game, when you have a knigh or bishop
  //    and/or some pawns (possibly a rook?).
  //    Also if the other side has no material, then null never seems to work
  //    so just don't bother.
  // 8. If Material+1 pawn is less than beta (not sure but no-slowdown...).
  if (currentPly>1
      && banNullForThisCall==false
      && (nullMove==false)
      && depth>1
      && !g_currentState->inCheck 
      && (getMaterialEval(searchData, g_currentSide, getOtherSide(g_currentSide), currentPly)+PIECE_VALUE[PAWN])>beta
      && beta>(-(WIN_SCORE-1))+currentPly
      && getPieceMaterial(searchData, g_currentSide, currentPly)>PIECE_VALUE[BISHOP]
      && getPieceMaterial(searchData, getOtherSide(g_currentSide), currentPly)>0) {

    // Make the null move (ie: Simply switch sides!).
    g_currentSide=getOtherSide(g_currentSide);

    // Save and clear the old enpassent/fifty counter, in case it was set.
    oldEnPass=g_currentState->enPass;
    g_currentState->enPass=NO_EN_PASSANT;
    oldFiftyCounter=g_currentState->fiftyCounter;
    g_currentState->fiftyCounter=0; // Init to Zero to stop TestRep from using!

    // Call Search() to find score, we use depth -3 as we are expecting to
    // search at least 2 plys further than without it (I think?!). Also Use a 
    // one sized window from current beta (full window for Deep Null?).
    // clean_up14: Now don't increase current ply, as no move made!
    score=-search(searchData,currentPly,-beta,(-beta)+1,depth-3,true);

    // Take the (null) move back (ie: Simply switch sides!).
    g_currentSide=getOtherSide(g_currentSide);

    // Restore the old enpassent/firty counter info.
    g_currentState->enPass=oldEnPass;
    g_currentState->fiftyCounter=oldFiftyCounter;

    // Check to see if timed out (Time is huge if no time limit!).
    if (shouldTimeOut(searchData)==true)
      return 0;                    // Search invalid now , leaving recusion.

    // If score>=beta, then we can cut the node.
    if (score>=beta) {
      searchData.numNullCutOffs++;                      // One more null cutoff done.
      best=score;                                // Save as score.
      goto LeaveSearch;                      // For TTable update.
    }

    // Not sure what this extention is for? (cap extention distance in case!)
    // Note: No MAX_SEARCH_DEPTH limit.
    if ((depth-3)>=1 
             && getMaterialEval(searchData, g_currentSide, getOtherSide(g_currentSide), currentPly)>beta && score<=((-WIN_SCORE)+100)) {
      depth++;
    }

  }

    // Generate all moves.
  genMoves(moves);
  searchData.totalMoveGens++;
   scoreMoves(searchData,currentPly,moves,moveScores,nullMove);

  // Loop through the moves.
   for (int i=0;i<moves.numMoves;i++) {

     sortMoves(moves,moveScores,i);

    // Try to make the move.
    if (!makeMove(moves.moves[i]))
      continue;

    // Update the material evaluation.
    updateMaterialEvaluation(searchData,currentPly,moves.moves[i]);

    // Search with a full window for the first branch and zero for others.
    if (found==false) {
      found=true;                  // Have got a legal move now.
      score=-search(searchData,currentPly+1,-beta,-alpha,depth-1,nullMove);
    }
    else {

      // Fail soft condition.
      alpha=std::max(best,alpha);

      // Search with a zero window.
      score=-search(searchData,currentPly+1,(-alpha)-1,-alpha,depth-1,nullMove);

      // Do we need to re-search the move.
      if (score>best && score>alpha && score<beta) {
        // BUG: This line had a the re-search call set to -beta,-score and I think
        //      this caused the correct search/score to be done, but the
        //      PV ends up a load of bollucks! - This seems to sort it, with
        //      no (or very little) extra speed cost (just forces an update
        //      of the PV to be done again - to keep it up-to-date!
        // BUG2: Have now made it so that when we re-search we don't allow the
        //       null move to be used. This SHOULD finally stop the negative
        //       length PVs (I flaming hope so after all this time!).
        score=-search(searchData,currentPly+1,-beta,-score,depth-1,nullMove);
      }
    }

    // Get the hash key before we take the move back.
    nextHashKey=g_currentState->key;

    takeMoveBack();                 // Take the move back.

    // Check to see if timed out (Time is huge if no time limit!).
    if (shouldTimeOut(searchData)==true)
      return 0;                     // Search invalid now , leaving recusion.

    // Test aginst best.
    if (score>best) {

      bestMove=moves.moves[i];

      bestHashKey=nextHashKey;

      // Save it for later.
      best=score;                       // Save the new best score.

      // See if it's a fail high/low on first ply.
      if (currentPly==0) {
   if (best<=searchData.rootAlpha) {
           if (searchData.startTime!=0 && searchData.iterDepth>1)
            printLine(searchData,bestMove,best,'-');
          return best;             // Exit and restart.
        }
     else if (best>=searchData.rootBeta) {
           if (searchData.startTime!=0 && searchData.iterDepth>1)
            printLine(searchData,bestMove,best,'+');
          return best;             // Exit and restart.
        }
      }

      // See if it's a cuttoff?
      if (best>=beta) {
        searchData.numBetaCutOffs++;
        goto LeaveSearch;           // So we may update killers ect.
      }

      // Save the score for the move (for Think() to see and print).
      // NOTE: Only safe if not failed high/low.
       if (currentPly==0 && best>searchData.rootAlpha && best<searchData.rootBeta) {
         searchData.computersMove = bestMove;
         searchData.computersMoveScore=best;    // Save it.
         if (searchData.startTime!=0 && searchData.iterDepth>1)
          printLine(searchData,searchData.computersMove,searchData.computersMoveScore,'&');
      }

    }

    // Have we found a (SHORT AS POSIBLE!) mate, if so don't search any more.
    // NOTE: Don't make this >WIN_SCORE-100 or we'll miss shorter mates!
    if ((best+currentPly)==(WIN_SCORE-1))
      goto LeaveSearch;              // So we may update killers ect.

  }

  // If No legal moves? then we're in checkmate or stalemate.
  // BUG: Set best and then see if it needs storing anywhere (history ect).
  if (!found) {
    if (g_currentState->inCheck)
      best=(-WIN_SCORE)+currentPly;    // Checkmate.
    else
      best=getDrawScore();               // Stalemate.
  }

  // Jump here when a draw if found at the top of function.
  LeaveSearch:

  // Update the TTable here.
  if (g_searchConfig.enableSearchDiagnostics && upperbound<best)
    std::cout << "Inconsistencies UB:" << upperbound << " best:" << best << std::endl;
   ttPut(searchData,currentPly,depth,saveAlpha,beta,best,bestMove,
        bestHashKey);

  // As this move caused a alpha update, it's history value should be 
  // increased. Note, beta cuttoff will get updated here too!
  // Also add it to the killer move vector.
   if (best>saveAlpha && bestMove.source!=-1) {
     searchData.numAlphaCutOffs++;
    searchData.moveHistory[bestMove.source][bestMove.target]|=(1<<depth);

    // Add this move to a killer move vector for this ply.
    // NOTE: Keep captures and promotions as non-killers, as they are done
    //       first anyway!
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

  // Return the best found here.
  return best;

} // End search.

// ==========================================================================
