// randomize_games.cc
// ==================
// This program randomizes the order of games in a binary database file.
// NOTE: A bit hacked and could do with var names sorting out etc, but ok at mo.

// Include headers only - implementations linked separately
#include "../chess_engine/chess_engine.h"
#include "../search_engine/search_engine.h"
#include "../interface/interface.h"
#include "../core/cli_parser.h"

#include <fstream>
#include <random>
#include <algorithm>
#include <vector>

using namespace std;

// =============================================================================

struct PosRand
{ // This is used to output the games in a random order.
  int    pos;                           // File position.
  double randNo;                        // Random number to sort.
}; // End PosRand.

// =============================================================================

// The maximum number of lines we may encounter in input data file (BAD!).
constexpr int MAX_LINES = 10000000;



// =============================================================================

int main(int argc,char** argv)
{

  // This is global, so the function comp can use it.
  ifstream inFile;                      // Unsorted input file.

  // Output files.
  ofstream outFile;                     // Sorted version of input.

  // This is for randomizing the data.
  std::vector<PosRand> randList;        // These are the non-twin games.
  randList.reserve(MAX_LINES);          // Pre-allocate for efficiency.
  int    numLines=0;                    // Number of games, after twin removal.

  int numMoves, gameResult, numWritten=0;

  // This is used to read junk lines into.
  std::vector<char> junk;

  // Random number generator for this program
  static std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.0, 1.0);

  // Setup CLI parser
  CliParser parser("randomize_games", "Randomize order of games in database");
  parser.addPositional("input", "Input binary database");
  parser.addPositional("output", "Output randomized database");

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

  // Open the input file.
  inFile.open(inputFile,ios::binary);
  if (inFile.fail())
    FATAL_ERROR("Could not open the input file.");

  // Open the output file.
  outFile.open(outputFile,ios::binary);
  if (outFile.fail())
    FATAL_ERROR("Could not open the output file.");

  // Seek to end, to find length of file, then back to start.
  // NOTE: This find the file length, BUT also is done to stop the bug that
  //       means the file will *NOT* work, after it gets past the eof (why?).
  inFile.seekg(0,ios::end);
  int fileLen=inFile.tellg();
  inFile.seekg(0,ios::beg);
  cout << "File length: " << fileLen << " bytes" << endl;

  // Start off with the first line being at 0 offset!.
  randList.push_back({static_cast<int>(inFile.tellg()), dist(rng)});
  numLines++;

  // Find the index in (bytes) each line is.
  cout << "Finding game indexes... "; cout.flush();
  for (;;) {

    // Read the header and skip as many bytes as needed.
    readMinimalHeader(numMoves,gameResult,inFile);
    inFile.seekg((numMoves*3)+1,ios::cur);           // 3 bytes per move + '\0'.

    // Check we are not at the end of the file.
    if (static_cast<int>(inFile.tellg())==fileLen)
      break;

    // Copy the pos no, along with a random number.
    randList.push_back({static_cast<int>(inFile.tellg()), dist(rng)});
    numLines++;

  }
  cout << "Done (" << numLines << " games)." << endl;

  cout << "Randomizing game orders... "; cout.flush();
  std::sort(randList.begin(), randList.begin() + numLines, 
            [](const PosRand& a, const PosRand& b) { return a.randNo < b.randNo; });
  cout << "Done (" << numLines << " games)." << endl;

  // Write each of the randomized games now (in random order now).
  cout << "Writing (randomized) games to output file... " << endl;
  for (int i=0;i<numLines;i++) {

    // Seek to the position in the input file.
    inFile.seekg(randList[i].pos,ios::beg);

    // Read the header and the game data.
    readMinimalHeader(numMoves,gameResult,inFile);
    junk.resize((numMoves*3)+1);
    inFile.read(junk.data(), junk.size());

    // Write the header and the game data.
    outFile << (unsigned char)numMoves;
    outFile << (unsigned char)gameResult;
    outFile.write(junk.data(), junk.size());

    numWritten++;

  }

  // Close files.
  inFile.close();
  outFile.close();

  cout << "Done (wrote " << numWritten << " games)." << endl;

  return 0;

} // End main.

// =============================================================================

// End randomize_games.cc