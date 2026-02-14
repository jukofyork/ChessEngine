// **************************************************************************
// *                          INTERFACE FUNCTIONS                           *
// **************************************************************************

#include "interface.h"                       // Include globals, params ect.
#include "../search_engine/search_engine.h"
#include "../core/timing.h"

#include <cstdint>
#include <iomanip>

using namespace std;

void printBoard(int sideUpBoard)
{ // Prints the current board.

  // The piece letters for printing.
  const char PIECE_CHAR[6]={'P','N','B','R','Q','k'};

  // Print with white up the board.
  if (sideUpBoard==WHITE) {
    cout << endl << "8|";
    for (int i=0;i<BOARD_SQUARES;i++) {
      if (g_gameHistory[g_moveNum].colour[i]==NONE) {
        if ((i/8)%2==0) {
          if (i%2==0)
            cout << " .";
          else
            cout << " #";
        }
        else {
          if (i%2==0)
            cout << " #";
          else
            cout << " .";
        }
      }
      else if (g_gameHistory[g_moveNum].colour[i]==WHITE)
        cout << ' ' << static_cast<char>(PIECE_CHAR[g_gameHistory[g_moveNum].piece[i]]);
      else if (g_gameHistory[g_moveNum].colour[i]==BLACK)
        cout << ' ' << static_cast<char>(PIECE_CHAR[g_gameHistory[g_moveNum].piece[i]]
                               +('a'-'A'));

      if ((i+1)%8==0 && i!=63)
        cout << endl << 7-getRank(i) << '|';
    }
    cout << endl << "   ---------------" << endl;
    cout << "   a b c d e f g h" << endl << endl;
  }

  // Print black up the board.
  else {
    cout << endl << "1|";
    for (int i=63;i>=0;i--) {
      if (g_gameHistory[g_moveNum].colour[i]==NONE) {
        if ((i/8)%2==0) {
          if (i%2==0)
            cout << " .";
          else
            cout << " #";
        }
        else {
          if (i%2==0)
            cout << " #";
          else
            cout << " .";
        }
      }
      else if (g_gameHistory[g_moveNum].colour[i]==WHITE)
        cout << ' ' << static_cast<char>(PIECE_CHAR[g_gameHistory[g_moveNum].piece[i]]);
      else if (g_gameHistory[g_moveNum].colour[i]==BLACK)
        cout << ' ' << static_cast<char>(PIECE_CHAR[g_gameHistory[g_moveNum].piece[i]]
                               +('a'-'A'));

      if ((i)%8==0 && i!=0 && i!=63)
        cout << endl << 9-getRank(i) << '|';
    }
    cout << endl << "   ---------------" << endl;
    cout << "   h g f e d c b a" << endl << endl;
  }

  // Print if the move discloses check.
  if (g_gameHistory[g_moveNum].inCheck)
    cout << "Check..." << endl << endl;

} // End printBoard.

// =============================================================================

void printMove(const MoveStruct &move)
{ // Print a move (with promotion info if needed!)
  // Check '+' will need appending outside...
  cout << static_cast<char>(getFile(move.source)+'a')
       << 8-getRank(move.source);
  if (move.type&CAPTURE)
    cout << 'x';
  else if (move.type&CASTLE)
    cout << '*';
  else
    cout << '-';
  cout << static_cast<char>(getFile(move.target)+'a')
       << 8-getRank(move.target);
  if (move.type&PROMOTION) {
    if (move.promote==QUEEN)
      cout << "=Q";
    else if (move.promote==ROOK)
      cout << "=R";
    else if (move.promote==BISHOP)
      cout << "=B";
    else if (move.promote==KNIGHT)
      cout << "=N";
  }
} // End printMove.

// -----------------------------------------------------------------------------

