// **************************************************************************
// *                        TRANSPOSITION TABLE FUNCTIONS                   *
// **************************************************************************

#include "search_engine.h"

// ==========================================================================

int foldHashKey(HashKey key,int numElementsPow2)
{ // Get the required (index) key from the large 64-bit key.
  // This uses the folding method, as the low/high bits of the random number
  // generator ar poor on their own.
  // NOTE: Do NOT simply and out the requires bit or use modulous as this will
  //       give a very poor distribution of indexes using the current scheme.
  // NOTE: Now used for eval cache key to index.

  int indexKey=0;
  int i;

  // Make sure all bits are used for the index to get a godd distribution.
  for (i=64-numElementsPow2;i>0;i-=numElementsPow2)
    indexKey^=(key>>i);
  if (i!=0)
    indexKey^=(key&((1<<(i+numElementsPow2+1))-1));

  // Take just the bits we need.
  return indexKey&((1<<numElementsPow2)-1);

} // End foldHashKey.

// ============================================================================

void ttPut(SearchData &searchData,int currentPly,int depth,int alpha,int beta,int score,
           MoveStruct move,HashKey nextKey)
{  // Put the current state (ie 64bit identifier key!) in the hash table.

  HashKey key;

  HashRecord *hash;

  // Get the key and a pointer to hash ellement.
  key=g_currentState->key;
  // Add the castle permisions and the en-passent info.
  key^=g_castleHashCode[g_currentState->castlePerm];
  if (g_currentState->enPass!=NO_EN_PASSANT)
    key^=g_enPassantHashCode[g_currentState->enPass];
  if (g_currentSide==BLACK)
    key^=g_sideHashCode;
  hash=&searchData.hashTable[foldHashKey(key)];

  // Is it better than this state (ie: lower depth?).
  if (hash->flags!=0 && hash->depth>depth && !isMateScore(score))
    return;

  // Is it a collision?
  if (hash->flags!=0)
    searchData.numHashCollisions++;

  // One more put in hash.
  searchData.totalPutHashCount++;

  // Save the currect stuuf to the hash record.
  hash->depth=depth;
  hash->key=key;
  hash->nextKey=nextKey;
  hash->move=move;
  if (isMateScore(score))
    hash->score=score+(score>0?currentPly:-currentPly);
  else
    hash->score=score;
  if (score>=beta)
    hash->flags=LOWERBOUND;
  else if (score<=alpha)
    hash->flags=UPPERBOUND;
  else
    hash->flags=EXACTSCORE;

} // End ttPut.

// =============================================================================

uint8_t ttGet(SearchData &searchData,int currentPly,int depth,int &score,MoveStruct &move,
              HashKey &nextKey)
{ // Get a record from the hash if possible.

  HashRecord *hash;

  HashKey key;

  // Get the key and a pointer to hash ellement.
  key=g_currentState->key;
  // Add the castle permisions and the en-passent info.
  key^=g_castleHashCode[g_currentState->castlePerm];
  if (g_currentState->enPass!=NO_EN_PASSANT)
    key^=g_enPassantHashCode[g_currentState->enPass];
  if (g_currentSide==BLACK)
    key^=g_sideHashCode;
  hash=&searchData.hashTable[foldHashKey(key)];

  // If not there, poor-draft or key not same - return.
  if (hash->key!=key) {
    move = MoveStruct{NONE, NONE, NORMAL_MOVE, NO_PROMOTION};  // So move is invalid.
   nextKey=0;                           // So key is 0.
   return 0;
  }

  // Get the move and the score to return.
  move=hash->move;
  nextKey=hash->nextKey;
  score=hash->score;

  // If depth is too low, we can still use the move!
  if (hash->depth<depth && !isMateScore(hash->score))
    return 0;

  // Alter to be the correct mate in N for the ply.
  if (isMateScore(score))
    score-=(score>0?currentPly:-currentPly);

  // Return the flags.
  return hash->flags;

} // End ttGet.

// ==========================================================================

