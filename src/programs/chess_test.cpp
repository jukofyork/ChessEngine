// chess_test.cc
// =============
// This code is used to test the chess engine on all the possitions in the
// 'Chess Test' test suit.
// It takes the name of all tests files as an argument and runs the tests on
// each position, producing the following output files:

// New: Have now decided (after comparing results with gnuchess results) 
//      that '!?' moves must actually be desired moves and this will affect the 
//      following files:
//      - ece3.fin    : 326 !?'s
//      - gelfer.fin  : 1   !?'s
//      - hardmid.fin : 552 !?'s
//      So that it should work in the future, i have made it do '?!' moves are
//      also classed as desired also now, even though none in files!

// 2003_v8:
// ********
// After getting an updated version of the tests, it apears that !? and ?! moves
// are bad/undesired moves and that it was a problem in hardmid.fin after all!
// Have changed it back so that these are bad moves now.

// Include headers only - implementations linked separately
#include "../chess_engine/chess_engine.h"
#include "../search_engine/search_engine.h"
#include "../interface/interface.h"
#include "../core/cli_parser.h"
#include "../chess_engine/globals.h"
#include "../search_engine/search_config.h"
#include <array>

using namespace std;

// How long to search each move.
constexpr double DEFAULT_SEARCH_TIME = 10.0;

