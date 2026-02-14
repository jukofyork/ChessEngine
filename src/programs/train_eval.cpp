// 2003: This is a much more advanced version of the old program:
//       It saves the value of all the important varaiables to an extra file
//       called <EvalSet>.vars, and this means that it can be stopped and
//       started *WITHOUT* losing any of the work it has done!!!
//       i have added in an extra signal handler for sigINT, so that it can
//       either be used with 'train_over_time' or used in a console and stopped
//       with cntr-C (Semiphore used, so 100% safe).
//       NOTE: Make sure you use *exactly* the same database each time you
//             continue the training (not checked for and file pos saved...).
//       NOTE: Now uses the binary version of the database...
//       * Now reads the extra '\0' char from the end of a move list in the DB.
//       * Now saves state and eval set only every SAVE_EVERY games.
//         - This was wasting *alot* of cpu time!!!

// WE ARE TRAINING, SO USE SLOW EVAL.
#define TRAINING

// Include headers only - implementations linked separately
#include "../chess_engine/chess_engine.h"
#include "../search_engine/search_engine.h"
#include <array>
#include <cmath>
#include "../interface/interface.h"
#include "../core/cli_parser.h"

// For safely handleing signals.
#include <signal.h>  // For catching SIGTERM/SIGINT.

#include <iomanip>                      // So we can set the precision.

using namespace std;

// For TD training.
constexpr double LEARNING_RATE = 0.00000001;    //@@(0.000001)//@(0.0005) //(0.001)//(0.001) //*03*(0.00001) // Learning Rate (0.0005 good?).
constexpr double LR_REDUCTION = 1.0;            //@0.75 //0.995 //1.0     // Reduce learning rate by factor N each iter.
constexpr double MAGNIFY = 1.0;                 // How much to maginfy learning to state T.
constexpr double DISCOUNT = 1.0;                //*03*1.0     // Discounting factor.
constexpr double START_LAMBDA = 1.0;            // If TD_BOTH, 0=TD0 / 1=TD1, else proporion.
constexpr double LAMBDA_REDUCTION = 1.0;        // Each turn reduce lambda by N.

// For NN.
constexpr double MAX_INIT = 0.0;                //0.0001  // Max int +/- for weights.
constexpr double NN_TARGET = 1.5;               //@@1.0  //*03*0.95    // So we can set to less than +/- 1.0 for w/l.

// Set this to how often you want to save the eval set+vars.
constexpr int SAVE_EVERY = 100000;              // In games.

// *****************************************************************************

bool g_saving=false;               // Semiphore, used for saving safely...
bool g_exitFlag=false;             // Used to exit if signal recieved in save...

// -----------------------------------------------------------------------------

void Signal_TERM_or_INT(int)
{ // Signal handeler for tesrmination (SIGTERM) from fvwm95 quitting!
  // 2003: Also made it work for INT signal now (cntl-C).
  // No interest in int value, but would contain the SIGNAL NUMBER.

  // Check semiphore.
  if (g_saving==true)
    g_exitFlag=true;       // Tell main loop to exit and then tidy up after.

  // Just stop then!
  else
    exit(0);

} // End SIGTERM Handeler.

// *****************************************************************************

