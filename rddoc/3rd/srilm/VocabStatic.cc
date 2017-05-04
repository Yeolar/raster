#include "VocabStatic.h"
#include <string.h>

namespace srilm {

// parse strings into words and update stats
unsigned int Vocab::parseWords(char *sentence, VocabString *words,
                               unsigned int max) {
  char *word;
  unsigned i;
  char *strtok_ptr = NULL;

  for (i = 0,
      word = strtok_r(sentence, wordSeparators, &strtok_ptr);
       i < max && word != 0;
       i++, word = strtok_r(0, wordSeparators, &strtok_ptr)) {
    words[i] = word;
  }
  if (i < max) {
    words[i] = 0;
  }

  return i;
}

/*
 * Length of Ngrams
 */
unsigned int Vocab::length(const VocabIndex *words) {
  unsigned int len = 0;

  while (words[len] != Vocab_None)
    len++;
  return len;
}

unsigned int Vocab::length(const VocabString *words) {
  unsigned int len = 0;

  while (words[len] != 0)
    len++;
  return len;
}

/*
 * Copying (a la strcpy())
 */
VocabIndex *Vocab::copy(VocabIndex *to, const VocabIndex *from) {
  unsigned i;
  for (i = 0; from[i] != Vocab_None; i++) {
    to[i] = from[i];
  }
  to[i] = Vocab_None;

  return to;
}

VocabString *Vocab::copy(VocabString *to, const VocabString *from) {
  unsigned i;
  for (i = 0; from[i] != 0; i++) {
    to[i] = from[i];
  }
  to[i] = 0;

  return to;
}

/*
 * Convert
 */
VocabIndex *Vocab::convert(VocabIndex *to, VocabString *from) {
  unsigned i;
  for (i = 0; from[i] != 0; i++) {
    to[i] = (VocabIndex)atoi(from[i]);
  }
  to[i] = Vocab_None;

  return to;
}

/*
 * Word containment
 */
bool Vocab::contains(const VocabIndex *words, VocabIndex word) {
  unsigned i;

  for (i = 0; words[i] != Vocab_None; i++) {
    if (words[i] == word) {
      return true;
    }
  }
  return false;
}

/*
 * Reversal of Ngrams
 */
VocabIndex *Vocab::reverse(VocabIndex *words) {
  int i, j; /* j can get negative ! */

  for (i = 0, j = length(words) - 1; i < j; i++, j--) {
    VocabIndex x = words[i];
    words[i] = words[j];
    words[j] = x;
  }
  return words;
}

VocabString *Vocab::reverse(VocabString *words) {
  int i, j; /* j can get negative ! */

  for (i = 0, j = length(words) - 1; i < j; i++, j--) {
    VocabString x = words[i];
    words[i] = words[j];
    words[j] = x;
  }
  return words;
}

/*
 * Output of Ngrams
 */

void Vocab::write(File &file, const VocabIndex *words) {
  for (unsigned int i = 0; words[i] != Vocab_None; i++) {
    file.fprintf("%s%d", (i > 0 ? " " : ""), words[i]);
  }
}

void Vocab::write(File &file, const VocabString *words) {
  for (unsigned int i = 0; words[i] != 0; i++) {
    file.fprintf("%s%s", (i > 0 ? " " : ""), words[i]);
  }
}

/*
 * Sorting of word sequences
 */

int Vocab::compare(const VocabString *words1, const VocabString *words2) {
  unsigned int i = 0;

  for (i = 0;; i++) {
    if (words1[i] == 0) {
      if (words2[i] == 0) {
        return 0;
      } else {
        return -1; /* words1 is shorter */
      }
    } else {
      if (words2[i] == 0) {
        return 1; /* words2 is shorted */
      } else {
        int comp = compare(words1[i], words2[i]);
        if (comp != 0) {
          return comp; /* they differ as pos i */
        }
      }
    }
  }
  /*NOTREACHED*/
}

int Vocab::compare(const VocabIndex *words1, const VocabIndex *words2) {
  unsigned int i = 0;

  for (i = 0;; i++) {
    if (words1[i] == Vocab_None) {
      if (words2[i] == Vocab_None) {
        return 0;
      } else {
        return -1; /* words1 is shorter */
      }
    } else {
      if (words2[i] == Vocab_None) {
        return 1; /* words2 is shorted */
      } else {
        int comp = compare(words1[i], words2[i]);
        if (comp != 0) {
          return comp; /* they differ as pos i */
        }
      }
    }
  }
  /*NOTREACHED*/
}

}

