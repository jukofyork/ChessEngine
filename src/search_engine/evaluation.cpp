// **************************************************************************
// *                        EVALUATION CODE/FUNCTIONS                       *
// **************************************************************************
  
#include "evaluation.h"
#include "search_engine.h"
#include "../chess_engine/constants.h"
#include <array>
#include <random>

using namespace std;

// Static member definitions - initialized from global config
bool EvaluationParameters::useLinearTraining = g_evaluationConfig.useLinearTraining;
bool EvaluationParameters::useKingDistanceFeatures = g_evaluationConfig.useKingDistanceFeatures;
bool EvaluationParameters::useEmptySquareFeatures = g_evaluationConfig.useEmptySquareFeatures;
bool EvaluationParameters::useSuperFastEval = g_evaluationConfig.useSuperFastEval;

// #############################################################################
// #                      PUBLIC (USER) MEMBER FUNCTIONS                       #
// #############################################################################

void EvaluationParameters::randomize(double maxInit)
{ // Random init of values.

  static std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<double> dist(-maxInit, maxInit);

  // For each stage, and each piece on each square: randomize.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<13;j++)
      for (int k=0;k<BOARD_SQUARES;k++)
        psValues[i][j][k]=dist(rng);

  // For each stage, and each king distance: randomize.
  for (int i=0;i<NUM_STAGES;i++) {
    for (int j=0;j<12;j++) {
      kingDistanceOwn[i][j]=dist(rng);
      kingDistanceOther[i][j]=dist(rng);
    }
  }

  // For each 'singular' weight: randomize.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<NUM_WEIGHTS;j++)
      for (int k=0;k<2;k++)
        weights[i][j][k]=dist(rng);

} // End EvaluationParameters::randomize.

// -----------------------------------------------------------------------------

void EvaluationParameters::mutate(const double randomSwing)
{ // Alter each weight slightly, to make the eval set play differently.

  static std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<double> dist(-randomSwing, randomSwing);

  // For each stage, and each piece on each square: mutate.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<13;j++)
      for (int k=0;k<BOARD_SQUARES;k++)
        psValues[i][j][k]+=dist(rng);

  // For each stage, and each king distance: mutate.
  for (int i=0;i<NUM_STAGES;i++) {
    for (int j=0;j<12;j++) {
      kingDistanceOwn[i][j]+=dist(rng);
      kingDistanceOther[i][j]+=dist(rng);
    }
  }

  // For each 'singular' weight: mutate.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<NUM_WEIGHTS;j++)
      for (int k=0;k<2;k++)
        weights[i][j][k]+=dist(rng);

} // End EvaluationParameters::mutate.

// -----------------------------------------------------------------------------

bool EvaluationParameters::load(const char* fileName)
{ // Load the values.
  // Returns true, if failed.

  ifstream inFile;                               // The input file.

  // Open the file or return error if not successfull.
  inFile.open(fileName);
  if (inFile.fail())
    return true;

  // For each stage, and each piece on each square: load.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<13;j++)
      for (int k=0;k<BOARD_SQUARES;k++)
        inFile >> psValues[i][j][k];

  // For each stage, and each king distance: load.
  for (int i=0;i<NUM_STAGES;i++) {
    for (int j=0;j<12;j++) {
      inFile >> kingDistanceOwn[i][j];
      inFile >> kingDistanceOther[i][j];
    }
  }

  // For each 'singular' weight: load.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<NUM_WEIGHTS;j++)
      for (int k=0;k<2;k++)
        inFile >> weights[i][j][k];

  // Close the file.
  inFile.close();

  // All O.k.
  return false;

} // End EvaluationParameters::load.

// -----------------------------------------------------------------------------

bool EvaluationParameters::save(const char* fileName)
{ // Save the values.
  // Returns true, if failed.

  ofstream outFile;                               // The output file.

  // Open the file or return error if not successfull.
  outFile.open(fileName);
  if (outFile.fail())
    return true;

  // For each stage, and each piece on each square: save.
  for (int i=0;i<NUM_STAGES;i++) {
    for (int j=0;j<13;j++) {
      for (int k=0;k<BOARD_SQUARES;k++) {
        outFile << setprecision(32) << psValues[i][j][k]
                << ((k%8==7)?'\n':' ');
      }
      outFile << endl;
    }
    outFile << endl;
  }

  // For each stage, and each king distance: save.
  for (int i=0;i<NUM_STAGES;i++) {
    for (int j=0;j<12;j++) {
      outFile << kingDistanceOwn[i][j] << endl;
      outFile << kingDistanceOther[i][j] << endl;
      outFile << endl;
    }
    outFile << endl;
  }

  // For each 'singular' weight: Save.
  for (int i=0;i<NUM_STAGES;i++) {
    for (int j=0;j<NUM_WEIGHTS;j++) {
      for (int k=0;k<2;k++)
        outFile << weights[i][j][k] << endl;
      outFile << endl;
    }
    outFile << endl;
  }

  // Close the file.
  outFile.close();

  // All O.k.
  return false;

} // End EvaluationParameters::save.

// -----------------------------------------------------------------------------

