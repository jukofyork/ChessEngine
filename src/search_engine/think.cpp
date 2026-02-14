// **************************************************************************
// *                              THINK FUNCTION                            *
// **************************************************************************

#include "search_engine.h"
#include "../interface/interface.h"
#include <algorithm>
#include <limits>

using namespace std;

// ==========================================================================

MoveStruct think(int searchDepth,double maxTimeSeconds,bool showOutput,
                 bool showThinking,double randomSwing,const EvaluationParameters &evalParams)
{ // This function calls search() iteratively and prints the thinking results
  // after every iteration (if asked to show thinking).
  // The move chosen is returned as a MoveStruct (with type/promotion ect).
  // NOTE: If Time linit is set, then depth is set infinate and as soon as the
  // time is exceeded, the computer will pick the best move found on the last
  // fully searched/completed ply as long as at least 2 plys have been searched.
  // 2003_v7: Now uses the 'random_swing' value on the eval set at the start,
  //          using the 'mutate' function...

  // The last score returned from the previous level of search (Aspiration...).
  int lastScore=0;

  // This is the data type that holds everything used while searching!
  // This is done to make it so only one extra parameter need be pass to
  // the Search() functions.
  static SearchData sd;
  
  // Initialize SearchData vectors based on current configuration.
  // Note: sd is static to reuse allocated memory across searches.
  sd.reset();

  // Copy it in to the Search Data.
  //memcpy(&sd.evalParams,&evalParams,sizeof(sd.evalParams)); // BAD FOR NN (=MEM LEAK *BUGS*)!
  sd.evalParams=evalParams;                           // Using assignment operator.

  // 2003_v7: Mutate the eval set.
  sd.evalParams.mutate(randomSwing);

  // Set the starting time of the search (for thinking info!).
  // NOTE: Do after clearing the big tables/lists/hash ect as their slow...
  if (showOutput==true && showThinking==true) {
    sd.startTime=getTime();
    sd.wallClockStart=getWallClockTime();
    sd.cpuStart=getCPUTime();
  }
  else {
    sd.startTime=0;                                  // For search to see.
    sd.wallClockStart=0.0;
    sd.cpuStart=0.0;
  }

  // Save the stopping time (in processor clock cycles).
  if (searchDepth==static_cast<int>(INFINITE_DEPTH))
    sd.stopTime=getTime()
                +(static_cast<ClockTime>(maxTimeSeconds*getClocksPerSecond()));
  else
    sd.stopTime=std::numeric_limits<ClockTime>::max();  // Don't stop search on time limit!

  // Print thinking table's header.
  if (showOutput)
    cout << "Please wait, Thinking..." << endl;
  if (showThinking && showOutput) {
    cout << "=================================================================="
         << endl;
    cout << "| Ply | Time      | Nodes      | Score    | Principal Variation..."
         << endl;
    cout << "|-----|-----------|------------|----------|-----------------------"
         << endl;
  }


  // Set up the the max positional value for ply 0 from a call to Eval().
  sd.minPositionEval[0]=sd.maxPositionEval[0]=sd.evalParams.eval();

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

  // Init first level (ie: Depth=1) to full width window.
  sd.rootAlpha=-WIN_SCORE;
  sd.rootBeta=WIN_SCORE;

  // Run for each iteration, going deeper each time.
  for (sd.iterDepth=1;;sd.iterDepth++) {

    // Aspiration search.
    // NOTE: The move's score it returned in ComputersMove.Score, as we may get
    //       a 0 here if we timed out.
    for (;;) {

      // Do the search with a reduced window.
      lastScore=search(sd,0,sd.rootAlpha,sd.rootBeta,sd.iterDepth,false);

      // Break if thime is up.
      if (shouldTimeOut(sd)==true)
        break;

      // See if we failed low/high.
      if (lastScore<=sd.rootAlpha)
        sd.rootAlpha=-WIN_SCORE;                 // Fail low.
      else if (lastScore>=sd.rootBeta)
        sd.rootBeta=WIN_SCORE;                   // Fail high.
      else
        break;

    }

    // Reset the aspiration window.
    sd.rootAlpha=lastScore-static_cast<int>(ASPIRATION_WINDOW*static_cast<double>(PIECE_VALUE[PAWN]));
    sd.rootBeta=lastScore+static_cast<int>(ASPIRATION_WINDOW*static_cast<double>(PIECE_VALUE[PAWN]));

    // Print the move chosen (different for partialy searched ply, but usable!).
    if (showThinking && showOutput) {
      if (shouldTimeOut(sd)==true)
        printLine(sd,sd.computersMove,sd.computersMoveScore,'%');
      else
        printLine(sd,sd.computersMove,sd.computersMoveScore,'.');
    }

    // Break time is up/depth is reached or definite forced mate.
    // Note: No MAX_SEARCH_DevalParamsTH limit - search continues until depth/time/mate.
    if ((sd.iterDepth==searchDepth && maxTimeSeconds==INFINITE_TIME)
        || shouldTimeOut(sd)==true
        || isMateScore(sd.computersMoveScore)) {
      break;
    }

  }

  // Print rest of the info.
  if (showThinking && showOutput) {
    cout << "=================================================================="
         << endl;

    // Print stats - both wall clock and CPU time for comparison.
    double wallClockElapsed = getWallClockTime() - sd.wallClockStart;
    double cpuElapsed = getCPUTime() - sd.cpuStart;
    cout << "Wall Clock Time                 : " << wallClockElapsed << " seconds" << endl;
    cout << "CPU Time                        : " << cpuElapsed << " seconds" << endl;
    cout << "Total Nodes Searched            : " << sd.totalNodesSearched << endl;
    if (wallClockElapsed > 0.0) {
      cout << "Nodes Per Second (wall clock)   : " 
           << (int)((double)sd.totalNodesSearched / wallClockElapsed)
           << endl;
    }
    if (cpuElapsed > 0.0) {
      cout << "Nodes Per Second (CPU)          : " 
           << (int)((double)sd.totalNodesSearched / cpuElapsed)
           << endl;
    }
    cout << "Total Move Gens                 : " << sd.totalMoveGens << endl;
    cout << "Total Alpha Updates             : " << sd.numAlphaCutOffs << endl;
    cout << "Total Beta Cutoffs              : " << sd.numBetaCutOffs << endl;
    cout << "Total Null Cutoffs              : " << sd.numNullCutOffs << endl;
    cout << "Total Material Evals            : " << sd.numMaterialEvals << endl;
    cout << "Total True Evals                : " << sd.numTrueEvals << endl;
    cout << "Total Hash Colisions            : " << sd.numHashCollisions 
                                                 << endl;
    cout << "Total Put In Hash               : " << sd.totalPutHashCount 
                                                 << endl;
    cout << "Total Get Hash Count            : " << sd.totalGetHashCount 
                                                 << endl;
    cout << "Total Hash Successes            : " << sd.numHashSuccesses << endl;
    cout << "Total Check Extentions          : " << sd.numCheckExtensions 
                                                 << endl;
    cout << "Total Mate Extentions           : " << sd.numMateExtensions 
                                                 << endl;

    // Find the Max positional difference for any ply of search.
    sd.maxPositionalDiff=0;
    // Use the actual vector size to prevent out-of-bounds access
    const size_t maxPly = sd.minPositionEval.size();
    for (size_t i=1; i<maxPly; i++) {
      if (sd.minPositionEval[i]==WIN_SCORE) {
        cout << "Min/Max positonal score seen    : "
             << (double)sd.minPositionEval[i-1]/(double)PIECE_VALUE[PAWN] << '/'
             << (double)sd.maxPositionEval[i-1]/(double)PIECE_VALUE[PAWN] 
             << " (" << i-2 << ')' << endl;
        cout << "Max positional score difference : "
             << (double)sd.maxPositionalDiff/(double)PIECE_VALUE[PAWN]
             << " (Current Window is " << EVAL_WINDOW << ')' << endl;
        break;
      }
      if (labs((-sd.minPositionEval[i])-sd.maxPositionEval[i-1])
          >sd.maxPositionalDiff) {
        sd.maxPositionalDiff=labs((-sd.minPositionEval[i])
                               -sd.maxPositionEval[i-1]);
      }
      if (labs((-sd.maxPositionEval[i])-sd.minPositionEval[i-1])
          >sd.maxPositionalDiff) {
        sd.maxPositionalDiff=labs((-sd.maxPositionEval[i])
                               -sd.minPositionEval[i-1]);
      }
    }

    // Tell the user about forced mate.
    // clean_up6: Have now made it report that a mate will be forced in N+1
    //            moves to make easier to understand output. What does X will
    //            force mate in 0 moves mean? - Rubish, N will force mate in 1
    //            move - ie: The move we have chosen, will mate after the move! 
    if (isMateScore(sd.computersMoveScore)) {
      if ((g_currentSide==WHITE 
           && ((WIN_SCORE-1)-labs(sd.computersMoveScore))%2==1)
          || (g_currentSide==BLACK 
              && ((WIN_SCORE-1)-labs(sd.computersMoveScore))%2==0)) {
        cout << endl << "Black will force mate in " 
             << (WIN_SCORE)-labs(sd.computersMoveScore) 
             << " moves." << endl;
      }
      else {
        cout << endl << "White will force mate in " 
             << (WIN_SCORE)-labs(sd.computersMoveScore) 
             << " moves." << endl;
      }
    }
  }

  // Return the move chosen by the computer.
  return sd.computersMove;

} // End think.

// ==========================================================================