void writeMinimalHeader(int numMoves,int gameResult,ofstream &outFile)
{ // Write a game header using only 2 bytes (in binary file).
  // See interface.h for detailed format specification.
  //
  // ENCODING:
  // Bit 0: Always set to 1 to avoid null bytes
  // Bits 1-13: NumMoves (13 bits = max 8191 moves)
  // Bits 14-15: GameResult + 2 (offset to avoid 0, range 2-5 for results -2 to +1)
  //
  // Example: 100 moves, white wins (result=0)
  //   Raw:    0b0000000001100100 (100 in binary)
  //   Shifted: 0b00000011001000 (<< 1)
  //   Result: 0b10 (result 0 + 2 = 2)
  //   Final:  0b10000000011001001 = 0x80C9

  uint16_t gameHeader;

  // Encode the header:
  // 1. Set bit 0 to 1 (avoid null byte)
  // 2. Shift moves left by 1 and mask to 13 bits
  // 3. Add offset to result and shift to bits 14-15
  gameHeader = 1
             | ((static_cast<uint16_t>(numMoves) & HEADER_MOVE_MASK) << HEADER_MOVE_SHIFT)
             | ((static_cast<uint16_t>(gameResult) + HEADER_RESULT_OFFSET) << HEADER_RESULT_SHIFT);

  // SAFETY CHECK: Verify no null bytes were produced
  // This can happen if NumMoves is 0 and result is -2 (both would create 0 bytes)
  if (reinterpret_cast<char*>(&gameHeader)[0] == 0 || reinterpret_cast<char*>(&gameHeader)[1] == 0) {
    FATAL_ERROR("writeMinimalHeader: Would write null byte to binary DB (NumMoves="
                + std::to_string(numMoves) + ", Result=" + std::to_string(gameResult) + ")");
  }

  // Write 2 bytes in native endianness
  outFile.write(reinterpret_cast<char*>(&gameHeader), 2);

} // End writeMinimalHeader.

// -----------------------------------------------------------------------------

void readMinimalHeader(int &numMoves, int &gameResult, ifstream &inFile)
{ // Read a game header using only 2 bytes (in binary file).
  // See interface.h for detailed format specification and writeMinimalHeader() for encoding.
  //
  // DECODING:
  // 1. Read 2 bytes as uint16_t
  // 2. Extract moves: (value >> 1) & 0x1FFF (mask 13 bits)
  // 3. Extract result: ((value >> 14) & 0x03) - 2 (reverse the offset)

  uint16_t gameHeader;

  // Read 2 bytes from file
  inFile.read(reinterpret_cast<char*>(&gameHeader), 2);

  // Decode the number of moves:
  // - Shift right by 1 to remove padding bit
  // - Mask with 0x1FFF to extract 13 bits
  numMoves = static_cast<int>((gameHeader >> HEADER_MOVE_SHIFT) & HEADER_MOVE_MASK);

  // Decode the game result:
  // - Shift right by 14 to get result bits
  // - Mask with 0x03 to extract 2 bits
  // - Subtract offset to get original value (-2 to +1)
  gameResult = static_cast<int>((gameHeader >> HEADER_RESULT_SHIFT) & ((1 << HEADER_RESULT_BITS) - 1))
               - HEADER_RESULT_OFFSET;

} // End readMinimalHeader.

// -----------------------------------------------------------------------------

void writeMinimalMove(const MoveStruct &move, bool isQuiescent, ofstream &outFile)
{ // Write a move using only 3 bytes (in binary file).
  // Also records whether the move leads to a quiescent position.
  // See interface.h for detailed format specification.
  //
  // ENCODING (24 bits total):
  // Byte 0 (bits 0-7):  [Source:6 bits][Pad:1 bit]
  // Byte 1 (bits 8-15): [Target:6 bits][Pad:1 bit][Type:1 bit]
  // Byte 2 (bits 16-23):[Type:5 bits][Promote-1:2 bits][Quiescent:1 bit]
  //
  // PROMOTION ENCODING:
  // Promotion piece is stored as (piece - 1) to fit in 2 bits:
  //   0 (none) -> not applicable (checked via PROMOTION flag)
  //   1 (N)    -> stored as 0
  //   2 (B)    -> stored as 1
  //   3 (R)    -> stored as 2
  //   4 (Q)    -> stored as 3

  uint32_t binMove = 0;

  // Build the 24-bit value by OR-ing together all fields at their bit positions
  binMove = static_cast<uint32_t>(move.source)                          // Bits 0-5: Source
          | (static_cast<uint32_t>(1) << MOVE_PAD_BIT_6)                 // Bit 6: Padding
          | (static_cast<uint32_t>(move.target) << MOVE_TARGET_SHIFT)    // Bits 7-12: Target
          | (static_cast<uint32_t>(1) << MOVE_PAD_BIT_13)                // Bit 13: Padding
          | (static_cast<uint32_t>(move.type) << MOVE_TYPE_SHIFT)        // Bits 14-19: Type
          | (static_cast<uint32_t>((move.type & PROMOTION) ? (move.promote - 1) : 0) << MOVE_PROMOTE_SHIFT) // Bits 20-21: Promote-1 (only if promotion)
          | (static_cast<uint32_t>(isQuiescent) << MOVE_QUISCENT_SHIFT)  // Bit 22: Quiescent
          | (static_cast<uint32_t>(1) << MOVE_PAD_BIT_23);               // Bit 23: Padding

  // SAFETY CHECK: Verify no null bytes in any position
  // Each byte should have at least one bit set (via padding or data)
  if (reinterpret_cast<char*>(&binMove)[0] == 0 ||
      reinterpret_cast<char*>(&binMove)[1] == 0 ||
      reinterpret_cast<char*>(&binMove)[2] == 0) {
    FATAL_ERROR("writeMinimalMove: Would write null byte to binary DB");
  }

  // Write exactly 3 bytes
  outFile.write(reinterpret_cast<char*>(&binMove), 3);

} // End writeMinimalMove.