void EvaluationParameters::normalize(void)
{ // Normalize the (piece-square) values.

  double mean;
  int numValues;

  // Normalize all (Mask) square based features.
  for (int i=0;i<NUM_STAGES;i++) {

    // Print the stage.
    if (i==0)
       cout << "*** OPENING - MEANS ***" << endl;
    else if (i==1)
       cout << "*** MIDDLE GAME - MEANS ***" << endl;
    else if (i==2)
       cout << "*** END GAME - MEANS ***" << endl;

    for (int j=0;j<13;j++) {

      // Init values for calculating mean.
      mean=0.0;
      numValues=0;

      // First find the mean (ignoring zero values).
      for (int k=0;k<BOARD_SQUARES;k++) {
        if (psValues[i][j][k]>EPSILON || psValues[i][j][k]<-EPSILON) {
          mean+=psValues[i][j][k];
          numValues++;
        }
      }

      // Now work out the true mean value for this set of masks.
      mean /= static_cast<double>(numValues);

      // Print the means for each, and it's relation to other pieces.
      if (j==0)
        cout << "White Pawn   = " << mean << " (" << 1.0/mean << " SF)" << endl;
      else if (j==1)
        cout << "White Knight = " << mean << endl;
      else if (j==2)
        cout << "White Bishop = " << mean << endl;
      else if (j==3)
        cout << "White Rook   = " << mean << endl;
      else if (j==4)
        cout << "White Queen  = " << mean << endl;
      else if (j==5)
        cout << "White King   = " << mean << endl;
      else if (j==6)
        cout << "Black Pawn   = " << mean << " (" << 1.0/mean << " SF)" << endl;
      else if (j==7)
        cout << "Black Knight = " << mean << endl;
      else if (j==8)
        cout << "Black Bishop = " << mean << endl;
      else if (j==9)
        cout << "Black Rook   = " << mean << endl;
      else if (j==10)
        cout << "Black Queen  = " << mean << endl;
      else if (j==11)
        cout << "Black King   = " << mean << endl;
      else if (j==12)
        cout << "Empty Square = " << mean << endl;

      // Now subtract the mean from each mask feature.
      for (int k=0;k<BOARD_SQUARES;k++)
        if (psValues[i][j][k]>EPSILON || psValues[i][j][k]<-EPSILON)
          psValues[i][j][k]-=mean;

    }

    // Print a blank line.
    cout << endl;

  }

} // End EvaluationParameters::normalize.

// -----------------------------------------------------------------------------

void EvaluationParameters::scale(double scaleFactor)
{ // Scale the values.

  // For each stage, and each piece on each square: scale.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<13;j++)
      for (int k=0;k<BOARD_SQUARES;k++)
        psValues[i][j][k]*=scaleFactor;

  // For each stage, and each king distance: scale.
  for (int i=0;i<NUM_STAGES;i++) {
    for (int j=0;j<12;j++) {
      kingDistanceOwn[i][j]*=scaleFactor;
      kingDistanceOther[i][j]*=scaleFactor;
    }
  }

  // For each 'singular' weight: Scale.
  for (int i=0;i<NUM_STAGES;i++)
    for (int j=0;j<NUM_WEIGHTS;j++)
      for (int k=0;k<2;k++)
        weights[i][j][k]*=scaleFactor;

} // End EvaluationParameters::scale.

// -----------------------------------------------------------------------------

double EvaluationParameters::train(double desiredOutput,double learningRate,
                                   double &output)
{ // Train the evaluation set, and return the output after training.
  // NOTE: Now uses the generalised delta-rule, to use a sigmoid activation.

  // First fire the network to find it's output.
  output=evalAndLearn(0.0);

  // Find the offset needed from the error (2003: Use derivative of bipol sig)..
  double error=(desiredOutput-activation(output));
  double offset=learningRate*error*gradient(output);

  // Alter the weights now.
  output=activation(evalAndLearn(offset));

  // Return the squared error.
  return error*error;

} // End EvaluationParameters::train.

// -----------------------------------------------------------------------------

inline double EvaluationParameters::evalPrecise(void)
{ // Get float eval.
  return evalAndLearn(0.0);                         // Just call with no offset.
} // End EvaluationParameters::evalPrecise.

// -----------------------------------------------------------------------------

int EvaluationParameters::eval(void)
{ // Get (scaled by PAWN_VALUE) Interger eval.
  // NOTE: This is the function to call from Search(), as it multiplies the
  //       small value up by the (internal) value of a pawn.
  //return (int)(evalAndLearn(0.0)*(double)PIECE_VALUE[PAWN]);
  //return (((int)(evalAndLearn(0.0)*(double)PIECE_VALUE[PAWN]))/250)*250;
  //return (((int)((evalAndLearn(0.0)/3.0)*(double)PIECE_VALUE[PAWN]))/200)*200;

  //return (((int)((evalAndLearn(0.0))*(double)PIECE_VALUE[PAWN]))/100)*100;
  return static_cast<int>(evalAndLearn(0.0) * static_cast<double>(PIECE_VALUE[PAWN]));
} // End EvaluationParameters::eval.

// #############################################################################
// #                     PRIVATE (CLASS) MEMBER FUNCTIONS                      #
// #############################################################################

int EvaluationParameters::getStage(void) noexcept
{ // This returns what stage the current position is (one of 3!).
  // clean_up15b: Now works on 3 stages, and only uses total *PIECES*.

  // The total number of pieces (not pawns/Kings) on board (Max=14) .
  int numPieces=0;

  // First find the total pieces (not pawns or kings) on the board.
  for (int i=0;i<BOARD_SQUARES;i++) {
    if (g_currentPiece[i]!=NONE && g_currentPiece[i]!=PAWN
        && g_currentPiece[i]!=KING)
      numPieces++;
  }

  // Are we in the opening?
  if (numPieces>MIDDLE_GAME_PIECES)
    return OPENING;

  // Are we in the middle game then?
  else if (numPieces>END_GAME_PIECES)
    return MIDDLE_GAME;

  // Must be in the ending then.
  else
    return END_GAME;

} // End EvaluationParameters::getStage.

// -----------------------------------------------------------------------------

inline double EvaluationParameters::activation(double value) noexcept
{ // Get bipolar-sigmoid activation value.
  if (useLinearTraining)
    return value;
  else
    return (1.0-exp(-value))/(1.0+exp(-value));
} // End EvaluationParameters::activation.

// -----------------------------------------------------------------------------

inline double EvaluationParameters::gradient([[maybe_unused]] double value) noexcept
{ // Get bipolar-sigmoid gradient value.
  if (useLinearTraining)
    return 1.0;
  else
    return (2.0*exp(-value))/((1.0+exp(-value))*(1.0+exp(-value)));
} // End EvaluationParameters::gradient.

