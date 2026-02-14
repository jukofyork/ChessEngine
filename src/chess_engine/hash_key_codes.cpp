// ****************************************************************************
// *                          HASH KEY & CODE FUNCTIONS                       *
// ****************************************************************************

#include "chess_engine.h"
#include "globals.h"
#include <random>
#include <cstdint>

// ============================================================================

void initHashCodes(void)
{ // Init the hash code to use to 64bit random numbers.
  // Uses Mersenne Twister with fixed seed for reproducible hash codes.

  static std::mt19937_64 hashRng(100);  // Fixed seed for reproducibility
  std::uniform_int_distribution<uint64_t> dist;

  // Make a load of random data for hash codes.
  for (int i=0;i<2;i++) {
    for (int j=0;j<6;j++) {
      for (int k=0;k<64;k++) {
        g_hashCode[i][j][k]=dist(hashRng);
      }
    }
  }
  for (int i=0;i<64;i++) {
    g_enPassantHashCode[i]=dist(hashRng);
  }
  for (int i=0;i<16;i++) {
    g_castleHashCode[i]=dist(hashRng);
  }
  g_sideHashCode=dist(hashRng);

}

// ============================================================================

HashKey currentKey(void)
{ // Make a key from the board description.
  // Only use for loading ect, updated on the fly in MakeMove.

  HashKey key=0;

  // Use XOR for all squares.
  for (int i=0;i<BOARD_SQUARES;i++) {
    if (g_currentColour[i]!=NONE)
      key^=g_hashCode[g_currentColour[i]][g_currentPiece[i]][i];
  }

  // Return the key.
  return key;

} // End currentKey.

// ============================================================================