// -----------------------------------------------------------------------------

void readMinimalMove(MoveStruct &move, uint8_t &isQuiescent, ifstream &inFile)
{ // Read a move from 3 bytes in binary file.
  // Also returns whether the move leads to a quiescent position.
  // See interface.h for detailed format specification and writeMinimalMove() for encoding.
  //
  // DECODING (24 bits total):
  // Byte 0 (bits 0-7):  Extract Source (mask 0x3F, no shift needed)
  // Byte 1 (bits 8-15): Extract Target (shift >> 7, mask 0x3F)
  // Byte 2 (bits 16-23): Extract Type (shift >> 14, mask 0x3F)
  //                      Extract Promote-1 (shift >> 20, mask 0x03), then add 1
  //                      Extract Quiescent (shift >> 22, mask 0x01)

  uint32_t binMove;

  // Read 3 bytes from file
  inFile.read(reinterpret_cast<char*>(&binMove), 3);

  // Decode source square: mask bits 0-5
  move.source = static_cast<int8_t>(binMove & MOVE_SOURCE_MASK);

  // Decode target square: shift to bits 0-5, then mask
  move.target = static_cast<int8_t>((binMove >> MOVE_TARGET_SHIFT) & MOVE_TARGET_MASK);

  // Decode move type: shift to bits 0-5, then mask
  move.type = static_cast<uint8_t>((binMove >> MOVE_TYPE_SHIFT) & MOVE_TYPE_MASK);

  // Decode promotion piece (if this is a promotion move)
  if (move.type & PROMOTION) {
    // Stored as (piece - 1), so add 1 to get actual piece
    // 0->1 (Knight), 1->2 (Bishop), 2->3 (Rook), 3->4 (Queen)
    move.promote = static_cast<uint8_t>(((binMove >> MOVE_PROMOTE_SHIFT) & MOVE_PROMOTE_MASK) + 1);
  } else {
    move.promote = NO_PROMOTION;  // No promotion
  }

  // Decode quiescent flag: shift to bit 0, mask with 1
  isQuiescent = static_cast<uint8_t>((binMove >> MOVE_QUISCENT_SHIFT) & 1);

} // End readMinimalMove.

// =============================================================================