// =============================================================================

// Note: Weight access functions have been converted to inline methods in the
// EvaluationParameters class. See evaluation.h for addWeight(), addWeightScaled(),
// addWeightSingular(), addWeightSingularScaled(), and flipIfNeeded().

double EvaluationParameters::evalAndLearn(double offsetValue)
{ // Eval and/or update weights at the same time.

  // This is the score to be returned for the position.
  double score=0.0; 

  // First find what stage we are on (Save this outside, so function can see).
  stage=getStage();

  // Save the offsetValue locally, so other function can see.
  offset=offsetValue;

  // See if we are looking from white or black perspective and decide to flip.
  bool flipBoard=(g_currentSide==WHITE?false:true);

  // 1st PASS: Set up pawnCount and pawnRank + Minor peice flags.
  if (!useSuperFastEval) {
    for (int i=0;i<10;i++) {
      pawnCount[WHITE][i]=0;
      pawnCount[BLACK][i]=0;
      pawnRank[WHITE][i]=-1; 
      pawnRank[BLACK][i]=8;
    }
    for (int i=0;i<2;i++) {
      hasKnights[i]=false;
      hasWhiteSquareBishop[i]=false;
      hasBlackSquareBishop[i]=false;
    }
    for (int i=0;i<64;i++) {

      // Setup the pawnCount and pawnRank arrays.
      if (g_currentPiece[i]==PAWN) {
        pawnCount[g_currentColour[i]][getFile(i)+1]++;
        if (g_currentColour[i]==WHITE) {
          if (pawnRank[WHITE][getFile(i)+1]<getRank(i))
            pawnRank[WHITE][getFile(i)+1]=getRank(i);
        }
        else {
          if (pawnRank[BLACK][getFile(i)+1]>getRank(i))
            pawnRank[BLACK][getFile(i)+1]=getRank(i);
        }
      }

      // Setup the Minor Piece flags (forepost checks, bishop avoidance, etc).
      else if (g_currentPiece[i]==KNIGHT) {
        hasKnights[g_currentColour[i]]=true;
      }
      else if (g_currentPiece[i]==BISHOP) {

        // Find out if bihops is on black or white square.
        if ((i/8)%2==0) {
          if (i%2==0)
            hasWhiteSquareBishop[g_currentColour[i]]=true;
          else
            hasBlackSquareBishop[g_currentColour[i]]=true;
        }
        else {
          if (i%2==0)
            hasBlackSquareBishop[g_currentColour[i]]=true;
          else
            hasWhiteSquareBishop[g_currentColour[i]]=true;
        }

      }

    } // End 1st PASS.
  }

  // 2nd PASS: For each square, activate the corresponding feature.
  for (int i=0;i<BOARD_SQUARES;i++) {

    // Is it one of our peices?
    if (g_currentColour[i]==g_currentSide) {

      // Add the piece-square score.
      score+=addWeight(psValues[stage][g_currentPiece[i]][flipIfNeeded(flipBoard,i)]);

      // Add the king-distance scores.
      if (useKingDistanceFeatures) {
        score+=addWeightScaled(kingDistanceOwn[stage][g_currentPiece[i]],
                        getDistanceToOwnKingManhattan(i));
        score+=addWeightScaled(kingDistanceOther[stage][g_currentPiece[i]],
                        getDistanceToOtherKingManhattan(i));
      }

    }

    // Is it our opponents peice?
    else if (g_currentColour[i]==getOtherSide(g_currentSide)) {

      // Add the piece-square score.
      score+=addWeight(psValues[stage][g_currentPiece[i]+6][flipIfNeeded(flipBoard,i)]);

      // Add the king-distance scores.
      if (useKingDistanceFeatures) {
        score+=addWeightScaled(kingDistanceOwn[stage][g_currentPiece[i]+6],
                        getDistanceToOwnKingManhattan(i));
        score+=addWeightScaled(kingDistanceOther[stage][g_currentPiece[i]+6],
                        getDistanceToOtherKingManhattan(i));
      }

    }

    // Must be an empty square then.
    else if (useEmptySquareFeatures) {

      // Add the piece-square score.
      score+=addWeight(psValues[stage][12][flipIfNeeded(flipBoard,i)]);

    }

    // Call the function for the peice to evaluate it.
    if (!useSuperFastEval) {
      if (g_currentPiece[i]==PAWN)
        score+=evalPawn(i);
      else if (g_currentPiece[i]==KNIGHT)
        score+=evalKnight(i);
      else if (g_currentPiece[i]==BISHOP)
        score+=evalBishop(i);
      else if (g_currentPiece[i]==ROOK)
        score+=evalRook(i);
      else if (g_currentPiece[i]==QUEEN)
        score+=evalQueen(i);
      else if (g_currentPiece[i]==KING)
        score+=evalKing(i);
    }

  } // End for each square.

  // Return the score.
  return score;

} // End EvaluationParameters::evalAndLearn.

// -----------------------------------------------------------------------------

