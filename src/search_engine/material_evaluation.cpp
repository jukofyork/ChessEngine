// ****************************************************************************
// *                        MATERIAL EVALUATION FUNCTIONS                     *
// ****************************************************************************

#include "search_engine.h"

// ==========================================================================

void updateMaterialEvaluation(RunningMaterial &runningMaterial, int currentPly,
                              const MoveStruct &moveMade)
{ // This function update the running material evaluation, with the move made.

  // Copy the last move's material evaluations to this new move.
  runningMaterial.pieceMatValue[currentPly+1][WHITE]=runningMaterial.pieceMatValue[currentPly][WHITE];
  runningMaterial.pieceMatValue[currentPly+1][BLACK]=runningMaterial.pieceMatValue[currentPly][BLACK];
  runningMaterial.pawnMatValue[currentPly+1][WHITE]=runningMaterial.pawnMatValue[currentPly][WHITE];
  runningMaterial.pawnMatValue[currentPly+1][BLACK]=runningMaterial.pawnMatValue[currentPly][BLACK];

  // Captures lower material.
  if (moveMade.type&CAPTURE) {

    // En passent removes one pawm.
    if (moveMade.type&EN_PASSANT)
      runningMaterial.pawnMatValue[currentPly+1][g_currentSide]-=PIECE_VALUE[PAWN];

    // Normal moves remove the piece taken.
    else {
      runningMaterial.pieceMatValue[currentPly+1][g_currentSide]
                   -=PIECE_VALUE[g_gameHistory[g_moveNum-1].piece[moveMade.target]];
    }

  }

  // Promotions increase material (and lose a pawn!).
  if (moveMade.type&PROMOTION) {
    runningMaterial.pieceMatValue[currentPly+1][getOtherSide(g_currentSide)]+=PIECE_VALUE[moveMade.promote];
    runningMaterial.pawnMatValue[currentPly+1][getOtherSide(g_currentSide)]-=PIECE_VALUE[PAWN];// P gone.
  }

} // End updateMaterialEvaluation.

// ==========================================================================