void printLine(SearchData &sd,MoveStruct &line,int moveScore,char boundType)
{ // Print a line (usually the PV)  and other stuff for thinking output.
  // Bound type may be:
  // '&' = New root move found on search.
  // '.' = Final move, ply completed.
  // '%' = Final move, ply partially completed (move usable though!).

  int j;

  // This is the list of Moves/Captures generated from this state.
  MoveList moves;

  // Print the other stuff.
  if (!isMateScore(moveScore)) {
    std::cout << "| " << std::setw(2) << sd.iterDepth << boundType << " | "
              << std::fixed << std::setprecision(2) << std::setw(9)
              << timeDiffToSeconds(sd.startTime, getTime()) << " | "
              << std::setw(10) << sd.totalNodesSearched << " | "
              << std::setw(8) << std::setprecision(4)
              << (double)moveScore/(double)PIECE_VALUE[PAWN] << " |";
  }
  else {
    std::cout << "| " << std::setw(2) << sd.iterDepth << boundType << " | "
              << std::fixed << std::setprecision(2) << std::setw(9)
              << timeDiffToSeconds(sd.startTime, getTime()) << " | "
              << std::setw(10) << sd.totalNodesSearched << " | "
              << std::setw(7) << getMateIn(moveScore) << "# |";
  }

  // Print the PV from the hash if using it!.
  MoveStruct pvMove;
  int tempScore;
  HashKey nextKey;

  // Get the first move from the PV[0][0] slot.
  cout << ' ';
  printMove(line);

  // Make the move and see if we are in check.
  makeMove(line);
  if (g_gameHistory[g_moveNum].inCheck) {

    // Is it a matting move or check?
    //if (labs(Score)>(WIN_SCORE-100))
    //  cout << '#';
    //else
      cout << '+';

  }

  // Make the initial move as it won't of been stored in the hash yet (in PV).
  for (j=1;;j++) {

    if (ttGet(sd,0,0,tempScore,pvMove,nextKey)==0) {
      cout << " ...";
      break;
    }

    // End if move sequence.
    if (pvMove.source==-1)
      break;

    // Test for repetition to stop cycles from causeing crashes!
    if (g_currentState->isDraw==true) {
      cout << " ... (HASH DRAW)";
      break;
    }

    // Test for repetition to stop cycles from causeing crashes!
    if (testSingleRepetition(g_moveNum-j)==true) {
      cout << " ... (HASH CYCLE)";
      break;
    }

    // Generate moves (this won't interfere as were not at search's ply.
    genMoves(moves);

    // See if the move we got is correct?
    bool found=false;
    for (int i=0;i<moves.numMoves;i++) {
      if (moves.moves[i].source==pvMove.source &&
          moves.moves[i].target==pvMove.target &&
          moves.moves[i].type==pvMove.type &&
          moves.moves[i].promote==pvMove.promote) {
        found=true;
        break;
      }
    }

    // If not found then somethings wrong, and the move should be ignored.
    if (found==false) {
      cout << " ... (HASH MOVE MISSING!)";      // Imposible move.
      break;
    }

    // Make the move and check we can.
    if (!makeMove(pvMove)) {
      cout << " ... (HASH MOVE ILLEGAL!)";      // Ilegal move.
      break;
    }

    // Make sure the keys are the same.
    if (g_currentState->key!=nextKey) {
      cout << " ... (KEY CLASH!)";
      break;
    }

    // Print the move.
    if (j==sd.iterDepth && boundType!='%')
      cout << " :";
    cout << ' ';
    printMove(pvMove);

    // See if we are in check.
    if (g_gameHistory[g_moveNum].inCheck) {

      // Is it a matting move?
      //if (j==(LineLength-1) && labs(Score)>(WIN_SCORE-100))
      //  cout << '#';
      //else
        cout << '+';

    }

  }

  // Show extention info.
  if (boundType!='%' && j<sd.iterDepth)
    cout << " (-" << sd.iterDepth-j << ')';
  else if (boundType!='%' && j>sd.iterDepth)
    cout << " (+" << j-sd.iterDepth << ')';
  cout << endl;

  // Take all the moves back.
  for (;j>0;j--)
    takeMoveBack();

} // End printLine.

// **************************************************************************

