// *****************************************************************************
// *                               LOOKUP TABLES                               *
// *****************************************************************************

#include "chess_engine.h"
#include "globals.h"
#include <mutex>

// ============================================================================
// Internal initialization function - called once via std::call_once
static void generateMoveTablesInternal(void) {
  int x,y;                   // Used to iterate through all squares in 2D.
  int i;                     // Used to iterate through move directions.
  int newX,newY;             // Used to iterate through move lines.

  // Go round all squares on board adding all sudo-legal moves for each one.
  for (y=0;y<8;y++) {
    for (x=0;x<8;x++) {

      // Generate the Knight's Moves for each sqaure.
      i=0;
      if (x-2>=0 && y-1>=0)
        g_knightMoves[(y*8)+x][i++]=((y-1)*8)+(x-2);
      if (x-2>=0 && y+1<8)
        g_knightMoves[(y*8)+x][i++]=((y+1)*8)+(x-2);
      if (x-1>=0 && y-2>=0)
        g_knightMoves[(y*8)+x][i++]=((y-2)*8)+(x-1);
      if (x+1<8 && y-2>=0)
        g_knightMoves[(y*8)+x][i++]=((y-2)*8)+(x+1);
      if (x+2<8 && y-1>=0)
        g_knightMoves[(y*8)+x][i++]=((y-1)*8)+(x+2);
      if (x+2<8 && y+1<8)
        g_knightMoves[(y*8)+x][i++]=((y+1)*8)+(x+2);
      if (x-1>=0 && y+2<8)
        g_knightMoves[(y*8)+x][i++]=((y+2)*8)+(x-1);
      if (x+1<8 && y+2<8)
        g_knightMoves[(y*8)+x][i++]=((y+2)*8)+(x+1);
      g_knightMoves[(y*8)+x][i]=END_OF_LOOKUP;           // End of list.

      // Generate the Diagonal (Bishop and Queen) moves for each square.
      for (i=0,newX=x-1,newY=y-1;newX>=0 && newY>=0;newX--,newY--)
        g_diagonalMoves[(y*8)+x][0][i++]=(newY*8)+newX;
      g_diagonalMoves[(y*8)+x][0][i]=END_OF_LOOKUP;      // End of list.
      for (i=0,newX=x-1,newY=y+1;newX>=0 && newY<8;newX--,newY++)
        g_diagonalMoves[(y*8)+x][1][i++]=(newY*8)+newX;
      g_diagonalMoves[(y*8)+x][1][i]=END_OF_LOOKUP;      // End of list.
      for (i=0,newX=x+1,newY=y-1;newX<8 && newY>=0;newX++,newY--)
        g_diagonalMoves[(y*8)+x][2][i++]=(newY*8)+newX;
      g_diagonalMoves[(y*8)+x][2][i]=END_OF_LOOKUP;      // End of list.
      for (i=0,newX=x+1,newY=y+1;newX<8 && newY<8;newX++,newY++)
        g_diagonalMoves[(y*8)+x][3][i++]=(newY*8)+newX;
      g_diagonalMoves[(y*8)+x][3][i]=END_OF_LOOKUP;      // End of list.

      // Generate the Straight (Rook and Queen) moves for each square.
      for (i=0,newX=x-1;newX>=0;newX--)
        g_straightMoves[(y*8)+x][0][i++]=(y*8)+newX;
      g_straightMoves[(y*8)+x][0][i]=END_OF_LOOKUP;      // End of list.
      for (i=0,newX=x+1;newX<8;newX++)
        g_straightMoves[(y*8)+x][1][i++]=(y*8)+newX;
      g_straightMoves[(y*8)+x][1][i]=END_OF_LOOKUP;      // End of list.
      for (i=0,newY=y-1;newY>=0;newY--)
        g_straightMoves[(y*8)+x][2][i++]=(newY*8)+x;
      g_straightMoves[(y*8)+x][2][i]=END_OF_LOOKUP;      // End of list.
      for (i=0,newY=y+1;newY<8;newY++)
        g_straightMoves[(y*8)+x][3][i++]=(newY*8)+x;
      g_straightMoves[(y*8)+x][3][i]=END_OF_LOOKUP;      // End of list.

      // Generate the King's moves for each square.
      i=0;
      if (x-1>=0 && y-1>=0)
        g_kingMoves[(y*8)+x][i++]=((y-1)*8)+(x-1);
      if (x-1>=0 && y+1<8)
        g_kingMoves[(y*8)+x][i++]=((y+1)*8)+(x-1);
      if (x+1<8 && y-1>=0)
        g_kingMoves[(y*8)+x][i++]=((y-1)*8)+(x+1);
      if (x+1<8 && y+1<8)
        g_kingMoves[(y*8)+x][i++]=((y+1)*8)+(x+1);
      if (x-1>=0)
        g_kingMoves[(y*8)+x][i++]=(y*8)+(x-1);
      if (x+1<8)
        g_kingMoves[(y*8)+x][i++]=(y*8)+(x+1);
      if (y-1>=0)
        g_kingMoves[(y*8)+x][i++]=((y-1)*8)+x;
      if (y+1<8)
        g_kingMoves[(y*8)+x][i++]=((y+1)*8)+x;
      g_kingMoves[(y*8)+x][i]=END_OF_LOOKUP;             // End of list.

    } // End for all Rows.
  } // End for all Columns.

} // End GenerateMoveTablesInternal.

