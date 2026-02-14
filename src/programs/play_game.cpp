// Include headers only - implementations linked separately
#include "../chess_engine/chess_engine.h"
#include "../search_engine/search_engine.h"
#include "../search_engine/search_config.h"
#include "../interface/interface.h"
#include "../core/cli_parser.h"

using namespace std;

// Target make the eval function more interesing for humans to play against.
constexpr double DEFAULT_RANDOM_SWING = 0.0;  // Alters evaluation +/- randomly.
constexpr double DEFAULT_SEARCH_TIME = 1.0;   // Can stll be changed in program.

// To be used if no filename is specified on the command line.
constexpr const char* DEFAULT_SET = "./evaluation_sets/best_so_far.set";

int main(int argc,char** argv)
{
  int    gameMode=COMPUTER_BLACK;           // Mode to play in.
  int    searchDepth=INFINITE_DEPTH;        // Search depth for computer (plys).
  double searchTime=DEFAULT_SEARCH_TIME;    // Assume infinite if depth set.
  bool   useBell=false;                     // Beep after computers move.

  const char* whiteSet=DEFAULT_SET;         // Use standard set by default.
  const char* blackSet=DEFAULT_SET;         // Use standard set by default.

  // Should we show thinking/output, or not?
  bool showOutput=true;                  // Show Output ON by default.
  bool showThinking=false;               // Show Thinking Off by default.

  // How many games should we play.
  int    numGamesToPlay=1;                  // Play 1, unless specified.

  // How much randomness to add.
  double randomSwing=DEFAULT_RANDOM_SWING;

  // The evaluation parameters to use.
  EvaluationParameters epw,epb;

  int    gameResult;                        // Reason game ended.

  // For keeping track of score when playing multiple games.
  int numWhiteWins=0;
  int numBlackWins=0;
  int numDraws=0;

  // Setup CLI parser
  CliParser parser("PlayChess", "Play chess against the engine");
  parser.addOption("mode", 'm', "Game mode (computer-black, computer-white, two-humans, two-computers)",
                   CliParser::OptionType::STRING, "computer-black");
  parser.addOption("white-set", 'w', "White evaluation set file",
                   CliParser::OptionType::STRING, DEFAULT_SET);
  parser.addOption("black-set", 'b', "Black evaluation set file",
                   CliParser::OptionType::STRING, DEFAULT_SET);
  parser.addOption("depth", 'd', "Search depth in plies (0=use time)",
                   CliParser::OptionType::INT, "0");
  parser.addOption("time", 't', "Search time per move (seconds)",
                   CliParser::OptionType::DOUBLE, "1.0");
  parser.addOption("games", 'n', "Number of games to play",
                   CliParser::OptionType::INT, "1");
  parser.addOption("random-swing", 'r', "Random evaluation swing",
                   CliParser::OptionType::DOUBLE, "0.0");
  parser.addOption("thinking", '\0', "Show thinking output",
                   CliParser::OptionType::BOOL, nullptr);
  parser.addOption("quiet", 'q', "Minimal output",
                   CliParser::OptionType::BOOL, nullptr);
  parser.addOption("bell", '\0', "Beep after computer move",
                   CliParser::OptionType::BOOL, nullptr);
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

  // Get mode
  const char* modeStr = parser.getString("mode");
  if (strcmp(modeStr, "computer-black") == 0) {
    gameMode = COMPUTER_BLACK;
  } else if (strcmp(modeStr, "computer-white") == 0) {
    gameMode = COMPUTER_WHITE;
  } else if (strcmp(modeStr, "two-humans") == 0) {
    gameMode = TWO_HUMANS;
  } else if (strcmp(modeStr, "two-computers") == 0) {
    gameMode = TWO_COMPUTERS;
  } else {
    cerr << "PlayChess: invalid mode: " << modeStr << endl;
    return 1;
  }

  // Get evaluation set files
  whiteSet = parser.getString("white-set");
  blackSet = parser.getString("black-set");

  // Get search parameters
  searchDepth = parser.getInt("depth");
  searchTime = parser.getDouble("time");
  numGamesToPlay = parser.getInt("games");
  randomSwing = parser.getDouble("random-swing");

  // Set timing mode
  if (parser.getBool("cpu-time")) {
    g_timingMode = TimingMode::CPU_TIME;
  }

  // Validate parameters
  if (searchDepth < 0) {
    cerr << "PlayChess: search depth must be >= 0" << endl;
    return 1;
  }
  if (searchTime <= 0.0) {
    cerr << "PlayChess: search time must be > 0" << endl;
    return 1;
  }
  if (numGamesToPlay < 1) {
    cerr << "PlayChess: must play at least 1 game" << endl;
    return 1;
  }
  if (randomSwing < 0) {
    cerr << "PlayChess: random swing must be >= 0" << endl;
    return 1;
  }

  // Parse hash table size
  int hashSizeMb = parser.getInt("hash-size");

  // Validate hash size
  if (hashSizeMb < 1 || hashSizeMb > 65536) {
    cerr << "PlayChess: hash-size must be between 1 and 65536 MB" << endl;
    return 1;
  }

  // Configure hash table size (other parameters use defaults)
  g_searchConfig.hashSizeMB = static_cast<size_t>(hashSizeMb);
  g_searchConfig.computeHashSize();

  if (!g_searchConfig.validate()) {
    cerr << "PlayChess: invalid search configuration" << endl;
    return 1;
  }

  // Initialize global arrays with the configured parameters
  initGlobals(g_searchConfig);

  // Get boolean flags
  showThinking = parser.getBool("thinking");
  showOutput = !parser.getBool("quiet");
  useBell = parser.getBool("bell");

  // If depth is specified (> 0), use infinite time
  if (searchDepth > 0) {
    searchTime = INFINITE_TIME;
  } else {
    searchDepth = INFINITE_DEPTH;
  }

  // Load in the requested evalution sets, testing for errors.
  // NOTE: A random one will be created in can't load.
  if (epw.load(whiteSet)==true)
    FATAL_ERROR("Could not load WHITE set.");
  if (epb.load(blackSet)==true)
    FATAL_ERROR("Could not load Black set.");

  // Print the info if output is on.
  cout << endl << "CHESS - Juk Armstrong (1998-2003)" 
               << " (Version: " << VERSION << ')' << endl << endl;

  // Print the options we are using.
  if (gameMode==COMPUTER_BLACK)
    cout << "Mode of play  : computer-black" << endl;
  else if (gameMode==COMPUTER_WHITE)
    cout << "Mode of play  : computer-white" << endl;
  else if (gameMode==TWO_HUMANS)
    cout << "Mode of play  : two-humans" << endl;
  else if (gameMode==TWO_COMPUTERS)
    cout << "Mode of play  : two-computers" << endl;
  cout << "White's Set   : " << whiteSet << endl;
  cout << "Black's Set   : " << blackSet  << endl;
  if (showOutput==false)
    cout << "Show Output   : OFF" << endl;
  else
    cout << "Show Output   : ON" << endl;
  if (showThinking==false)
    cout << "Show Thinking : OFF" << endl;
  else
    cout << "Show Thinking : ON" << endl;
  if (searchTime==INFINITE_TIME)
    cout << "Search Depth  : " << searchDepth << " ply" << endl;
  else
    cout << "Search Time   : " << searchTime << " seconds" << endl;
  cout << "Random Swing  : " << randomSwing << " points" << endl;
  if (useBell==false)
    cout << "Move Bell     : OFF" << endl;
  else
    cout << "Move Bell     : ON" << endl;
  cout << "Hash Memory   : " << g_searchConfig.getHashMemoryMB() << " MB (" << g_searchConfig.numHashSlots << " slots)" << endl;
  cout << "Games to Play : " << numGamesToPlay << endl;

  // Print multigame header.
  if (numGamesToPlay>1 && showOutput==false) {
    cout << endl << "Individual Game Results" << endl
         << "=======================" << endl;
  }

  // Play game(s).
  for (int gameNum=0;gameNum<numGamesToPlay;gameNum++) {

    // Print game prelude.
    if (numGamesToPlay>1 && showOutput==false) {
      cout << gameNum+1 << ": ";
      cout.flush();
    }

    // Play the game.
    gameResult=playGame(gameMode,searchDepth,searchTime,useBell,showOutput,
                        showThinking,randomSwing,epw,epb);

    // See why the game ended -if showOutput is off then this show also.
    if (gameResult==WHITE_MATES) {
      cout << "White Mates..." << endl;
      numWhiteWins++;
    }
    else if (gameResult==BLACK_MATES) {
      cout << "Black Mates..." << endl;
      numBlackWins++;
    }
    else if (gameResult==STALEMATE) {
      cout << "Stalemate..." << endl;
      numDraws++;
    }
    else if (gameResult==FIFTY_MOVE_RULE) {
      cout << "Fifty-Move-Rule Draw..." << endl;
      numDraws++;
    }
    else if (gameResult==THREE_IDENTICLE_POS) {
      cout << "Draw by Repetition of three Identicle Positions..." << endl;
      numDraws++;
    }
    else if (gameResult==NOT_ENOUGH_MATERIAL) {
      cout << "Insuficient Material for either side to force Mate..." << endl;
      numDraws++;
    }
    else if (gameResult==WHITE_RETIRES) {
      cout << endl << "White has retired..." << endl;
      numBlackWins++;
    }
    else if (gameResult==BLACK_RETIRES) {
      cout << endl << "Black has retired..." << endl;
      numWhiteWins++;
    }

  }

  // Print the final stats if we were playing a tournament.
  if (numGamesToPlay>1) {
    cout << endl << "FINAL RESULTS" << endl;
    cout << "=============" << endl;
    cout << "White Wins : " << numWhiteWins << endl;
    cout << "Black Wins : " << numBlackWins << endl;
    cout << "Draws      : " << numDraws << endl << endl;
    cout << "WHITE      : " << ((static_cast<double>(numWhiteWins)+(static_cast<double>(numDraws)/2.0))
                                /static_cast<double>(numGamesToPlay))*100.0 << '%' << endl;
    cout << "BLACK      : " << ((static_cast<double>(numBlackWins)+(static_cast<double>(numDraws)/2.0))
                                /static_cast<double>(numGamesToPlay))*100.0 << '%' << endl;
  }

  // All O.K.
  return 0;

} // End main.