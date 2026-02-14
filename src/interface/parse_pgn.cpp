// 2003: Have now made the code work for the following:
//       - Castles that use 0-0-0 or 0-0 instead of O-O-O or O-O.
//       - Promotions that use ...=Q, or ...Q (with/without =).
//       - The code to find 'strange' full algebraic moves had bug & works now.
// 2003b: Have now completely re-written the function, and it works much better
//        now.
//       - There are no problems with fixed length(s) testing and moves with ?
//         or ! on end etc, should not cause problems now... Well tested.
//       - Have included a 'prefix' number for error, to show where it was too.
//         May not be needed now, and should be removed when used for other
//         parsing (rather than convert_from_pgn.cc).
//       - The validity of all moves is checked in here to with a MakeMove() and
//         TakeBackMove() call.

#include <cstring>
#include "../chess_engine/chess_engine.h"
#include "../chess_engine/globals.h"

using namespace std;

bool convertFromSAN(char* sanMove,MoveStruct& algMove)
{ // This is the better, more reliable version.

  // The move list.
  MoveList moves;

  // Used to check if their is a file and/or rank specifier for a piece move.
  char fileSpec='0',rankSpec='0';  // Init to safe values (none found yet).

  // Used to find what the piece is we are looking for.
  int desiredPeice;

  // Generate the moves for the current position.
  genMoves(moves);

  // 1. Check to see if it's a castling move.
  //    NOTE: Also check to see if 0's have been used instead of O's.
  if ((sanMove[0]=='O' && sanMove[1]=='-' && sanMove[2]=='O'
       && sanMove[3]=='-' && sanMove[4]=='O')
      || (sanMove[0]=='0' && sanMove[1]=='-' && sanMove[2]=='0'
          && sanMove[3]=='-' && sanMove[4]=='0')) {

    // If the king is on the 'e' file then this is the queen side castle.
    if (getFile(g_currentState->kingSquare[g_currentSide])==4) {
      algMove.source=g_currentState->kingSquare[g_currentSide];
      algMove.target=g_currentState->kingSquare[g_currentSide]-2;
      algMove.type=CASTLE;
    }

    // Else it is the other way around (ie King on 'd' file).
    else {
      algMove.source=g_currentState->kingSquare[g_currentSide];
      algMove.target=g_currentState->kingSquare[g_currentSide]+2;
      algMove.type=CASTLE;
    }

    // Check to see if the move is legal.
    if (makeMove(algMove)) {
      takeMoveBack();
      return false;                            // Move O.K.
    }
    else {
      cout << "2: ";
      return true;                             // Error - not legal.
    }

  }

  // 2. Check to see if it's a king side castling move.
  //    NOTE: Also check to see if 0's have been used instead of O's.
  if ((sanMove[0]=='O' && sanMove[1]=='-' && sanMove[2]=='O')
      || (sanMove[0]=='0' && sanMove[1]=='-' && sanMove[2]=='0')) {

    // If the king is on the 'e' file then this is the king side castle.
    if (getFile(g_currentState->kingSquare[g_currentSide])==4) {
      algMove.source=g_currentState->kingSquare[g_currentSide];
      algMove.target=g_currentState->kingSquare[g_currentSide]+2;
      algMove.type=CASTLE;
    }

    // Else it is the other way around (ie King on 'd' file).
    else {
      algMove.source=g_currentState->kingSquare[g_currentSide];
      algMove.target=g_currentState->kingSquare[g_currentSide]-2;
      algMove.type=CASTLE;
    }

    // Check to see if the move is legal.
    if (makeMove(algMove)) {
      takeMoveBack();
      return false;                            // Move O.K.
    }
    else {
      cout << "3: ";
      return true;                             // Error - not legal.
    }

  }

  // 3. Check for a '=' (promotion) move.
  for (int i=0;sanMove[i]!='\0';i++) {

    // Look for the '=' character.
    if (sanMove[i]=='=') {

      // Find out what piece to promote to.
      if (sanMove[i+1]=='N' || sanMove[i+1]=='n')
        algMove.promote=KNIGHT;
      else if (sanMove[i+1]=='B' || sanMove[i+1]=='b')
        algMove.promote=BISHOP;
      else if (sanMove[i+1]=='R' || sanMove[i+1]=='r')
        algMove.promote=ROOK;
      else
        algMove.promote=QUEEN;                  // Default to queen.

      // Now remove the '=' part from the SAN string.
      for (int j=i;sanMove[j]!='\0';j++)
        sanMove[j]=sanMove[j+1];

      break;

    }

  }

  // 4. Find the target square.
  for (int i=strlen(sanMove)-1;i>0;i--) {

    // Look for the target square.
    if (sanMove[i]>='a' && sanMove[i]<='h'
        && sanMove[i+1]>='1' && sanMove[i+1]<='8') {

      // Set the target square.
      algMove.target=getSquare(sanMove[i],sanMove[i+1]);

      // Check to see if their is a file and/or rank specifier here also.
      fileSpec='0';                              // None found yet.
      rankSpec='0';                              // None found yet.
      for (int j=i-1;j>0;j--) {
        if (sanMove[j]>='a' && sanMove[j]<='h')
          fileSpec=sanMove[j];                  // Found file specifier.
        else if (sanMove[j]>='1' && sanMove[j]<='8')
          rankSpec=sanMove[j];                  // Found rank specifier.
      }

      break;                                     // Found, so exit now.

    }

    // If we get to the end, then there is no traget and it's an error.
    if (i==1) {
      cout << "1: ";
      return true;                              // No target, so error.
    }

  }

  // If the source piece is the king - easy, as only 1 king.
  if (sanMove[0]=='k') {
    algMove.source=g_currentState->kingSquare[g_currentSide];
  }

  // If we have both the file and rank specifier, then easy too.
  else if (fileSpec!='0' && rankSpec!='0') {
    algMove.source=getSquare(fileSpec,rankSpec);
  }

  // Will have to try things out - harder.
  else {

    // Find what kind of piece the source is.
    if (sanMove[0]=='Q')
      desiredPeice=QUEEN;
    else if (sanMove[0]=='R') 
      desiredPeice=ROOK;
    else if (sanMove[0]=='B') 
      desiredPeice=BISHOP;
    else
      desiredPeice=KNIGHT;                        // MUST be a knight then.

    // Loop through the generated moves to see if it matches and is legal.
    for (int i=0;i<moves.numMoves;i++) {
      if (((fileSpec=='0' && rankSpec=='0')
           || (fileSpec!='0'
               && getFile(moves.moves[i].source)==(fileSpec-'a'))
           || (rankSpec!='0' 
               && getRank(moves.moves[i].source)==7-(rankSpec-'1')))
          && moves.moves[i].target==algMove.target
          && g_currentPiece[moves.moves[i].source]==desiredPeice) {

        // Set the source square.
        algMove.source=moves.moves[i].source;

        // See if there is a capture in the SAN move.
        // NOTE: It may not be a capture, as 'x' is often omitted!
        if (strchr(sanMove,'x')!=nullptr)
          algMove.type|=CAPTURE;

        // Check to see if the move is legal.
        if (makeMove(algMove)) {
          takeMoveBack();
          return false;                         // Move O.K.
        }
        else {
          cout << "4: ";
          return true;                          // Error - not legal.
        }

      }
    }

    // If we get here then no move was found, so return an error.
    cout << "5: ";
    return true;

  }

  // 5. See if the move is a promotion, and find out the promotion peice.
  if (sanMove[0]>='a' && sanMove[0]<='h') {

    // Find the file of the pawn.
    int file=sanMove[0]-'a';

    // Find the rank of the pawn (from the current side).
    int rank;
    if (g_currentSide==WHITE) {
      for (int i=0;i<64;i++)
        if (g_currentPiece[i]==PAWN
            && g_currentColour[i]==WHITE
            && getFile(i)==file) {
          rank=getRank(i);
          break;
        }
    }
    else {
      for (int i=0;i<64;i++)
        if (g_currentPiece[i]==PAWN
            && g_currentColour[i]==BLACK
            && getFile(i)==file) {
          rank=getRank(i);
          break;
        }
    }

    // Set the source square.
    algMove.source=getSquare(file,rank);

    // See if there is a capture in the SAN move.
    if (strchr(sanMove,'x')!=nullptr)
      algMove.type|=CAPTURE;

    // Check to see if the move is legal.
    if (makeMove(algMove)) {
      takeMoveBack();
      return false;                             // Move O.K.
    }
    else {
      cout << "6: ";
      return true;                              // Error - not legal.
    }

  }

  // 6. Must be a normal piece move then.

  // Check to see if the move is legal.
  if (makeMove(algMove)) {
    takeMoveBack();
    return false;                               // Move O.K.
  }
  else {
    cout << "7: ";
    return true;                                // Error - not legal.
  }

} // End convertFromSAN.