// Wrapper function for thread-safe initialization
  void generateMoveTables(void) {
  static std::once_flag initFlag;
  std::call_once(initFlag, generateMoveTablesInternal);
} // End GenerateMoveTables.

// =============================================================================

void generateExposedAttackTable(void)
{ // This function generates a 64x64 vector of possible attacks from exposed
  // check (ie Queen/Rook/Bishop) attacks.
  // This may be used to save a full test for check in MakeMove(), so long as
  // the king was not the piece moved.
  // Also generates the knight attack table now.

  int* movePtr;                             // To iterate through Move Tables.
  int tempSquare;                           // Holds square read from lookup.

  // Assume the square can't be attacked.
  for (int i=0;i<64;i++) {
    for (int j=0;j<64;j++) {
      g_exposedAttackTable[i][j]=-1;
      g_knightAttackTable[i][j]=false;
    }
  }

  // For each square see if a Queen (ie: +rook/bishop) can attack the square.
  for (int i=0;i<64;i++) {

    // Add straight moves.
    for (int j=0;j<4;j++) {
      movePtr=g_straightMoves[i][j];
      while ((tempSquare=(*(movePtr++)))!=END_OF_LOOKUP)
        g_exposedAttackTable[i][tempSquare]=j;
    }

    // Add diagonal moves.
    for (int j=0;j<4;j++) {
      movePtr=g_diagonalMoves[i][j];
      while ((tempSquare=(*(movePtr++)))!=END_OF_LOOKUP)
        g_exposedAttackTable[i][tempSquare]=4+j;
    }

    // Add the knights moves.
    movePtr=g_knightMoves[i];
    while ((tempSquare=(*(movePtr++)))!=END_OF_LOOKUP)
      g_knightAttackTable[i][tempSquare]=true;

  }

} // End generateExposedAttackTable.

// =============================================================================

