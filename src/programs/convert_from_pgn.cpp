// Include headers only - implementations linked separately
#include "../chess_engine/chess_engine.h"
#include "../search_engine/search_engine.h"
#include "../search_engine/search_config.h"
#include "../interface/interface.h"
#include "../core/cli_parser.h"

using namespace std;

// Old: * BUGGED!
//      * Quiescent score won't fucking work now???
//      * FIXED: IsQuiescent() (quick) wasn't sorting moves for expansion!

// 2003: * Have now made it print the games on a single line.
//       * Ignore 'silly' number of moves in game.
//       * Ignore games with no result.
//       * Now takes input on stdin, and produces a binary output file.

// 2003a: * Have now made the Quick quiescent search time out.
//        * This was done to stop 'silly' exibition games from causing problems.

// 2003b: * Have almost completely re-written the program to make more stable,
//          and to work with more PGN games/files, as i have with the
//          parse_pgn.cc function to make it more stable - at the same time.
//        * Now capable of reading/ignoreing comments and variations, including
//          nested ones up to any level.
//          - Have now made it so the program tried to parse the pgn file using
//            non-nested comments, if it fails and find an unterminated comment
//            when using nexted comments.
//          - Uses the "[Event" tag of the next game to detect unterminated
//            comments (or the eof).
//         * Have added in a #define to allow for testing code without the need
//           to add real IsQuiescent() values in (good for testing/debugging).
//         * Have added in code to make the program only update how many games
//           have been converted every N games (was slowing things down...).
//         * No longer reports errors on small/large number of moves and when
//           their is no result etc... Only clases bad SAN move as error now.
//         * Have made the code ignore 'strange' '$' symbol (jatsmak2 games). i
//           think this is variation number, eg: $4, $15, etc????
//         * The program will now handle continuation moves like ...Kc4, where
//           before this would be read wrongly (comes after variations etc).
//         * The PGN tags are scanned now for 'FEN' position, and if there is
//           one, it is ignored as an error. There is little point (for
//           train_eval) to use these, so just ignore them and don't report an
//           error now.
//         * The result 'aborted' is now read. This was in many games from
//           the crafty.pgn file, and now it is treated as a no-result game and
//           not classed as an error (non-standard????). Also all the extra text
//           between each crafty.pgn game is handled well now (no errors...).
//         * Have made it so that when the program comes accross and error, it
//           saves and prints all the text in the game, upto the error. This can
//           then be pasted into xboard etc, to check if the error is the fault
//           of the pgn file or the program.
//           - Also error numbers are displayed in parse_pgn code, to help
//             debugging it...
//         * Have reset the size range for 'non-sensible' number of moves for
//           games lower than 3 moves and more than 800 (from observation...).
//         * Now also adds the '\0' char to the end of a move list, for using
//           unix sort (new binary format).
//           NOTE: This *should* be the only '\0' char in the whole game, and
//                 have added code to the WriteMinimal...() functions to check
//                 for this (was bugged b4!).
//         * The program no longer acts as a filter, so that the eof() if not
//           passed, allowing us to re-try parsing comments that are non-nested.
//         * Have now made it so that the Quick quiescent search has a maximum
//           node limit, rather than a time out (bugs...).

// This is how often we print the number of games converted so far.
constexpr int PRINT_EVERY = 1000;

// Shall we really do a quiescence search or just fake it (for testing)?
#define REAL_QUIESCENCE