double EvaluationParameters::evalPawn(int square)
{ // Eval the pawn at square.

  // This is the score to be returned for the knight.
  double score=0.0;

  // Is this feature for our side or the opponets ([0]=us, [1]=Opponent)?
  int sideIndex=(g_currentColour[square]==g_currentSide?0:1);

  int file=getFile(square)+1;    // The pawn's file.
  int protectedBy=0;              // How may freindly pawns protect us (0/1/2),
  int side=g_currentColour[square]; // The side we are on for this piece.

  // If there's a pawn behind this one, it's doubled.
  // Also if there are 3 or more pawns of a colour on a file the value is
  // multplied N-1 times (ie a trippled pawn is twice as bad!).
  if (side==WHITE) {
    if (pawnRank[side][file]>getRank(square))
      score+=addWeightSingular(weights[stage][DOUBLED_PAWN],sideIndex);
  }
  else {
    if (pawnRank[side][file]<getRank(square))
      score+=addWeightSingular(weights[stage][DOUBLED_PAWN],sideIndex);
  }

  // If there aren't any friendly pawns on either side of this one, it's
  // isolated.
  if ((!pawnCount[side][file-1]) && (!pawnCount[side][file+1]))
    score+=addWeightSingular(weights[stage][ISOLATED_PAWN],sideIndex);

  // If it's not isolated, it might be backwards.
  // JUK: Added the fact that a backward pawn must have no hostile pawn
  //      on its file. A semi-BacKward pawn is one with a hostile pawn
  //      on its file.
  // clean_up6: Fixed bug where a pawn was backward on it's first rank,
  //            if it's neighbour had only moved 1 square - now it's 2 squares!
  else if (side==WHITE) {
    if ((getRank(square)==6 && pawnRank[side][file-1]<(getRank(square)-1)
         && pawnRank[side][file+1]<(getRank(square)-1))
        || (getRank(square)<6 && pawnRank[side][file-1]<getRank(square)
            && pawnRank[side][file+1]<getRank(square))) {

      // See if it's backward or semi-backward.
      if (pawnCount[BLACK][file]==0)
        score+=addWeightSingular(weights[stage][BACKWARD_PAWN],sideIndex);
      else
        score+=addWeightSingular(weights[stage][SEMI_BACKWARD_PAWN],sideIndex);

      // See if the backward Pawn's square in front is attacked by pawn(s).
      if (getFile(square)>0 && getRank(square)>2
          && g_currentPiece[square-17]==PAWN
          && g_currentColour[square-17]==getOtherSide(side)) {
        score+=addWeightSingular(weights[stage][BACKWARD_ATTACK],sideIndex);
      }
      if (getFile(square)<7 && getRank(square)>2
          && g_currentPiece[square-15]==PAWN
          && g_currentColour[square-15]==getOtherSide(side)) {
        score+=addWeightSingular(weights[stage][BACKWARD_ATTACK],sideIndex);
      }

    }
  }
  else {
    if ((getRank(square)==1 && pawnRank[side][file-1]>(getRank(square)+1)
         && pawnRank[side][file+1]>(getRank(square)+1))
        || (getRank(square)>1 && pawnRank[side][file-1]>getRank(square)
        && pawnRank[side][file+1]>getRank(square))) {

      // See if it's backward or semi-backward.
      if (pawnCount[WHITE][file]==0)
        score+=addWeightSingular(weights[stage][BACKWARD_PAWN],sideIndex);
      else
        score+=addWeightSingular(weights[stage][SEMI_BACKWARD_PAWN],sideIndex);

      // See if the backward Pawn's square in front is attacked by pawn(s).
      if (getFile(square)>0 && getRank(square)<5
          && g_currentPiece[square+15]==PAWN
          && g_currentColour[square+15]==getOtherSide(side)) {
        score+=addWeightSingular(weights[stage][BACKWARD_ATTACK],sideIndex);
      }
      if (getFile(square)<7 && getRank(square)<5
          && g_currentPiece[square+17]==PAWN
          && g_currentColour[square+17]==getOtherSide(side)) {
        score+=addWeightSingular(weights[stage][BACKWARD_ATTACK],sideIndex);
      }

    }
  }

  // See if the pawn if adjacent to other pawns (left or right).
  if (getFile(square)>0
      && g_currentPiece[square-1]==PAWN && g_currentColour[square-1]==side) {
    score+=addWeightSingular(weights[stage][ADJACENT_PAWNS],sideIndex);
  }
  if (getFile(square)<7
      && g_currentPiece[square+1]==PAWN && g_currentColour[square+1]==side) {
     score+=addWeightSingular(weights[stage][ADJACENT_PAWNS],sideIndex);
  }

  // See if the pawn if protected (ie: Chained) by other pawns (SE,SW).
  if (getFile(square)>0) {
    if (side==WHITE) {
      if (g_currentPiece[square+7]==PAWN && g_currentColour[square+7]==side) {
        score+=addWeightSingular(weights[stage][PAWN_CHAIN],sideIndex);
        protectedBy++;
      }
    }
    else {
      if (g_currentPiece[square-9]==PAWN && g_currentColour[square-9]==side) {
        score+=addWeightSingular(weights[stage][PAWN_CHAIN],sideIndex);
        protectedBy++;
      }
    }
  }
  if (getFile(square)<7) {
    if (side==WHITE) {
      if (g_currentPiece[square+9]==PAWN && g_currentColour[square+9]==side) {
        score+=addWeightSingular(weights[stage][PAWN_CHAIN],sideIndex);
        protectedBy++;
      }
    }
    else {
      if (g_currentPiece[square-7]==PAWN && g_currentColour[square-7]==side) {
        score+=addWeightSingular(weights[stage][PAWN_CHAIN],sideIndex);
        protectedBy++;
      }
    }
  }

  // Past pawns.
  // clean_up6: Fixed another bug here, where the portected bit was being
  //            executed even if the pawn wasn't past!!!
  if (side==WHITE && pawnRank[1-side][file-1]>=getRank(square)
      && pawnRank[1-side][file]>=getRank(square)
      && pawnRank[1-side][file+1]>=getRank(square)) {

    // Multiply by how far up we are.
    score+=addWeightSingularScaled(weights[stage][PASSED_PAWN],sideIndex,(7-getRank(square)));

    // See if it's protected (from earlier).
    if (protectedBy>0)
      score+=addWeightSingularScaled(weights[stage][PROT_PASSED_PAWN],sideIndex,(protectedBy));

    // See if it's blocked.
    if (g_currentPiece[square-8]!=NONE)
      score+=addWeightSingular(weights[stage][BLOCKED_PASSED_PAWN],sideIndex);

  }
  else if (side==BLACK && pawnRank[1-side][file-1]<=getRank(square)
           && pawnRank[1-side][file]<=getRank(square)
           && pawnRank[1-side][file+1]<=getRank(square)) {

    // Multiply by how far up we are.
    score+=addWeightSingularScaled(weights[stage][PASSED_PAWN],sideIndex,getRank(square));

    // See if it's protected (from earlier).
    if (protectedBy>0)
      score+=addWeightSingularScaled(weights[stage][PROT_PASSED_PAWN],sideIndex,(protectedBy));

    // See if it's blocked.
    if (g_currentPiece[square+8]!=NONE)
      score+=addWeightSingular(weights[stage][BLOCKED_PASSED_PAWN],sideIndex);

  }

  // Finally see if the pawn is in a local (ie: 3 window) majority.
  if ((pawnCount[side][file-1]+pawnCount[side][file]+pawnCount[side][file+1])
      >(pawnCount[1-side][file-1]+pawnCount[1-side][file]
        +pawnCount[1-side][file+1])) {
    score+=addWeightSingular(weights[stage][PAWN_MAJORITY],sideIndex);
  }

  return score;

} // End EvaluationParameters::evalPawn.

