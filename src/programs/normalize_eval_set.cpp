// normalize_eval_set.cc
// =====================
// * NOTE: You MUST use the program on any trained file: BEFORE playing it!
// This program will do the following, given a evalation file:
// 1. It will find the mean (material) values for each type of peice.
// 3. Will subtract the means (matarial), to make real piece_square data!
// 3. Will scale all the values by the given amount.
// AFTER DOING THIS: You may use this to play with!

// Include headers only - implementations linked separately
#include "../chess_engine/chess_engine.h"
#include "../search_engine/search_engine.h"
#include "../interface/interface.h"
#include "../core/cli_parser.h"

using namespace std;

int main(int argc,char** argv)
{

  // This is the evaluation set we are going to use.
  EvaluationParameters evalParams;

  // This is the scale factor we want to use.
  double scaleFactor;

  // Setup CLI parser
  CliParser parser("normalize_eval_set", "Normalize and scale evaluation weights");
  parser.addPositional("input", "Input evaluation set");
  parser.addPositional("output", "Output evaluation set");
  parser.addPositional("scale", "Scale factor");

  if (!parser.parse(argc, argv)) {
    const char* error = parser.getError();
    if (error && error[0]) {
      cerr << error << endl << endl;
    }
    parser.printHelp();
    return 1;
  }

  const char* inputFile = parser.getPositional(0);
  const char* outputFile = parser.getPositional(1);
  scaleFactor = atof(parser.getPositional(2));

  // Attempt to read the input file.
  if (evalParams.load(inputFile)==true)
    FATAL_ERROR("Could not open the input file.");

  // Print scale factor.
  cout << "ScaleFactor: " << scaleFactor << endl << endl;

  // Normalize and Scale the features.
  evalParams.normalize();
  evalParams.scale(scaleFactor);

  // Save the new features.
  if (evalParams.save(outputFile)==true)
    FATAL_ERROR("Could not open the output file.");

  return 0;

} // End main.