int main(int argc, char** argv)
{

  // This is the Buffer that we read the moves into.
  std::string buffer;

  // This is used to store any move which is stuck on the front of a comment,
  // without any whitespace seperating it.
  std::string sanPrefix;

  // This is the text of all the moves in the current game (for debuging).
  std::string gameText;

  // The result of a game, after finding it in text.
  int result;

  MoveStruct moveChosen;

  int numPos;

  int numGames;
  int numErrors;

  // Used to exit loop when an error is found or game is written.
  int numTries;

  // This is used to test if the search 'timed-out'...
  int isQuiescentResult;

  // This stores whether the move *LEADS TO* a quiescent position.
  // It is then printed with the minimal move.
  std::vector<uint8_t> moveIsQuiescent(SearchConfig::DEFAULT_MAX_PLYS_PER_GAME);

  // The PGN input file.
  ifstream inFile;

  // The (Binary) ouput file.
  ofstream outFile;

  // Have we found a fen tagg before this game?
  bool fenTaggFound;

  // The length of the input file and the place where current game starts.
  int fileLen,movesStartAt;

  // Used to find if we have the '[Event' tag: detecting unterminated comments.
  const char constEventTag[7]="[event"; // NOTE: Lower case, so we can check
  int eventTextIndex;                   //       upper case in code too.

  // Setup CLI parser
  CliParser parser("convert_from_pgn", "Convert PGN games to binary format");
  parser.addPositional("input", "Input PGN file");
  parser.addPositional("output", "Output binary file");

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

  // Try to open the input file and get the lengtg of the file.
  inFile.open(inputFile);
  if (inFile.fail())
    FATAL_ERROR("Could not open the (pgn) input file.");
  inFile.seekg(0,ios::end);
  fileLen=inFile.tellg();
  inFile.seekg(0,ios::beg);

  // Try to open the output file (Binary flag set).
  outFile.open(outputFile,ios::binary);
  if (outFile.fail())
    FATAL_ERROR("Could not open the (binary) output file.");

  // First create the lookup tables from own program (to make hybrid)
  generateMoveTables();                // Create the move lookup tables.

  // Generate the exposed attack table.
  generateExposedAttackTable();        // MUST BE DONE AFTER MOVE TABLES.

  // Generate the g_posData tables.
  initPosData();

  // Init the hash codes.
  initHashCodes();

  initAll();

  numGames=0;
  numErrors=0;

  // Init the pos data.
  numPos=0;

  // The number of round and curly brakets (for nexted count).
  int roundB,curlyB;

  // Print a reminder, just in case.
#ifndef REAL_QUIESCENCE
  cout << "********************************************************" << endl;
  cout << "*** TESTING MODE - *NOT* WORKING OUT REAL QUIESCENCE ***" << endl;
  cout << "********************************************************" << endl;
#endif

  // Keep playing games until no more in file.
  while ((inFile >> buffer) && !inFile.eof()) {

    // If we are not into the PGN tags yet, skip all the junk until we get one.
    while (buffer[0]!='[') {
      inFile >> buffer;
      if (inFile.eof())
        goto Finished;
    }
      
    // Keep reading PGN tags, until we get to the end of them.
    while (buffer[0]=='[') {

      // Is it a 'FEN' tag? - If so, set the flag so we don't count as error.
      if (buffer=="[FEN" || buffer=="[Fen"
          || buffer=="[fen") {
        fenTaggFound=true;
      }

      // Skip to the end of the tagg.
      while (buffer[buffer.size()-1]!=']') {
        inFile >> buffer;
        if (inFile.eof())
          goto Finished;
      }

      // Store the place in the file where we are now (ie: Start of move list).
      movesStartAt=inFile.tellg();

      // Read the next file, ready to test to see if it is a PGN tagg.
      inFile >> buffer;
      if (inFile.eof())
        goto Finished;

    }

    // If this is now the first move, then skip it.
    if (buffer[0]!='1' || buffer[1]!='.') {
        initAll();
        numTries=2;
        fenTaggFound=false;
        continue;
    }

    // WE SHOULD NOW BE AT THE START OF THE GAME'S MOVES...

    // Init the game text (For debugging output).
    gameText.clear();

    // Not tried  twice yet.
    numTries=0;

    // Start reading the game text.
    do {

      // Add the string to the game text.
      gameText += buffer; gameText += " "; // In case of EOL.

      // Is there a variation or a comment 'stuck' after a move etc?
      sanPrefix.clear();                              // Make null to start with.
      if (buffer[0]!='(' && buffer[0]!='{') {
        for (size_t i=1;i<buffer.size();i++) {
          if (buffer[i]=='(' || buffer[i]=='{') {
            sanPrefix = buffer.substr(0, i);          // Make a copy of the move.
            buffer[i]=' ';                            // Replace with space.
            break;
          }
        }
      }

      // Is it a variation or a comment?
      // NOTE: Read one char at a time, just in case of double comment.
      // NOTE: This code will take 'nested' variations and comments.
      //       This *could* go wrong if we come across an unterminated one!!!
      // NOTE: Assumes that comments/variations will not be joined to end of
      //       a SAN move (eg: not Qa6{....}), but can handle if at start.
      if (buffer[0]=='(' || buffer[0]=='{') {

        // Set up the counts.
        if (buffer[0]=='(') {
          roundB=1;
          curlyB=0;
        }
        else {
          roundB=0;
          curlyB=1;
        }

        // Test the rest of the string...
        for (size_t i=1;i<buffer.size();i++) {
          // *ADD* if its another nested, or *TAKE* if we found an ending one.
          if (numTries!=1 && buffer[i]=='(')
            roundB++;
          else if (numTries!=1 && buffer[i]=='{')
            curlyB++;
          else if (buffer[i]==')')
            roundB--;
          else if (buffer[i]=='}')
            curlyB--;
        }

        // Keep going until we get all brackets closed.
        // Set the "[Event" test index to 0.
        eventTextIndex=0;
        char ch;
        while (roundB>0 || curlyB>0) {

          // Read the next byte in the file (including whitespace).
          inFile.get(ch);
          if (static_cast<int>(inFile.tellg())==fileLen) {
            cout << "Unterminated ()'s/{}'s. Found EOF." << endl;
            initAll();
            fenTaggFound=false;
            numTries++;                          // Try again...
            inFile.seekg(movesStartAt,ios::beg); // Seek to start.
            inFile >> buffer;
            cout << gameText << endl << endl;
            gameText.clear();
            if (numTries==2) {
              numErrors++;
              goto Finished;
            }
            cout << "Attempting to parse, using non-nexted comments..." << endl;
            continue;
          }

          // *ADD* if its another nested, or *TAKE* if we found an ending one.
          if (numTries!=1 && ch=='(')
            roundB++;
          else if (numTries!=1 && ch=='{')
            curlyB++;
          else if (ch==')')
            roundB--;
          else if (ch=='}')
            curlyB--;

          // Is this a letter the 'event tag' (small, capitol or mixture)
          if (ch==constEventTag[eventTextIndex]
              || (eventTextIndex>0
                  && ch==(constEventTag[eventTextIndex]-'a')+'A')) {
             eventTextIndex++;
          }
          else {
            eventTextIndex=0;                 // BUG: Make sure you reset it.
          }

          // Save to the gameText debug buffer.
          if (ch=='\n' || ch=='\r')
            ch=' ';
          gameText += ch;

          // Have we found an event tag?
          if (eventTextIndex==6)
            break;                        // All matched, so stop.

        }

        // Was it an, unterminated comment?
        if (eventTextIndex==6) {
          cout << "Unterminated ()'s/{}'s. Found '[Event' (PGN tag?)." << endl;
          initAll();
          fenTaggFound=false;
          numTries++;                          // Try again...
          inFile.seekg(movesStartAt,ios::beg); // Seek to start.
          inFile >> buffer;
          cout << gameText << endl << endl;
          gameText.clear();
          if (numTries==1)
            cout << "Attempting to parse, using non-nexted comments..." << endl;
          else
            numErrors++;
          continue;
        }

        // Did we have a SAN prefix move before the comment?
        if (!sanPrefix.empty())
          buffer = sanPrefix;                // Carry on and parse it.
        else
          continue;                          // Got to the end of them now!!!

      } // End remove 'nested' comments and variations.

      // Is it a move number only? (ie: no move on end).
      if (buffer[buffer.size()-1]=='.')
        continue;

      // BUG?: Is it a 'strange' '$' symbol (jatsmak2 games).
      if (buffer[0]=='$')
        continue;

      // If it's a '}' or a ')', it is terminating a non-starting commment- fix.
      // Hacked, but try to continue parsing - rather than exit.
      if (buffer[0]=='}' || buffer[0]==')') {
        cout << "Unstarted ()'s/{}'s. Ignoreing and attempting to continuing..."
             << endl;
        cout << gameText << endl << endl;
        continue;
      }

      // Lets see if it its a no result game (Not really an error).
      for (size_t i=0;i<buffer.size();i++) {
        if (buffer[i]=='*') {
          numTries=2;
          break;
        }
      }
      if (numTries==2) {                    // 2003: BUG, must check...
        initAll();
        fenTaggFound=false;
        continue;
      }

          // BUG?: Lets see if the game was aborted (Crafty PGN file...).
          // + If we find the start of a FEN tagg, before then game result, error.
          if (buffer[0]=='[' || buffer.compare(0, 7, "ABORTED")==0
              || buffer.compare(0, 7, "Aborted")==0
              || buffer.compare(0, 7, "aborted")==0) {
            initAll();
            numTries=2;
            fenTaggFound=false;
            continue;
          }

      // Is it a win, loss or draw? [BUG: Make sure you cast the size()!].
      result=-2;                               // No result found yet.
      for (size_t i=0;i<buffer.size()-2;i++) {
        if (buffer[i]=='1' && buffer[i+1]=='-' && buffer[i+2]=='0') {
          result=1;
          break;
        }
        else if (buffer[i]=='0' && buffer[i+1]=='-' && buffer[i+2]=='1') {
          result=-1;
          break;
        }
        else if (i<buffer.size()-6
                 && buffer[i]=='1' && buffer[i+1]=='/' && buffer[i+2]=='2'
                 && buffer[i+3]=='-' && buffer[i+4]=='1' && buffer[i+5]=='/'
                 && buffer[i+6]=='2') {
          result=0;
          break;
        }
      }

      // Did we find a result?
      if (result!=-2) {

        // Inform it we succeed on the second attempt (without nesting).
        if (numTries==1) { 
          cout << "*Succeeded* on 2nd pass (non-nested comments...)" << endl
               << endl;
        }

        // First check to see if their is a sensible number of moves in game.
        // ALSO: Is the fen tagg set, if so then just ignore the game.
        if (fenTaggFound==true || g_moveNum<3 || g_moveNum>800) {
          initAll();
          numTries=2;
          fenTaggFound=false;
          continue;
        }

        // Print the header (Binary).
        writeMinimalHeader(g_moveNum,result,outFile);

        // Print the moves in minimal format (Binary).
        for (int i=0;i<g_moveNum;i++)
          writeMinimalMove(g_movesMade[i],moveIsQuiescent[i],outFile);

        // Print the null terminator (for sort to use).
        outFile << '\0';

        initAll();
        numGames++;
        numTries=2;
        fenTaggFound=false;
        if ((numGames%PRINT_EVERY)==0)
          cout << "Converted: " << numGames << " games." << endl;
        continue;

      } // End found result.

        // If the fen tagg is set, just skip all the moves until we get result.
      if (fenTaggFound==true)
        continue;

      // If it is a move with a move-number on front, remove the move number.
      // Also, check (by working backwards), if it is a "N..." moves number.
      for (int i=static_cast<int>(buffer.size())-2;i>=0;i--) {
        if (buffer[i]=='.') {
          buffer = buffer.substr(i+1);
          break;
        }
      }

      // Must be a move then (The validity of the move is checked in here).
      // Create a mutable buffer for convertFromSAN
      std::vector<char> moveBuffer(buffer.begin(), buffer.end());
      moveBuffer.push_back('\0');
      if (convertFromSAN(moveBuffer.data(),moveChosen)==true) {
        cout << "Bad move found: " << buffer << endl;
        initAll();
        numErrors++;
        numTries=2;
        fenTaggFound=false;
        cout << gameText << endl << endl;
        continue;
      } else {
        // The move is OK, so use it.

        // Make the move now (The validity of the move is checked before).
        if (makeMove(moveChosen)==false) {
          cout << "Bad move found(???): " << buffer << endl;
          initAll();
          numErrors++;
          fenTaggFound=false;
          numTries=2;
          cout << gameText << endl << endl;
          continue;
        }

        // Find out if the move *LEADS TO* A Quiescent position.
#ifdef REAL_QUIESCENCE
        isQuiescentResult=isQuiescent();
#else
        isQuiescentResult=1;
#endif

        // See if the search timed-out, if so skip this game...
        if (isQuiescentResult==-1) {
          cout << "IsQuiescent() - Timed out..." << endl;
          initAll();
          numErrors++;
          numTries=2;
          fenTaggFound=false;
          cout << gameText << endl << endl;
          continue;
        }

        // If not timed out, then set the value.
        if (isQuiescentResult==true)
          moveIsQuiescent[g_moveNum-1]=true;
        else
          moveIsQuiescent[g_moveNum-1]=false;

        numPos++;

      } // End move ok, test for Quiescentness.

    } while (numTries<2 && static_cast<int>(inFile.tellg())!=fileLen && (inFile >> buffer));

  } // End for each game.

  Finished:

  cout << "Converted : " << numGames << endl;
  cout << "Errors    : " << numErrors << endl;
  cout << "TOTAL     : " << numGames+numErrors << endl;
  cout << "Positions : " << numPos << endl;

  // Close the files.
  inFile.close();
  outFile.close();

  return 0;

} // End main.