// -----------------------------------------------------------------------------

double EvaluationParameters::evalKnight(int square)
{ // Eval the knight at square.

  // This is the score to be returned for the knight.
  double score=0.0;

  // First see if we are looking at a black or white peice, and decide to flip.
  bool flipBoard=(g_currentColour[square]==WHITE?false:true);

  // Is this feature for our side or the opponets ([0]=us, [1]=Opponent)?
  int sideIndex=(g_currentColour[square]==g_currentSide?0:1);

  // Test to see if it's a f3/c3 knight with pawn above it and not below.
  if (flipIfNeeded(flipBoard,square)==C3
      && g_currentPiece[flipIfNeeded(flipBoard,C4)]==PAWN
      && g_currentColour[flipIfNeeded(flipBoard,C4)]==g_currentColour[square]
      && !(g_currentPiece[flipIfNeeded(flipBoard,C2)]==PAWN
           && g_currentColour[flipIfNeeded(flipBoard,C2)]==g_currentColour[square])) {
    score+=addWeightSingular(weights[stage][NO_BLOCK_KNIGHT],sideIndex);
  }
  else if (flipIfNeeded(flipBoard,square)==F3
           && g_currentPiece[flipIfNeeded(flipBoard,F4)]==PAWN
           && g_currentColour[flipIfNeeded(flipBoard,F4)]==g_currentColour[square]
           && !(g_currentPiece[flipIfNeeded(flipBoard,F2)]==PAWN
                && g_currentColour[flipIfNeeded(flipBoard,F2)]==g_currentColour[square])) {
    score+=addWeightSingular(weights[stage][NO_BLOCK_KNIGHT],sideIndex);
  }

  // Add forepost bonuses (if it is on a forepost).
  score+=forepostBonus(square);

  return score;

} // End EvaluationParameters::evalKnight.

// -----------------------------------------------------------------------------

double EvaluationParameters::evalBishop(int square)
{ // Eval the bishiop at square.

  // This is the score to be returned for the rook.
  double score=0.0;

  // First see if we are looking at a black or white peice, and decide to flip.
  bool flipBoard=(g_currentColour[square]==WHITE?false:true);

  // Is this feature for our side or the opponets ([0]=us, [1]=Opponent)?
  int sideIndex=(g_currentColour[square]==g_currentSide?0:1);

  // Test to see if it's a fienchetto.
  // NOTE: Must have pawn to edge and above...
  if (flipIfNeeded(flipBoard,square)==B2
      && g_currentPiece[flipIfNeeded(flipBoard,B3)]==PAWN
      && g_currentColour[flipIfNeeded(flipBoard,B3)]==g_currentColour[square]
      && g_currentPiece[flipIfNeeded(flipBoard,A2)]==PAWN
      && g_currentColour[flipIfNeeded(flipBoard,A2)]==g_currentColour[square]) {
    score+=addWeightSingular(weights[stage][FIENCHETTO],sideIndex);
  }
  else if (flipIfNeeded(flipBoard,square)==G2
           && g_currentPiece[flipIfNeeded(flipBoard,G3)]==PAWN
           && g_currentColour[flipIfNeeded(flipBoard,G3)]==g_currentColour[square]
           && g_currentPiece[flipIfNeeded(flipBoard,H2)]==PAWN
           && g_currentColour[flipIfNeeded(flipBoard,H2)]==g_currentColour[square]) {
    score+=addWeightSingular(weights[stage][FIENCHETTO],sideIndex);
  }

  // Add forepost bonuses (if it is on a forepost).
  score+=forepostBonus(square);

  return score;

} // End EvaluationParameters::evalBishop.

// -----------------------------------------------------------------------------

double EvaluationParameters::evalRook(int square)
{ // Eval the rook at square.

  // This is the score to be returned for the rook.
  double score=0.0;

  // First see if we are looking at a black or white peice, and decide to flip.
  bool flipBoard=(g_currentColour[square]==WHITE?false:true);

  // Is this feature for our side or the opponets ([0]=us, [1]=Opponent)?
  int sideIndex=(g_currentColour[square]==g_currentSide?0:1);

  // Give bonus for not moving before the king has.
  if (getFile(g_currentState->kingSquare[g_currentColour[square]])==4) {
    if (flipIfNeeded(flipBoard,square)==A1)
      score+=addWeightSingular(weights[stage][ROOK_NO_MOVE],sideIndex);
    else if (flipIfNeeded(flipBoard,square)==H1)
      score+=addWeightSingular(weights[stage][ROOK_NO_MOVE],sideIndex);
  }

  // Test for Rook being on a semi-open or open file.
  if (pawnCount[g_currentColour[square]][getFile(square)+1]==0) {
    if (pawnCount[1-g_currentColour[square]][getFile(square)+1]==0)
      score+=addWeightSingular(weights[stage][ROOK_OPEN_FILE],sideIndex);
    else
      score+=addWeightSingular(weights[stage][ROOK_SEMI_OPEN_FILE],sideIndex);
  }

  // Find out if there is another rook(s) or queen(s) on this file (Battery(s)).
  // NOTE: This is done by checking if there is another one *below* this one,
  //       as this means the feature is only activated once.
  // NOTE: It not matter this way if we are white or black!
  for (int i=getRank(square)+8;i<64;i+=8) {
    if (g_currentColour[i]==g_currentColour[square]
        && (g_currentPiece[i]==ROOK || g_currentPiece[i]==QUEEN)) {
      score+=addWeightSingular(weights[stage][BATTERY_BONUS],sideIndex);
      break;  // Break so that the one behind can test for a 3rd one of file.
    }
  }

  return score;

} // End EvaluationParameters::evalRook.

