/*
 * Vocab.h -- Interface to the Vocab class.
 */

#pragma once

#include "File.h"
#include "LHash.h"
#include "SArray.h"

namespace srilm {

#ifdef USE_SHORT_VOCAB
typedef unsigned short VocabIndex;
#else
typedef unsigned int VocabIndex;
#endif
typedef const char *VocabString;

const unsigned int maxWordLength = 1024;

const VocabIndex Vocab_None = (VocabIndex)-1;

const VocabString Vocab_Unknown = "<unk>";
const VocabString Vocab_SentStart = "<s>";
const VocabString Vocab_SentEnd = "</s>";
const VocabString Vocab_Pause = "-pau-";

class Vocab {
public:
  static unsigned int parseWords(char *line, VocabString *words,
                                 unsigned int max);

  static unsigned int length(const VocabIndex *words);
  static unsigned int length(const VocabString *words);

  static VocabIndex *copy(VocabIndex *to, const VocabIndex *from);
  static VocabString *copy(VocabString *to, const VocabString *from);

  static VocabIndex *convert(VocabIndex *to, VocabString *from);

  static bool contains(const VocabIndex *words, VocabIndex word);

  static VocabIndex *reverse(VocabIndex *words);
  static VocabString *reverse(VocabString *words);

  static void write(File &file, const VocabIndex *words);
  static void write(File &file, const VocabString *words);

  /*
   *  Comparison of Vocabs and their sequences
   */
  static int compare(VocabIndex word1, VocabIndex word2) {
    return word2 - word1;
  }
  static int compare(VocabString word1, VocabString word2) {
    return strcmp(word1, word2);
  }
  static int compare(const VocabIndex *word1, const VocabIndex *word2);
  /* order on word index sequences */
  static int compare(const VocabString *word1, const VocabString *word2);
};

/*
 * We sometimes use strings over VocabIndex as keys into maps.
 * Define the necessary support functions (see Map.h, LHash.cc, SArray.cc).
 */
static inline size_t LHash_hashKey(const VocabIndex *key, unsigned maxBits) {
  unsigned i = 0;

  if (key == 0) {
    return 0;
  }

  /*
   * The rationale here is similar to LHash_hashKey(unsigned),
   * except that we shift more to preserve more of the typical number of
   * bits in a VocabIndex.  The value was optimized to encoding 3 words
   * at a time (trigrams).
   */
  for (; *key != Vocab_None; key++) {
    i += (i << 12) + *key;
  }
  return LHash_hashKey(i, maxBits);
}

static inline const VocabIndex *Map_copyKey(const VocabIndex *key) {
  VocabIndex *copy = new VocabIndex[Vocab::length(key) + 1];
  assert(copy != 0);

  unsigned i;
  for (i = 0; key[i] != Vocab_None; i++) {
    copy[i] = key[i];
  }
  copy[i] = Vocab_None;

  return copy;
}

static inline void Map_freeKey(const VocabIndex *key) {
  delete[](VocabIndex *)key;
}

static inline bool LHash_equalKey(const VocabIndex *key1,
                                  const VocabIndex *key2) {
  if (key1 == 0) {
    return (key2 == 0);
  } else if (key2 == 0) {
    return false;
  }

  unsigned i;
  for (i = 0; key1[i] != Vocab_None && key2[i] != Vocab_None; i++) {
    if (key1[i] != key2[i]) {
      return false;
    }
  }
  if (key1[i] == Vocab_None && key2[i] == Vocab_None) {
    return true;
  } else {
    return false;
  }
}

static inline int SArray_compareKey(const VocabIndex *key1,
                                    const VocabIndex *key2) {
  unsigned int i = 0;

  if (key1 == 0) {
    if (key2 == 0) {
      return 0;
    } else {
      return -1;
    }
  } else if (key2 == 0) {
    return 1;
  }

  for (i = 0;; i++) {
    if (key1[i] == Vocab_None) {
      if (key2[i] == Vocab_None) {
        return 0;
      } else {
        return -1; /* key1 is shorter */
      }
    } else {
      if (key2[i] == Vocab_None) {
        return 1; /* key2 is shorted */
      } else {
        int comp = SArray_compareKey(key1[i], key2[i]);
        if (comp != 0) {
          return comp; /* they differ at pos i */
        }
      }
    }
  }
  /*NOTREACHED*/
}

}