bool loadNextPosition(ifstream &inFile,std::array<MoveStruct, 100>& desiredMoves,
                      int &numDesired,bool &unDesired)
{ // Loads the next position from the file.
  // Returns true if no more exist in the file, else false.

  int bufferIndex;

  // The line of text read from the file.
  std::string buffer;

  // The number of slashes in the line.
  int numSlashes;

  // We jump here if we find a bad move.
  StartAgain:

  // Lists the moves that had a ? (NOT: !?/?! = desired) with them.
  std::array<bool, 100> questions;
  questions.fill(false);

  // Try to load the buffer with a string, until we get a valid move or EOF.
  do {

    // Read the string.
    inFile >> buffer;
    if (inFile.eof())
      return true;                          // Failed, enof of file.

    // See if we have exactly 8 '/' characters, if not then it's not a good pos.
    numSlashes=0;
    for (size_t i=0; i<buffer.size(); i++) {
      if (buffer[i]=='/')
        numSlashes++;
    }

  } while (numSlashes<8);

  // Assume certain things to be overrided later in function.
  g_gameHistory[0].castlePerm=0;         // No castling yet.
  g_gameHistory[0].enPass=NO_EN_PASSANT; // En-Pasent is NOT allowed yet.

  // We have a valid line, so lets parse it.
  bufferIndex=0;
  for (int i=0;;bufferIndex++) {

    // See if we got too many squares.
    if (i==64) {
      if (buffer[bufferIndex]!='/') {
        LOG_WARNING("Too many/invalid squares.");
        goto StartAgain;
      }
      else {
        if (buffer[bufferIndex+1]=='w') {
          g_currentSide=WHITE;
        }
        else if (buffer[bufferIndex+1]=='b') {
          g_currentSide=BLACK;
        }
        else {
          LOG_WARNING("Invalid first player.");
          goto StartAgain;
        }
        break;                                 // Get move(s) now!
      }
    }

    // Should this be a slash?
    if (buffer[bufferIndex]=='/')
      continue;

    // Is it a number - ie: Blank spaces.
    if (buffer[bufferIndex]>='1' && buffer[bufferIndex]<='8') {
      for (int j=0;j<(buffer[bufferIndex]-'0');j++) {
        g_gameHistory[0].piece[i]=NONE;
        g_gameHistory[0].colour[i++]=NONE;
      }
    }

    // Is it a white pawn.
    else if (buffer[bufferIndex]=='P') {
      g_gameHistory[0].piece[i]=PAWN;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black pawn.
    else if (buffer[bufferIndex]=='p') {
      g_gameHistory[0].piece[i]=PAWN;
      g_gameHistory[0].colour[i++]=BLACK;
    }
    // Is it a white knight.
    else if (buffer[bufferIndex]=='N') {
      g_gameHistory[0].piece[i]=KNIGHT;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black knight.
    else if (buffer[bufferIndex]=='n') {
      g_gameHistory[0].piece[i]=KNIGHT;
      g_gameHistory[0].colour[i++]=BLACK;
    }
    // Is it a white bishop.
    else if (buffer[bufferIndex]=='B') {
      g_gameHistory[0].piece[i]=BISHOP;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black bishop.
    else if (buffer[bufferIndex]=='b') {
      g_gameHistory[0].piece[i]=BISHOP;
      g_gameHistory[0].colour[i++]=BLACK;
    }
    // Is it a white rook.
    else if (buffer[bufferIndex]=='R') {
      if (i==63)
        g_gameHistory[0].castlePerm|=WHITE_KING_SIDE;
      else if (i==56)
        g_gameHistory[0].castlePerm|=WHITE_QUEEN_SIDE;
      g_gameHistory[0].piece[i]=ROOK;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black rook.
    else if (buffer[bufferIndex]=='r') {
      if (i==7)
        g_gameHistory[0].castlePerm|=BLACK_KING_SIDE;
      else if (i==0)
        g_gameHistory[0].castlePerm|=BLACK_QUEEN_SIDE;
      g_gameHistory[0].piece[i]=ROOK;
      g_gameHistory[0].colour[i++]=BLACK;
    }
    // Is it a white queen.
    else if (buffer[bufferIndex]=='Q') {
      g_gameHistory[0].piece[i]=QUEEN;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black queen.
    else if (buffer[bufferIndex]=='q') {
      g_gameHistory[0].piece[i]=QUEEN;
      g_gameHistory[0].colour[i++]=BLACK;
    }
    // Is it a white king.
    else if (buffer[bufferIndex]=='K') {
      g_gameHistory[0].kingSquare[WHITE]=i; // Set to start position.
      g_gameHistory[0].piece[i]=KING;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black king.
    else if (buffer[bufferIndex]=='k') {
      g_gameHistory[0].kingSquare[BLACK]=i;  // Set to start position.
      g_gameHistory[0].piece[i]=KING;
      g_gameHistory[0].colour[i++]=BLACK;
    }

    // Special case characters.

    // Is it a white rook (moved, but on orig square).
    else if (buffer[bufferIndex]=='S') {
      g_gameHistory[0].piece[i]=ROOK;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black rook (moved, but on orig square).
    else if (buffer[bufferIndex]=='s') {
      g_gameHistory[0].piece[i]=ROOK;
      g_gameHistory[0].colour[i++]=BLACK;
    }

    // Set up en-passent pawn.
    else if (buffer[bufferIndex]=='O') {
      g_gameHistory[0].enPass=i+8; // En-Pass target square.
      g_gameHistory[0].piece[i]=PAWN;
      g_gameHistory[0].colour[i++]=WHITE;
    }
    // Is it a black rook (moved, but on orig square).
    else if (buffer[bufferIndex]=='o') {
      g_gameHistory[0].enPass=i-8; // En-Pass target square.
      g_gameHistory[0].piece[i]=PAWN;
      g_gameHistory[0].colour[i++]=BLACK;
    }

    else {
      cout << "(See below:" << buffer[bufferIndex] << ')' << endl;
      LOG_WARNING("Bad piece char^.");
      goto StartAgain;
    }
  }

  // See if the king positions allow castling.
  if (g_gameHistory[0].kingSquare[WHITE]!=60)
    g_gameHistory[0].castlePerm&=~(WHITE_KING_SIDE|WHITE_QUEEN_SIDE);
  if (g_gameHistory[0].kingSquare[BLACK]!=4)
    g_gameHistory[0].castlePerm&=~(BLACK_KING_SIDE|BLACK_QUEEN_SIDE);

  // Start on move 0.
  g_moveNum=0;                           // Now on move 0.

  // Init fifty move counter to 0.
  g_gameHistory[0].fiftyCounter=0;       // Reset the first-move-rule counter.

  // Can't (shouldn't!) be a draw on the first move.
  g_gameHistory[0].isDraw=false;

  // Set the pointer to the first state.
  g_currentState=&g_gameHistory[0];

  // Set up the pointer to the current board.
  g_currentColour=g_currentState->colour;
  g_currentPiece=g_currentState->piece;

  // See if the current side is in check to start with.
  g_gameHistory[0].inCheck=isAttacked(g_gameHistory[0].kingSquare[g_currentSide],
                                getOtherSide(g_currentSide));

  // Set the currect Hash key up.
  g_gameHistory[0].key=currentKey();

  // Parse the list of moves we must (or must not!) choose.
  // Get the source and target square first.
  numDesired=0;
  inFile >> buffer;

  // See if it's a crap move!
  if (inFile.eof())
    return true;
  if (buffer[0]=='*' || buffer.size()<5) {
    // No need for recursion!
    cout << "Bad move found, ignoreing..." << endl;
    goto StartAgain;                                // Try again!
  }

  for (int i=0,bufferIndex=0,foundAnother=true;foundAnother==true;i++) {
    desiredMoves[i].source=-1;
    desiredMoves[i].target=-1;
    desiredMoves[i].promote=0;          // Empty.
    desiredMoves[i].type=NORMAL_MOVE;
    for (;bufferIndex<static_cast<int>(buffer.size());bufferIndex++) {
      if (buffer[bufferIndex]>='a' && buffer[bufferIndex]<='h') {
        if (buffer[bufferIndex+1]<'1' || buffer[bufferIndex+1]>'8') {
          LOG_WARNING("Invalid move (source) found^.");
          goto StartAgain;
        }
        desiredMoves[i].source=(8*('8'-buffer[bufferIndex+1]))+(buffer[bufferIndex]-'a');
        break;
      }
    }
    for (bufferIndex++;bufferIndex<static_cast<int>(buffer.size());bufferIndex++) {
      if (buffer[bufferIndex]>='a' && buffer[bufferIndex]<='h') {
        if (buffer[bufferIndex+1]<'1' || buffer[bufferIndex+1]>'8') {
          goto StartAgain;
          LOG_WARNING("Invalid move (target) found.");
        }
        desiredMoves[i].target=(8*('8'-buffer[bufferIndex+1]))+(buffer[bufferIndex]-'a');
        break;
      }
    }
    if (desiredMoves[i].source<0 || desiredMoves[i].source>63
        || desiredMoves[i].target<0 || desiredMoves[i].target>63) {
      cout << static_cast<int>(desiredMoves[i].source) << '-'
           << static_cast<int>(desiredMoves[i].target) << endl;
      LOG_WARNING("Move sanity^...");
      goto StartAgain;
    }
    foundAnother=false;
    // Promotion, if so what peice.
    for (bufferIndex++;bufferIndex<static_cast<int>(buffer.size());bufferIndex++) {
      if (buffer[bufferIndex]=='=') {
        if (buffer[bufferIndex+1]=='Q')
          desiredMoves[i].promote=QUEEN;
        else if (buffer[bufferIndex+1]=='R')
          desiredMoves[i].promote=ROOK;
        else if (buffer[bufferIndex+1]=='B')
          desiredMoves[i].promote=BISHOP;
        else if (buffer[bufferIndex+1]=='N')
          desiredMoves[i].promote=KNIGHT;
        else {
          LOG_WARNING("Invalid promotion piece.");
          goto StartAgain;
        }
        desiredMoves[i].type=PROMOTION;
      }
      if (buffer[bufferIndex]=='?') {
        // new: Take '!?' and '?!' moves to be desired moves now!
        // 2003_v8: Definitely unwanted moves (hardmid.fin changed in new ver).
        //if (buffer[bufferIndex-1]!='!' && ((bufferIndex+1)>=buffer.size() || buffer[bufferIndex+1]!='!'))
          questions[numDesired]=true;
      }
      if (buffer[bufferIndex]==',') {
        foundAnother=true;
        break;
      }
      if (buffer[bufferIndex]==' ')
        break;
    }

    // One more got.
    numDesired++;

  }

  // See if any undesired moves.
  unDesired=false;
  for (int i=0;i<100;i++)
    if (questions[i]==true)
      unDesired=true;

  // If there were some undesired, just get them and no more.
  if (unDesired==true) {
    int newNum=0;
    for (int i=0;i<numDesired;i++) {
      if (questions[i]==true)
        desiredMoves[newNum++]=desiredMoves[i];
    }
    numDesired=newNum;
  }

  // All OK.
  return false;

} // End loadNextPosition.

// ----------------------------------------------------------------------------

int main(int argc, char** argv)
{
  // How many position in file.
  int count=0;

  // How many correct.
  int numCorrect=0;

  // This is the move(s) we want/don't want.
  std::array<MoveStruct, 100> desiredMoves; // List of moves.
  int numDesired;               // How many in list.
  bool unDesired;               // Not wanted, inverts the move list's meaning.

  // This is the list of Moves/Captures generated from this state.
  MoveList moves;

  // This is the move returned by think.
  MoveStruct chosenMove;

  // Input file.
  ifstream inFile;

  // Do we want correct or incorrect moves to be tested for this position.
  bool correct;

  // This is the amount of time to search each position with.
  double searchTime=DEFAULT_SEARCH_TIME;

  // The evaluation parameter set to use.
  EvaluationParameters evalParams;

  // Setup CLI parser
  CliParser parser("ChessTest", "Run chess engine test suite on positions");
  parser.addPositional("test_file", "Test positions file (.fin)");
  parser.addPositional("eval_set", "Evaluation weights file (.set)");
  parser.addOption("time", 't', "Search time per position (seconds)",
                   CliParser::OptionType::DOUBLE, "10.0");
  parser.addOption("cpu-time", '\0', "Use CPU time instead of wall clock time",
                   CliParser::OptionType::BOOL, nullptr);
  parser.addOption("hash-size", 'H', "Hash table size in MB (default: 512)",
                   CliParser::OptionType::INT, "512");

  if (!parser.parse(argc, argv)) {
    const char* error = parser.getError();
    if (error && error[0]) {
      cerr << error << endl << endl;
    }
    parser.printHelp();
    return 1;
  }

  // Get arguments
  const char* testFile = parser.getPositional(0);
  const char* evalSet = parser.getPositional(1);
  searchTime = parser.getDouble("time");
  if (searchTime <= 0) {
    cerr << "ChessTest: search time must be > 0" << endl;
    return 1;
  }

  // Set timing mode
  if (parser.getBool("cpu-time")) {
    g_timingMode = TimingMode::CPU_TIME;
  }

  // Set hash table size from CLI (other parameters use defaults)
  g_searchConfig.hashSizeMB = parser.getInt("hash-size");

  // Validate hash size
  if (g_searchConfig.hashSizeMB <= 0 || g_searchConfig.hashSizeMB > 4096) {
    cerr << "ChessTest: hash-size must be between 1 and 4096 MB" << endl;
    return 1;
  }

  // Initialize global resources with search configuration
  initGlobals(g_searchConfig);

  // First cearte the lookup tables from own program (to make hybrid)
  generateMoveTables();                // Create the move lookup tables.

  // Generate the exposed attack table.
  generateExposedAttackTable();        // MUST BE DONE AFTER MOVE TABLES.

  // Generate the g_posData tables.
  initPosData();

  // Init the hash codes.
  initHashCodes();

  if (evalParams.load(evalSet)==true) {
    cout << "Invalid Evaluation Set file: Exiting..." << endl;
    return 1;
  }

  // Load all positions and display them.
  inFile.open(testFile);
  if (inFile.fail())
    FATAL_ERROR("Could not open input file.");

  while (loadNextPosition(inFile,desiredMoves,numDesired,unDesired)==false)  {
    genMoves(moves);
    cout << "File: " << testFile << " / Position: " << count << endl;
    printBoard(g_currentSide);
    count++;
    if (g_currentSide==WHITE)
      cout << "WHITE to move." << endl << endl;
    else
      cout << "BLACK to move." << endl << endl;
    chosenMove=think(INFINITE_DEPTH,searchTime,true,true,0.0,evalParams);
    cout << endl;

    // Print the move chosen and the desired move.
    cout << "Chosen: ";
    printMove(chosenMove);
    if (unDesired==false)
      cout << " / Desired move(s): ";
    else
      cout << " / Undesired move(s): ";
    for (int i=0;i<numDesired;i++) {
      printMove(desiredMoves[i]);
      cout << ' ';
    }

    // Did we get it correct?
    if (unDesired==false) {
      correct=false;
      for (int i=0;i<numDesired;i++) {
        if (desiredMoves[i].source==chosenMove.source
            && desiredMoves[i].target==chosenMove.target
            && (desiredMoves[i].promote==0
                || desiredMoves[i].promote==chosenMove.promote)) {
          correct=true;
          break;
        }
      }
    }
    else {
      correct=true;
      for (int i=0;i<numDesired;i++) {
        if (desiredMoves[i].source==chosenMove.source
            && desiredMoves[i].target==chosenMove.target
            && (desiredMoves[i].promote==0
                || desiredMoves[i].promote==chosenMove.promote)) {
          correct=false;
          break;
        }
      }
    }
    if (correct==true) {
      cout << "= CORRECT" << endl;
      numCorrect++;
    }
    else {
      cout << "= WRONG" << endl;
    }
    cout << endl << endl;
    //MainCanvas.StartEventLoop(NULL,Key);
  }

  cout << "TOTAL POSISTIONS ANALYZED : " << count << endl;
  cout << "TOTAL NUMBER CORRECT      : " << numCorrect << endl;
  cout << "TOTAL % CORRECT           : "
       << 100.0*(static_cast<double>(numCorrect)/static_cast<double>(count)) << endl;

  // Close the input file.
  inFile.close();

  return 0;

} // End main.