// -----------------------------------------------------------------------------

double EvaluationParameters::evalQueen(int square)
{ // Eval the queen at square.

  // This is the score to be returned for the queen.
  double score=0.0;

  // Is this feature for our side or the opponets ([0]=us, [1]=Opponent)?
  int sideIndex=(g_currentColour[square]==g_currentSide?0:1);

  // Test for Queen being on a semi-open or open file.
  if (pawnCount[g_currentColour[square]][getFile(square)+1]==0) {
    if (pawnCount[1-g_currentColour[square]][getFile(square)+1]==0)
      score+=addWeightSingular(weights[stage][QUEEN_OPEN_FILE],sideIndex);
    else
      score+=addWeightSingular(weights[stage][QUEEN_SEMI_OPEN_FILE],sideIndex);
  }

  // Find out if there is another rook(s) or queen(s) on this file (Battery(s)).
  // NOTE: This is done by checking if there is another one *below* this one,
  //       as this means the feature is only activated once.
  // NOTE: It not matter this way if we are white or black!
  for (int i=getRank(square)+8;i<64;i+=8) {
    if (g_currentColour[i]==g_currentColour[square]
        && (g_currentPiece[i]==ROOK || g_currentPiece[i]==QUEEN)) {
      score+=addWeightSingular(weights[stage][BATTERY_BONUS],sideIndex);
      break;  // Break so that the one behind can test for a 3rd one of file.
    }
  }

  return score;

} // End EvaluationParameters::evalQueen.

// -----------------------------------------------------------------------------