int playGame(int modeOfPlay,int searchDepth,double maxTimeSeconds,bool useBell,
             bool showOutput,bool showThinking,double randomSwing,
             const EvaluationParameters &epw,const EvaluationParameters &epb)
{ // This function basically is  an infinite loop that either calls Think() 
  // when it's the computer's turn to move or prompts the user for a 
  // command (and deciphers it).
  // Returns the reason for ending.

  int ComputerSide=NONE;       // Colour of the computer side, may be NONE.

  std::string StringBuffer;      // String read from user.
  int Source,Target;           // User's move.
  int moveIndex;                   // Move chosen converted into genertor index.

  bool Found;                  // Used for finding if a move exists.
  int  NumFound;               // How many successor moves found (1=Instant)

  // This is the list of Moves/Captures generated from this state.
  MoveList Moves;

  // For holding the current best move chosen by the computer (to be returned).
  MoveStruct ComputersMove;    // Returned from Think()


  // Set up the ComputerSide according to input parameter modeOfPlay.
  if (modeOfPlay==TWO_HUMANS)
    ComputerSide=NONE;
  else if (modeOfPlay==COMPUTER_WHITE)
    ComputerSide=WHITE;
  else if (modeOfPlay==COMPUTER_BLACK)
    ComputerSide=BLACK;
  else if (modeOfPlay==TWO_COMPUTERS)
    ComputerSide=NONE;          // Both sides are computers, so no single computer side

  // Init all a data to a new game and gererate the initial set of moves.
  initAll();
  genMoves(Moves);

  // Loop until game over.
  for (;;) {

    // Print the board so we can see it.
    if (showOutput) {
      if (ComputerSide==WHITE && modeOfPlay!=TWO_COMPUTERS)
        printBoard(BLACK);           // If human vs white the print black up.
      else
        printBoard(WHITE);           // Print the board for user to see.
    }

    // Check for games 50-move-rule ending condition.
    if (g_gameHistory[g_moveNum].fiftyCounter>=50)
      return FIFTY_MOVE_RULE;        // 50 moves and no pawn move/capture.

    // Check for repetition draw.
    if (testRepetition())
      return THREE_IDENTICLE_POS;    // Have had three identicle positions.

    if (testNotEnoughMaterial())
      return NOT_ENOUGH_MATERIAL;    // Not enough to win.

    // No Moves found AT NOT yet.
    Found=false;
    NumFound=0;

    // Loop through the generated moves to see if ANY are legal.
    // Also count how many are legal to see if only one is, then the computer
    // does not need to think (ie just does the move!).
    for (moveIndex=0;moveIndex<Moves.numMoves;moveIndex++) {
      if (makeMove(Moves.moves[moveIndex])) {
        takeMoveBack();              // Take it back as were only testing.
        Found=true;                  // Have found one.
        NumFound++;                  // One more move found.
      }
    }

    // If no valid moves found, the games over for one reson or another.
    if (!Found) {

      // Allow the user to take back a move at a terminal node.
      /*
      if (ModeOfPlay!=TWO_COMPUTERS) {
        MainCanvas.SetCursor(SKULL_CURSOR);
        do 
          MainCanvas.NextEvent(true,Evt);
        while (!ProcessEvent(Evt));
        MainCanvas.SetCursor(WATCH_CURSOR);
        if (MoveString[0]=='t' || MoveString[0]=='T') {
          TakeMoveBack();                 // Take back computer's move.
          TakeMoveBack();                 // Take back your move.
          cout << "(Terminal) Move has been taken back." << endl;
          GenMoves(Moves);
          continue;
        }
      }
	  */

      if (g_gameHistory[g_moveNum].inCheck) {
        if (getOtherSide(g_currentSide)==BLACK)
          return BLACK_MATES;         // BLACK has Mated.
        else
          return WHITE_MATES;        // White has Mated.
      }
      else {
        return STALEMATE;            // Not in check & no moves - Stalemate.
      }
    }

    // Print move number an person to move, after game end checking.
    if (showOutput) {
      if (g_currentSide==WHITE)
        cout << "Move number " << g_moveNum << " - White to move next." << endl
             << endl;
      else
        cout << "Move number " << g_moveNum << " - Black to move next." << endl
             << endl;
    }

    // Check if it's the computer's turn.
    if (modeOfPlay==TWO_COMPUTERS || g_currentSide==ComputerSide) {

      // Only bother if there are more than one move to choose from.
      //if (NumFound==1) {
      //  if (ShowOutput)
      //    cout << "No Thinking Needed - Only 1 Possible Move..." << endl;

        // Just do the only move found.
      //  for (moveIndex=0;moveIndex<Moves.NumMoves;moveIndex++) {

          // Keep checking until the legal one is found, then print it.
      //    if (MakeMove(Moves.Move[moveIndex])) {
      //      TakeMoveBack();            // Take back for later (after printing)
      //      ComputersMove=Moves.Move[moveIndex]; // Save it.
      //      break;                     // As there is only one move! 
      //    }

      //  }

      //}

      // Else search for a move and put it in ComputersMove.
      //else {

        // Call think.
        if (g_currentSide==WHITE) {
          ComputersMove=think(searchDepth,maxTimeSeconds,showOutput,
                              showThinking,randomSwing,epw);
        }
        else {
          ComputersMove=think(searchDepth,maxTimeSeconds,showOutput,
                              showThinking,randomSwing,epb);
        }

        if (useBell==true)
          cout << '\a' << endl;
      //}

      // Print the move the computer chose.
      if (showOutput) {
        // Move format is always 4 chars (e.g., "e2e4") + null terminator
        std::cout << '\n' << "Move: "
                  << static_cast<char>(getFile(ComputersMove.source)+'a')
                  << (8-getRank(ComputersMove.source))
                  << static_cast<char>(getFile(ComputersMove.target)+'a')
                  << (8-getRank(ComputersMove.target))
                  << '.' << '\n';
      }

      makeMove(ComputersMove);

      genMoves(Moves);                 // Re-Generate the moves now.
      continue;                        // Skip out for next move.
    }

    // Print the Move Enetering Prompt. (Alwyas show Output from here on!)
    cout << "Enter move ('?'=Help): ";   // Print Prompt.

    // If a key was pressed, goto console mode.
    cin >> StringBuffer;                 // Get move or retire command.

    // First check if the user has Retired.
    if (StringBuffer=="Q" || StringBuffer=="q") {
      if (g_currentSide==WHITE)
        return WHITE_RETIRES;
      else
        return BLACK_RETIRES;
    }

    // Check if the user wants to take a (2-ply) move back.
    if (StringBuffer=="t" || StringBuffer=="T") {

      // Take the 'chess' Move (ie: two plys!) back, if moves to be taken back.
      if (g_moveNum<2) {
        cout << "\aNo (2-Ply) moves to take back." << endl;
        continue;
      }
      else {
        takeMoveBack();                 // Take back computer's move.
        takeMoveBack();                 // Take back your move.
        cout << "Move has been taken back." << endl;
      }

      // Generate the moves again.
      genMoves(Moves);
      continue;

    }

    // Has the user asked for help.
    if (StringBuffer=="?") {
      cout << endl << " * Enter moves in coord notation (eg. e2e4)." << endl;
      cout << " * When promoting a pawn use 5th charater as the target peice:"
           << endl;
      cout << "   k=Knight / b=Bishop / r=Rook / q=Queen (Default)" 
           << endl;
      cout << " * Type 'q' to retire and lose the current game." << endl;
      cout << " * Type 't' to take back a (2-ply) move." << endl;
      continue;                        // Skip as not a move.
    }

    // Attempt to parse the move eneterd and see if it is legal. 
    Source=StringBuffer[0]-'a';          // Get first Letter.
    Source+=8*(8-(StringBuffer[1]-'0')); // Get first Digit.
    Target=StringBuffer[2]-'a';            // Get second Letter.
    Target+=8*(8-(StringBuffer[3]-'0'));   // Get second Digit.

    // Move NOT found yet.
    Found=false;

    // Loop through the generated moves to see if it's legal.
    for (moveIndex=0;moveIndex<Moves.numMoves;moveIndex++) {
      if (Moves.moves[moveIndex].source==Source 
          && Moves.moves[moveIndex].target==Target) {

        Found=true;                     // Have now found move, it's legal.

        // Get the promotion piece from after the move string.
        // Defaults to a Queen.
        if (Moves.moves[moveIndex].type&32) {
          if (StringBuffer[4]=='N' || StringBuffer[4]=='n') {}   // Knight.
          else if (StringBuffer[4]=='B' || StringBuffer[4]=='b') // Bishop.
            moveIndex+=1;
          else if (StringBuffer[4]=='R' || StringBuffer[4]=='r') // Rook.
            moveIndex+=2;
          else                                             // Queen (Default)
            moveIndex+=3;
        }
        break;
      }
    }

    // If we have not found a valid move then re-try. 
    if (!Found || !makeMove(Moves.moves[moveIndex]))
      cout << "\aIlegal Move. Please try again." << endl;

    // Generate the moves again. (NEEDED ALWAYS?)
    genMoves(Moves);

  } // End for-ever run game running loop.

} // End playGame.