int main(int argc,char** argv)
{

  // * THESE VARIABLES NEED TO HAVE THEIR VALUES SAVED ALONG WITH THE EVAL SET *

  // The iteration number we are on.
  int iter=0;

  // How many quiecent postions have we got?
  int numQuiescentPositions;

  // This is the current learning rate.
  double learningRate=LEARNING_RATE;

  // This is the current lambda setting to use.
  double lambda=START_LAMBDA;

  // Current Error(s).
  double totalSquaredError;             // For LMS.
  double totalError;                    // Linear/absolute differance.

  // Number of Draws and Wins/Loses in database.
  int draws;
  int winLose;

  // * THESE VARIABLES DO NOT NEED THEIR VALUES SAVING *

  bool variablesLoaded=false;           // Have we loaded the variables?

  int        gameResult;                // Final reward for game.
  int        numMovesInGame;            // Number of moves in current game.
  MoveStruct moveLoaded;                // Move from database.
  // Use vector for runtime-sized array (MAX_PLYS_PER_GAME is now configurable)
  // Note: Using uint8_t instead of bool to avoid std::vector<bool> specialization issues
  std::vector<uint8_t> positionIsQuiescent(SearchConfig::DEFAULT_MAX_PLYS_PER_GAME); // Is Position N quiescent?
  positionIsQuiescent.assign(SearchConfig::DEFAULT_MAX_PLYS_PER_GAME, 1); // Initialize all elements to true (1) 

  // Evaluation sets (First is actual, second is temporary).
  EvaluationParameters evalParams;

  // For setting expected reward.
  double firstEval;                     // Target Eval of last position.
  double nextEval;                      // What we got from current eval.
  double mul;                           // To alternate player side.
  double proportion;                    // Decaying proportion of TD1/TD0.

  ifstream inFile;                      // (Minimal) Database file to use.

  int fileLen;                          // The size of the file we are using.

  // Desired/actual vectors.
  double desiredOutput,actualOutput;

  // These two are used to save and load the variables.
  ifstream inVars;
  ofstream outVars;

  // This holds the name of the vars file.
  std::string varsName;

  // This is used to read the pos in the data file we want to seek to.
  int savedFilePos;

  // This is used to read the extra '\0' char at the end of the moves in DB.
  char junk;

  // This is how long it is since we last save eval set and params (in games).
  int gameSinceLastSave=0;

  // * END OF VARIABLES *

  // Set up the signal handelers.
  signal(SIGTERM,Signal_TERM_or_INT);
  signal(SIGINT,Signal_TERM_or_INT);

  // Setup CLI parser
  CliParser parser("TrainEval", "Train evaluation weights from game database");
  parser.addPositional("database", "Training database file (.min)");
  parser.addPositional("eval_set", "Evaluation set file (.set)");

  if (!parser.parse(argc, argv)) {
    const char* error = parser.getError();
    if (error && error[0]) {
      cerr << error << endl << endl;
    }
    parser.printHelp();
    return 1;
  }

  const char* dataFile = parser.getPositional(0);
  const char* evalSet = parser.getPositional(1);

  // Attempt to open the database requested and find the size of it in bytes.
  inFile.open(dataFile,ios::binary);
  if (inFile.fail())
    FATAL_ERROR("Could not open the database file.");
  inFile.seekg(0,ios::end);
  fileLen=inFile.tellg();
  inFile.seekg(0,ios::beg);

  // Start off with random (or zeroed) sets, if one noe already there.
  // If their is one their, use the save variables also.
  if (evalParams.load(evalSet)==true) {
    evalParams.randomize(MAX_INIT);
    cout << "*NEW* Set     : " << evalSet << endl;
  }
  else {

    // Make the name of the vars file.
    varsName = std::string(evalSet) + ".vars";

    // Attempt to open the vars file.
    inVars.open(varsName);
    if (inVars.fail())
      FATAL_ERROR("Could not open the corresponding *.vars file.");

    // Read each of the variables.
    inVars >> iter
           >> numQuiescentPositions
           >> learningRate
           >> lambda
           >> totalSquaredError
           >> totalError
           >> draws
           >> winLose
           >> savedFilePos;

    // Close the vars file.
    inVars.close();

    // Seek to the desired position in data file.
    inFile.seekg(savedFilePos,ios::beg);

    // Set the fact that we have loaded the varibles.
    variablesLoaded=true;

  }

  // Print the settings we are using (if we are not continuing with saved vars).
  if (variablesLoaded==false) {
    cout << "Database      : " << dataFile << " (" << fileLen << " bytes)"
                               << endl;
    cout << "Learning Rate : " << learningRate << endl;
    cout << "L.R. R.F.     : " << LR_REDUCTION << endl;
    cout << "Magnify       : " << MAGNIFY << endl;
    cout << "Discount      : " << DISCOUNT << endl;
    cout << "Start Lambda  : " << lambda << endl;
    cout << "Lambda R.F.   : " << LAMBDA_REDUCTION << endl;
    cout << "Max Init      : " << MAX_INIT << endl;
    //cout << "Num Weights   : " << (NUM_STAGES*NUM_FEATURES) << endl;
    cout << "NN_Target     : " << NN_TARGET << endl;
    cout << endl;

    cout << "Training..." << endl;
  }

  // Run for many iterations.
  for (;;iter++) {

    // If we have loaded the variables, keep them and clear flag.
  if (variablesLoaded==false) {

      totalSquaredError=0;
      totalError=0;

      draws=0;
      winLose=0;

      numQuiescentPositions=0;

      // Print leader.
      cout << (iter+1) << ' ';
      cout.flush();

    }
    else {
      variablesLoaded=false;                  // Clear flag as used now.
    }

    // Keep going unitl we get the the end of the file.
    while (static_cast<int>(inFile.tellg())<fileLen) {

      // Do we want to do a save now?
      if (gameSinceLastSave==SAVE_EVERY) {

        // Save the evaluation set and current vars (*bofore* each game!).
        g_saving=true;                            // Set up semiphore.

        // Save the evaluation set we have at the moment.
        if (!evalParams.save(evalSet))
          FATAL_ERROR("Failed to save evaluation set.");

        // Make the name of the vars file.
        varsName = std::string(evalSet) + ".vars";

        // Attempt to open the vars file.
        outVars.open(varsName);
        if (outVars.fail())
          FATAL_ERROR("Could not open the corresponding *.vars file.");

        // Write each of the variables.
        outVars << setprecision(32)
                << iter << endl
                << numQuiescentPositions << endl
                << learningRate << endl
                << lambda << endl
                << totalSquaredError << endl
                << totalError << endl
                << draws << endl
                << winLose << endl
                << inFile.tellg() << endl;

        // Close the vars file.
        outVars.close();

        g_saving=false;                           // Take down semiphore.

        // Do we need to exit now?
        if (g_exitFlag==true)
          exit(0);

        // Reset the count.
        gameSinceLastSave=0;

      }

      // We done one more game now (for save to see above).
      gameSinceLastSave++;

      // Read the data from the file (*AFTER* Saving the vars).
      readMinimalHeader(numMovesInGame,gameResult,inFile);

      // Init all a data to a new game.
      initAll();

      // Count draws.
      if (gameResult==0)
        draws++;
      else
        winLose++;

      // Read and make all the moves.
      for (int i=0;i<numMovesInGame;i++) {
        readMinimalMove(moveLoaded,positionIsQuiescent[i+1],inFile);
        if (!makeMove(moveLoaded))
            FATAL_ERROR("Move in the database is invalid(?).");
      }

      // Read the extra '\0' char.
      inFile.read(&junk,1);

      // Draws are worth 0, win +1 and loss -1.
      if (gameResult==0) {
        firstEval=0;
      }
      else if (gameResult==1) {
        if (g_currentSide==WHITE) {
          firstEval=NN_TARGET;
        }
        else {
          firstEval=-NN_TARGET;
        }
      }
      else {
        if (g_currentSide==BLACK) {
          firstEval=NN_TARGET;
        }
        else {
          firstEval=-NN_TARGET;
        }
      }

      // Learn weights for the final state (Only if Quiescent!).
	  // NOTE: We now makes sure that the material is even too.
      if (positionIsQuiescent[numMovesInGame]==true && basicMaterialEval()==0) {

        // Set output.
        desiredOutput=firstEval;

        // Train (Quick version can't use momentums!).
        totalSquaredError+=evalParams.train(desiredOutput,learningRate*MAGNIFY,
                                    actualOutput);

        // Find total (linear) error.
        totalError+=fabs(desiredOutput-actualOutput);

        // Set up the Next eval.
        nextEval=actualOutput;

        // One more Quiescent position.
        numQuiescentPositions++;                 // One more.

      }
      else {
        nextEval=firstEval;
      }

      // For each of the imbetween moves, update.
      mul=1.0;
      for (int i=numMovesInGame-1;i>=0;i--) {

        // Take the move back.
        takeMoveBack();

        // Next player now.
        mul=-mul;

        // Reduce the first evaluation.
        firstEval*=DISCOUNT;

        // Learn weights for the this state (Only if Quiescent!).
		// NOTE: We now makes sure that the material is even too.
        if (positionIsQuiescent[i]==true && basicMaterialEval()==0) {

          // TD-LAMBDA.
          proportion=pow(lambda,numMovesInGame-i);
          desiredOutput=((proportion*(firstEval*mul))
                           +(DISCOUNT*(1.0-proportion)*(-nextEval)));

          // Train (Quick version can't use momentums!).
          totalSquaredError+=evalParams.train(desiredOutput,learningRate,actualOutput);

          // Find total (linear) error.
          totalError+=fabs(desiredOutput-actualOutput);

          // Set up the Next eval.
          nextEval=actualOutput;

          // One more Quiescent position.
          numQuiescentPositions++;                 // One more.
      
        }
        else {
          nextEval=DISCOUNT*(-nextEval);
        }

      } // End for each move.

    } // End for each game.

    // Seek back to the start of the data file.
    inFile.seekg(0,ios::beg);

    // Print running stats.
    cout << setprecision (8)
		 << totalSquaredError/(double)numQuiescentPositions
         << " # E=" << totalError/(double)numQuiescentPositions
         << " LR=" << learningRate << " L=" << lambda
         << " Q=" << numQuiescentPositions
         << " D=" << draws << " WL=" << winLose << endl;

    // Reduce the learning rate.
    learningRate*=LR_REDUCTION;

    // Reduce lambda for next iteration.
    lambda*=LAMBDA_REDUCTION;

  } // End for each iteration.

  return 0;

} // End main.