double EvaluationParameters::evalKing(int square)
{ // Eval the king at square.

  // This is the score to be returned for the king.
  double score=0.0;

  // First see if we are looking at a black or white peice, and decide to flip.
  bool flipBoard=(g_currentColour[square]==WHITE?false:true);

  // Is this feature for our side or the opponets ([0]=us, [1]=Opponent)?
  int sideIndex=(g_currentColour[square]==g_currentSide?0:1);

  // Now check for a pawn storm, infront of castled king.
  if (flipIfNeeded(flipBoard,square)==A1 || flipIfNeeded(flipBoard,square)==B1 || flipIfNeeded(flipBoard,square)==C1
      || flipIfNeeded(flipBoard,square)==A2 || flipIfNeeded(flipBoard,square)==B2 || flipIfNeeded(flipBoard,square)==C2) {
    if (g_currentColour[flipIfNeeded(flipBoard,A2)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,A2)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,B2)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,B2)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,C2)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,C2)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,A3)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,A3)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,B3)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,B3)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,C3)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,C3)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }

  }
  else if (flipIfNeeded(flipBoard,square)==F1 || flipIfNeeded(flipBoard,square)==G1 || flipIfNeeded(flipBoard,square)==H1
           || flipIfNeeded(flipBoard,square)==F2 || flipIfNeeded(flipBoard,square)==G2 || flipIfNeeded(flipBoard,square)==H2) {
    if (g_currentColour[flipIfNeeded(flipBoard,F2)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,F2)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,G2)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,G2)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,H2)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,H2)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,F3)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,F3)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,G3)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,G3)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }
    if (g_currentColour[flipIfNeeded(flipBoard,H3)]==(1-g_currentColour[square])
        && g_currentPiece[flipIfNeeded(flipBoard,H3)]==PAWN) {
      score+=addWeightSingular(weights[stage][PAWN_STORM],sideIndex);
    }

  } // End Test for pawn storm.

  // Test for casting...
  // Test for King on a1/b1/c1 or f1/g1/h1 + no rook on a1/a2 or h1/h2.
  if ((flipIfNeeded(flipBoard,square)==A1
       || (flipIfNeeded(flipBoard,square)==B1
           && !(g_currentPiece[flipIfNeeded(flipBoard,A1)]==ROOK
                && g_currentColour[flipIfNeeded(flipBoard,A1)]==g_currentColour[square]))
       || (flipIfNeeded(flipBoard,square)==C1
          && !(g_currentPiece[flipIfNeeded(flipBoard,A1)]==ROOK
                && g_currentColour[flipIfNeeded(flipBoard,A1)]==g_currentColour[square])
           && !(g_currentPiece[flipIfNeeded(flipBoard,B1)]==ROOK
                && g_currentColour[flipIfNeeded(flipBoard,B1)]==g_currentColour[square])))) {

    // Give the bonus for castling then.
    score+=addWeightSingular(weights[stage][CASTLE_BONUS],sideIndex);

    // Now check for a good pawn defence, infront of castled king.
    if (!(g_currentPiece[flipIfNeeded(flipBoard,A2)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,A2)]==g_currentColour[square])) {
      score+=addWeightSingular(weights[stage][CASTLE_MISSING_PAWN],sideIndex);
    }
    if (!(g_currentPiece[flipIfNeeded(flipBoard,B2)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,B2)]==g_currentColour[square])) {

      // Check first to see if we have a fiencheto defence.
      if (g_currentPiece[flipIfNeeded(flipBoard,B3)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,B3)]==g_currentColour[square]
          && g_currentPiece[flipIfNeeded(flipBoard,B2)]==BISHOP
          && g_currentColour[flipIfNeeded(flipBoard,B2)]==g_currentColour[square]) {
        score+=addWeightSingular(weights[stage][CASTLE_FIENCHETTO],sideIndex);
      }
      else {
        score+=addWeightSingular(weights[stage][CASTLE_MISSING_PAWN],sideIndex);
      }

    }
    if (!(g_currentPiece[flipIfNeeded(flipBoard,C2)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,C2)]==g_currentColour[square])) {
      score+=addWeightSingular(weights[stage][CASTLE_MISSING_PAWN],sideIndex);
    }

    // Do we have a propective knight at C3?
    if (g_currentPiece[flipIfNeeded(flipBoard,C3)]==KNIGHT
        && g_currentColour[flipIfNeeded(flipBoard,C3)]==g_currentColour[square]) {
      score+=addWeightSingular(weights[stage][CASTLE_KNIGHT_PROT],sideIndex);
    }

  }
  else if ((flipIfNeeded(flipBoard,square)==H1
           || (flipIfNeeded(flipBoard,square)==G1
               && !(g_currentPiece[flipIfNeeded(flipBoard,H1)]==ROOK
                    && g_currentColour[flipIfNeeded(flipBoard,H1)]==g_currentColour[square]))
           || (flipIfNeeded(flipBoard,square)==F1
               && !(g_currentPiece[flipIfNeeded(flipBoard,H1)]==ROOK
                    && g_currentColour[flipIfNeeded(flipBoard,H1)]==g_currentColour[square])
               && !(g_currentPiece[flipIfNeeded(flipBoard,G1)]==ROOK
                    && g_currentColour[flipIfNeeded(flipBoard,G1)]==g_currentColour[square])))) {

    // Give the bonus for castling then.
    score+=addWeightSingular(weights[stage][CASTLE_BONUS],sideIndex);

    // Now check for a good pawn defence, infront of castled king.
    if (!(g_currentPiece[flipIfNeeded(flipBoard,F2)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,F2)]==g_currentColour[square])) {
      score+=addWeightSingular(weights[stage][CASTLE_MISSING_PAWN],sideIndex);
    }
    if (!(g_currentPiece[flipIfNeeded(flipBoard,G2)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,G2)]==g_currentColour[square])) {

      // Check first to see if we have a fiencheto defence.
      if (g_currentPiece[flipIfNeeded(flipBoard,G3)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,G3)]==g_currentColour[square]
          && g_currentPiece[flipIfNeeded(flipBoard,G2)]==BISHOP
          && g_currentColour[flipIfNeeded(flipBoard,G2)]==g_currentColour[square]) {
        score+=addWeightSingular(weights[stage][CASTLE_FIENCHETTO],sideIndex);
      }
      else {
        score+=addWeightSingular(weights[stage][CASTLE_MISSING_PAWN],sideIndex);
      }

    }
    if (!(g_currentPiece[flipIfNeeded(flipBoard,H2)]==PAWN
          && g_currentColour[flipIfNeeded(flipBoard,H2)]==g_currentColour[square])) {
      score+=addWeightSingular(weights[stage][CASTLE_MISSING_PAWN],sideIndex);
    }

    // Do we have a propective knight at F3?
    if (g_currentPiece[flipIfNeeded(flipBoard,F3)]==KNIGHT
        && g_currentColour[flipIfNeeded(flipBoard,F3)]==g_currentColour[square]) {
      score+=addWeightSingular(weights[stage][CASTLE_KNIGHT_PROT],sideIndex);
    }

  }

  // Test for King being on a semi-open or open file.
  if (pawnCount[g_currentColour[square]][getFile(square)+1]==0) {
    if (pawnCount[1-g_currentColour[square]][getFile(square)+1]==0)
      score+=addWeightSingular(weights[stage][KING_OPEN_FILE],sideIndex);
    else
      score+=addWeightSingular(weights[stage][KING_SEMI_OPEN_FILE],sideIndex);
  }

  // Test for the king having semi-open or open files to its left and right.
  if (getFile(square)>0
      && pawnCount[g_currentColour[square]][getFile(square)]==0) {
    if (pawnCount[1-g_currentColour[square]][getFile(square)]==0)
      score+=addWeightSingular(weights[stage][KING_OPEN_FILE_SIDE],sideIndex);
    else
      score+=addWeightSingular(weights[stage][KING_SEMI_OPEN_FILE_SIDE],sideIndex);
  }
  if (getFile(square)<7
      && pawnCount[g_currentColour[square]][getFile(square)+2]==0) {
    if (pawnCount[1-g_currentColour[square]][getFile(square)+2]==0)
      score+=addWeightSingular(weights[stage][KING_OPEN_FILE_SIDE],sideIndex);
    else
      score+=addWeightSingular(weights[stage][KING_SEMI_OPEN_FILE_SIDE],sideIndex);
  }

  // Find out if there is an opposing rook(s) or queen(s) on file/rank.
  for (int i=0;i<64;i++) {
    if (getFile(i)==getFile(square) || getRank(i)==getRank(square)) {
      if (g_currentPiece[i]==ROOK && g_currentColour[i]==(1-g_currentColour[square]))
        score+=addWeightSingular(weights[stage][KING_ROOK_XRAY],sideIndex);
      if (g_currentPiece[i]==QUEEN && g_currentColour[i]==(1-g_currentColour[square]))
        score+=addWeightSingular(weights[stage][KING_QUEEN_XRAY],sideIndex);
    }
  }

  // Return the score.
  return score;

} // End EvaluationParameters::evalKing.

// =============================================================================