void initPosData(void)
{ // This function inits the g_posData lookup table.

  int i,j,s,count;
  int tableIndex;

  // Add the Knignt moves.
  for (s=0;s<64;s++) {

    tableIndex=0;

    // Add the moves.
    for (i=0;g_knightMoves[s][i]!=END_OF_LOOKUP;i++,tableIndex++) {
      g_posData[KNIGHT][s][tableIndex].testSquare=g_knightMoves[s][i];
      g_posData[KNIGHT][s][tableIndex].skip=(g_posData[KNIGHT][s]+tableIndex+1);
    }
    g_posData[KNIGHT][s][tableIndex].testSquare=END_OF_LOOKUP;
    g_posData[KNIGHT][s][tableIndex].skip=nullptr;

  }

  // Add the King moves.
  for (s=0;s<64;s++) {

    tableIndex=0;

    // Add the moves.
    for (i=0;g_kingMoves[s][i]!=END_OF_LOOKUP;i++,tableIndex++) {
      g_posData[KING][s][tableIndex].testSquare=g_kingMoves[s][i];
      g_posData[KING][s][tableIndex].skip=(g_posData[KING][s]+tableIndex+1);
    }
    g_posData[KING][s][tableIndex].testSquare=END_OF_LOOKUP;
    g_posData[KING][s][tableIndex].skip=nullptr;

  }

  // Add the bishop moves.
  for (s=0;s<64;s++) {

    tableIndex=0;

    // Add the moves.
    for (j=0;j<4;j++) {

      // count how mnay are in list.
      for (count=0,i=0;g_diagonalMoves[s][j][i]!=END_OF_LOOKUP;i++)
        count++;

      int nextEnd=tableIndex+count;

      for (i=0;g_diagonalMoves[s][j][i]!=END_OF_LOOKUP;i++,tableIndex++) {
        g_posData[BISHOP][s][tableIndex].testSquare=g_diagonalMoves[s][j][i];
        g_posData[BISHOP][s][tableIndex].skip=(g_posData[BISHOP][s]+nextEnd);
      }

    }
    g_posData[BISHOP][s][tableIndex].testSquare=END_OF_LOOKUP;
    g_posData[BISHOP][s][tableIndex].skip=nullptr;

  }

  // Add the rook moves.
  for (s=0;s<64;s++) {

    tableIndex=0;

    // Add the moves.
    for (j=0;j<4;j++) {

      // count how mnay are in list.
      for (count=0,i=0;g_straightMoves[s][j][i]!=END_OF_LOOKUP;i++)
        count++;

      int nextEnd=tableIndex+count;

      for (i=0;g_straightMoves[s][j][i]!=END_OF_LOOKUP;i++,tableIndex++) {
        g_posData[ROOK][s][tableIndex].testSquare=g_straightMoves[s][j][i];
        g_posData[ROOK][s][tableIndex].skip=(g_posData[ROOK][s]+nextEnd);
      }

    }
    g_posData[ROOK][s][tableIndex].testSquare=END_OF_LOOKUP;
    g_posData[ROOK][s][tableIndex].skip=nullptr;

  }

  // Add the queen moves.
  for (s=0;s<64;s++) {

    tableIndex=0;

    // Add the moves.
    for (j=0;j<4;j++) {

      // count how mnay are in list.
      for (count=0,i=0;g_diagonalMoves[s][j][i]!=END_OF_LOOKUP;i++)
        count++;

      int nextEnd=tableIndex+count;

      for (i=0;g_diagonalMoves[s][j][i]!=END_OF_LOOKUP;i++,tableIndex++) {
        g_posData[QUEEN][s][tableIndex].testSquare=g_diagonalMoves[s][j][i];
        g_posData[QUEEN][s][tableIndex].skip=(g_posData[QUEEN][s]+nextEnd);
      }

    }
    // Add the moves.
    for (j=0;j<4;j++) {

      // count how mnay are in list.
      for (count=0,i=0;g_straightMoves[s][j][i]!=END_OF_LOOKUP;i++)
        count++;

      int nextEnd=tableIndex+count;

      for (i=0;g_straightMoves[s][j][i]!=END_OF_LOOKUP;i++,tableIndex++) {
        g_posData[QUEEN][s][tableIndex].testSquare=g_straightMoves[s][j][i];
        g_posData[QUEEN][s][tableIndex].skip=(g_posData[QUEEN][s]+nextEnd);
      }

    }
    g_posData[QUEEN][s][tableIndex].testSquare=END_OF_LOOKUP;
    g_posData[QUEEN][s][tableIndex].skip=nullptr;

  }

} // End initPosData.

// ============================================================================