double EvaluationParameters::forepostBonus(int square)
{ // This function checks if the piece on the square is on a forepost, and
  // returns any bonuses for it (ie: basic, absolute, protected, ...).
  // NOTE: It would be better later on to have the protection test not use
  //       the pawnRank arrays!!!

  // This is the score to be returned for the forepost (if any).
  double score=0.0;

  // Get the file the peice is on.
  int file=getFile(square)+1;     // Add 1 because of the extra file in array.

  // Is this feature for our side or the opponets ([0]=us, [1]=Opponent)?
  int sideIndex=(g_currentColour[square]==g_currentSide?0:1);

  // If it is a Knight use the feature index, if its a bishop add 1 to it.
  int pieceTypeOffset=(g_currentPiece[square]==KNIGHT?0:1);

  // Depends on which side we are.
  if (g_currentColour[square]==WHITE) {

    // Check for forepost (Can't have a forepost on back rank!).
    if (getRank(square)>0
        && (pawnRank[BLACK][file-1]>=getRank(square))
        && (pawnRank[BLACK][file+1]>=getRank(square))) {

      // Add the forepost bonus.
      score+=addWeightSingular(weights[stage][FOREPOST_BONUS+pieceTypeOffset],sideIndex);

      // See how many pawns protect the forepost.
      // Would be better not to use pawn rank table!!!
      if (pawnRank[WHITE][file-1]==(getRank(square)+1))
        score+=addWeightSingular(weights[stage][PROTECTED_FOREPOST+pieceTypeOffset],sideIndex);
      if (pawnRank[WHITE][file+1]==(getRank(square)+1))
        score+=addWeightSingular(weights[stage][PROTECTED_FOREPOST+pieceTypeOffset],sideIndex);

      // Is there an enemy pawn infront of this square.
      if (g_currentPiece[square-8]==PAWN
          && g_currentColour[square-8]==BLACK) {
        score+=addWeightSingular(weights[stage][PAWN_INFRONT_FOREPOST+pieceTypeOffset],sideIndex);
      }

    }

  } // End check for white forepost.

  else {

    // Check for forepost (Can't have a forepost on back rank!).
    if (getRank(square)<7
        && (pawnRank[WHITE][file-1]<=getRank(square))
        && (pawnRank[WHITE][file+1]<=getRank(square))) {

      // Add the forepost bonus.
      score+=addWeightSingular(weights[stage][FOREPOST_BONUS+pieceTypeOffset],sideIndex);

      // See how many pawns protect the forepost.
      // Would be better not to use pawn rank table!!!
      if (pawnRank[BLACK][file-1]==getRank(square)-1)
        score+=addWeightSingular(weights[stage][PROTECTED_FOREPOST+pieceTypeOffset],sideIndex);
      if (pawnRank[BLACK][file+1]==getRank(square)-1)
        score+=addWeightSingular(weights[stage][PROTECTED_FOREPOST+pieceTypeOffset],sideIndex);

      // Is there an enemy pawn infront of this square.
      if (g_currentPiece[square+8]==PAWN
          && g_currentColour[square+8]==WHITE) {
        score+=addWeightSingular(weights[stage][PAWN_INFRONT_FOREPOST+pieceTypeOffset],sideIndex);
      }

    }

  } // End check for black forepost.

  // Test if the minor peice can actuallt be attacked by opponents minor peice
  // to find out if it is absolute.
  if (hasKnights[1-g_currentColour[square]]==false) {
    if ((square/8)%2==0) {
      if (square%2==0 && hasWhiteSquareBishop[1-g_currentColour[square]]==false) {
        score+=addWeightSingular(weights[stage][ABSOLUTE_FOREPOST+pieceTypeOffset],sideIndex);
      }
      else if (square%2==1
               && hasBlackSquareBishop[1-g_currentColour[square]]==false) {
        score+=addWeightSingular(weights[stage][ABSOLUTE_FOREPOST+pieceTypeOffset],sideIndex);
      }
    }
    else {
      if (square%2==0 && hasBlackSquareBishop[1-g_currentColour[square]]==false) {
        score+=addWeightSingular(weights[stage][ABSOLUTE_FOREPOST+pieceTypeOffset],sideIndex);
      }
      else if (square%2==1
               && hasWhiteSquareBishop[1-g_currentColour[square]]==false) {
        score+=addWeightSingular(weights[stage][ABSOLUTE_FOREPOST+pieceTypeOffset],sideIndex);
      }
    }
  }

  // Return the score.
  return score;

} // End forepostBonus.

// #############################################################################
// #############################################################################
// #############################################################################

// GLOBAL FUNCTION PROTOTYPES:

int basicMaterialEval(void) noexcept
{ // Returns basic material evaluation.
  // NOTE: In search, use the macros that use the running totals.

  int retVal=0;

  // Go round all squares.
  for (int i=0;i<BOARD_SQUARES;i++) {
    if (g_currentColour[i]==g_currentSide)
      retVal+=PIECE_VALUE[g_currentPiece[i]];
    else if (g_currentColour[i]==getOtherSide(g_currentSide))
      retVal-=PIECE_VALUE[g_currentPiece[i]];
  }

  return retVal;

} // End BasicMaterialEval.

// -----------------------------------------------------------------------------

bool materialExactlyEven(void) noexcept
{ // Returns true if both sides have exactly the same material.
  // eg: Same no. of Queens, Rooks, Bishops, Knights and pawns...

  // This holds how many of each piece each side has.
  std::array<std::array<int, 6>, 2> numPieces = {};

  // Go round all squares, counting how mnay of each piece, each side has.
  for (int i=0;i<64;i++)
    if (g_currentColour[i]!=NONE)
      numPieces[g_currentColour[i]][g_currentPiece[i]]++;

  // See if they have the same.
  for (int i=0;i<6;i++)
    if (numPieces[0][i]!=numPieces[1][i])
      return false;                     // Not got the same amount of this...

  // If we get here then the position is exactly materially even.
  return true;

} // End MaterialExactlyEven.
