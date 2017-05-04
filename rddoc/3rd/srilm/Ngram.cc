#include "Ngram.h"

#include <new>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/param.h>

namespace srilm {

const LogP LogP_PseudoZero = -99.0; /* non-inf value used for log 0 */

/* Constant for binary format */
const char *Ngram_BinaryFormatString = "TX_PECKER_BINARY_NGRAM\n";

Ngram::Ngram(unsigned neworder)
    : contexts(0),
      order(neworder) {
  if (order < 1) {
    order = 1;
  }
  logp_unk_word = LogP_PseudoZero;
}

unsigned Ngram::setorder(unsigned neworder) {
  unsigned oldorder = order;

  if (neworder > 0) {
    order = neworder;
  }

  return oldorder;
}

bool Ngram::setUnkProb() {
  LogP *prob = contexts.value().probs.find(unkIndex);

  if (!prob) {
    return false;
  }
  logp_unk_word = *prob;

  return true;
}
/*
 * Locate a BOW entry in the n-gram trie
 */
LogP *Ngram::findBOW(const VocabIndex *context) const {
  BOnode *bonode = contexts.find(context);
  if (bonode) {
    return &(bonode->bow);
  } else {
    return 0;
  }
}

/*
 * Locate a prob entry in the n-gram trie
 */
LogP *Ngram::findProb(VocabIndex word, const VocabIndex *context) const {
  BOnode *bonode = contexts.find(context);
  if (bonode) {
    return bonode->probs.find(word);
  } else {
    return 0;
  }
}

/*
 * Locate or create a BOW entry in the n-gram trie
 */
LogP *Ngram::insertBOW(const VocabIndex *context) {
  bool found;
  BOnode *bonode = contexts.insert(context, found);

  if (!found) {
    /*
     * initialize the index in the BOnode
     */
    new (&bonode->probs) PROB_INDEX_T<VocabIndex, LogP>(0);
  }
  return &(bonode->bow);
}

/*
 * Locate or create a prob entry in the n-gram trie
 */
LogP *Ngram::insertProb(VocabIndex word, const VocabIndex *context) {
  bool found;
  BOnode *bonode = contexts.insert(context, found);

  if (!found) {
    /*
     * initialize the index in the BOnode
     * We know we need at least one entry!
     */
    new (&bonode->probs) PROB_INDEX_T<VocabIndex, LogP>(1);
  }
  return bonode->probs.insert(word);
}

/*
 * Remove a BOW node (context) from the n-gram trie
 */
void Ngram::removeBOW(const VocabIndex *context) {
  contexts.removeTrie(context);
}

/*
 * Remove a prob entry from the n-gram trie
 */
void Ngram::removeProb(VocabIndex word, const VocabIndex *context) {
  BOnode *bonode = contexts.find(context);

  if (bonode) {
    bonode->probs.remove(word);
  }
}

/*
 * Remove all probabilities and contexts from n-gram trie
 */
void Ngram::clear() {
  VocabIndex context[order];

  BOnode *node;

  /*
   * Remove all ngram probabilities
   */
  for (unsigned i = order; i > 0; i--) {
    BOnode *node;
    NgramBOsIter iter(*this, context, i - 1);

    while ((node = iter.next())) {
      node->probs.clear(0);
    }
  }

  /*
   * Remove all unigram context tries
   * (since it won't work to delete at the root)
   * This also has the advantage of preserving the allocated space in
   * the root node, chosen to match the vocabulary.
   */
  if (order > 1) {
    NgramBOsIter iter(*this, context, 1);

    while ((node = iter.next())) {
      removeBOW(context);
    }
  }
}

/*
 * Higher level methods, implemented low-level for efficiency
 */

/*
 * This method implements the backoff algorithm in a way that descends into
 * the context trie only once, finding the maximal ngram and accumulating
 * backoff weights along the way.
 */
LogP Ngram::wordProb(VocabIndex word, const VocabIndex *context,
                     unsigned int clen) {
  LogP logp = logp_unk_word;  //LogP_Zero;
  LogP bow = LogP_One;

  BOtrie *trieNode = &contexts;
  unsigned i = 0;

  do {
    LogP *prob = trieNode->value().probs.find(word);

    if (prob) {
      /*
       * If a probability is found at this level record it as the
       * most specific one found so far and reset the backoff weight.
       */
      logp = *prob;
      bow = LogP_One;
    }

    if (i >= clen || context[i] == Vocab_None)
      break;

    BOtrie *next = trieNode->findTrie(context[i]);
    if (next) {
      /*
       * Accumulate backoff weights
       */
      bow += next->value().bow;
      trieNode = next;
      i++;
    } else {
      break;
    }
  } while (1);

  return logp + bow;
}

/*
 * Higher level methods (using the lower-level methods)
 */

LogP Ngram::wordProb(VocabIndex word, const VocabIndex *context) {
  unsigned int clen = Vocab::length(context);
  /*
   * Perform the backoff algorithm for a context lenth that is
   * the minimum of what we were given and the maximal length of
   * the contexts stored in the LM
   */
  return wordProb(word, context, ((clen > order - 1) ? order - 1 : clen));
}

bool Ngram::read(File &file) {
  char *line;
  unsigned maxOrder = 0; /* maximal n-gram order in this model */
  Count numNgrams[maxNgramOrder + 1];
  /* the number of n-grams for each order */
  Count numRead[maxNgramOrder + 1];
  /* Number of n-grams actually read */
  int state = -1;                 /* section of file being read:
                                   * -1 - pre-header, 0 - header,
                                   * 1 - unigrams, 2 - bigrams, ... */

  for (unsigned i = 0; i <= maxNgramOrder; i++) {
    numNgrams[i] = 0;
    numRead[i] = 0;
  }

  clear();

  /*
   * The ARPA format implicitly assumes a zero-gram backoff weight of 0.
   * This has to be properly represented in the BOW trie so various
   * recursive operations work correctly.
   */
  VocabIndex nullContext[1];
  nullContext[0] = Vocab_None;
  *insertBOW(nullContext) = LogP_Zero;

  while ((line = file.getline())) {
    bool backslash = (line[0] == '\\');

    switch (state) {
    case -1: /* looking for start of header */
      if (backslash && strncmp(line, "\\data\\", 6) == 0) {
        state = 0;
        continue;
      }
      /*
       * Everything before "\\data\\" is ignored
       */
      continue;

    case 0: /* ngram header */
      unsigned thisOrder;
      long long nNgrams;

      if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
        /*
         * start reading n-grams
         */
        if (state < 1 || (unsigned)state > maxOrder) {
          file.position() << "invalid ngram order " << state << "\n";
          return false;
        }
        continue;
      } else if (sscanf(line, "ngram %u=%lld", &thisOrder, &nNgrams) == 2) {
        /*
         * scanned a line of the form
         *  ngram <N>=<howmany>
         * now perform various sanity checks
         */
        if (thisOrder <= 0 || thisOrder > maxNgramOrder) {
          file.position() << "ngram order " << thisOrder << " out of range\n";
          return false;
        }
        if (nNgrams < 0) {
          file.position() << "ngram number " << nNgrams << " out of range\n";
          return false;
        }
        if (thisOrder > maxOrder) {
          maxOrder = thisOrder;
        }
        numNgrams[thisOrder] = nNgrams;
        continue;
      } else {
        file.position() << "unexpected input\n";
        return false;
      }

    default: /* reading n-grams, where n == state */
      if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
        if (state < 1 || (unsigned)state > maxOrder) {
          file.position() << "invalid ngram order " << state << "\n";
          return false;
        }
        /*
         * start reading more n-grams
         */
        continue;
      } else if (backslash && strncmp(line, "\\end\\", 5) == 0) {
        /*
         * Check that the total number of ngrams read matches
         * that found in the header
         */
        for (unsigned i = 0; i <= maxOrder && i <= order; i++) {
          if (numNgrams[i] != numRead[i]) {
            file.position() << "warning: " << numRead[i] << " " << i
                            << "-grams read, expected " << numNgrams[i] << "\n";
          }
        }
        if (!setUnkProb()) {
          return false;
        }
        return true;
      } else if ((unsigned)state > order) {
        /*
         * Save time and memory by skipping ngrams outside
         * the order range of this model
         */
        continue;
      } else {
        VocabString words[1 + maxNgramOrder + 1 + 1];
        /* result of parsing an n-gram line
         * the first and last elements are actually
         * numerical parameters, but so what? */
        VocabIndex wids[maxNgramOrder + 1];
        /* ngram translated to word indices */
        LogP prob, bow = LogP_One;
        /* probability and back-off-weight */

        /*
         * Parse a line of the form
         *  <prob>  <w1> <w2> ... [ <bow> ]
         */
        unsigned howmany = Vocab::parseWords(line, words, state + 3);

        if (howmany < (unsigned)state + 1 || howmany > (unsigned)state + 2) {
          file.position() << "ngram line has " << howmany << " fields ("
                          << state + 2 << " expected)\n";
          return false;
        }

        /*
         * Parse prob
         */
        if (!parseLogP(words[0], prob)) {
          file.position() << "bad prob \"" << words[0] << "\"\n";
          return false;
        }

        if (prob > LogP_One || prob != prob) {
          file.position() << "warning: questionable prob \"" << words[0]
                          << "\"\n";
        } else if (prob == LogP_PseudoZero) {
          /*
           * convert pseudo-zeros back into real zeros
           */
          prob = LogP_Zero;
        }

        /*
         * Parse bow, if any
         */
        if (howmany == (unsigned)state + 2) {
          /*
           * Parsing floats strings is the most time-consuming
           * part of reading in backoff models.  We therefore
           * try to avoid parsing bows where they are useless,
           * i.e., for contexts that are longer than what this
           * model uses.  We also do a quick sanity check to
           * warn about non-zero bows in that position.
           */
          if ((unsigned)state == maxOrder) {
            if (words[state + 1][0] != '0') {
              file.position() << "ignoring non-zero bow \"" << words[state + 1]
                              << "\" for maximal ngram\n";
            }
          } else if ((unsigned)state == order) {
            /*
             * save time and memory by skipping bows that will
             * never be used as a result of higher-order ngram
             * skipping
             */
            ;
          } else {
            /*
             * Parse BOW
             */
            if (!parseLogP(words[state + 1], bow)) {
              file.position() << "bad bow \"" << words[state + 1] << "\"\n";
              return false;
            }

            if (bow == LogP_Inf || bow != bow) {
              file.position() << "warning: questionable bow \""
                              << words[state + 1] << "\"\n";
            } else if (bow == LogP_PseudoZero) {
              /*
               * convert pseudo-zeros back into real zeros
               */
              bow = LogP_Zero;
            }
          }
        }

        numRead[state]++;

        /*
         * Terminate the words array after the last word,
         * then translate it to word indices.  We also
         * reverse the ngram since that's how we'll need it
         * to index the trie.
         */
        words[state + 1] = 0;

        Vocab::convert(wids, &words[1]);
        Vocab::reverse(wids);

        /*
         * Store bow, if any
         */
        if (howmany == (unsigned)state + 2 && (unsigned)state < order) {
          *insertBOW(wids) = bow;
        }

        /*
         * Save the last word (which is now the first, due to reversal)
         * then use the first n-1 to index into
         * the context trie, storing the prob.
         */
        BOnode *bonode = contexts.find(&wids[1]);
        if (!bonode) {
          file.position() << "warning: no bow for prefix of ngram \""
                          << &words[1] << "\"\n";
        } else {
          /* efficient for: *insertProb(wids[0], &wids[1]) = prob */
          *bonode->probs.insert(wids[0]) = prob;
        }

        /*
         * Hey, we're done with this ngram!
         */
      }
    }
  }

  /*
   * we reached a premature EOF
   */
  file.position() << "reached EOF before \\end\\\n";
  return false;
}

bool Ngram::write(File &file) {
  unsigned i;
  Count howmanyNgrams[maxNgramOrder + 1];
  VocabIndex context[maxNgramOrder + 2];

  if (order > maxNgramOrder) {
    order = maxNgramOrder;
  }

  file.fprintf("\n\\data\\\n");

  for (i = 1; i <= order; i++) {
    howmanyNgrams[i] = numNgrams(i);
    file.fprintf("ngram %d=%lld\n", i, (long long)howmanyNgrams[i]);
  }

  for (i = 1; i <= order; i++) {
    file.fprintf("\n\\%d-grams:\n", i);

    NgramBOsIter iter(*this, context + 1, i - 1, SArray_compareKey);
    BOnode *node;

    while ((node = iter.next())) {
      Vocab::reverse(context + 1);

      NgramProbsIter piter(*node, SArray_compareKey);
      VocabIndex pword;
      LogP *prob;

      while ((prob = piter.next(pword))) {
        if (file.error()) {
          return false;
        }

        file.fprintf("%.*lg\t", LogP_Precision,
                     (double)(*prob == LogP_Zero ? LogP_PseudoZero : *prob));
        Vocab::write(file, context + 1);
        file.fprintf("%s%d", (i > 1 ? " " : ""), pword);

        if (i < order) {
          context[0] = pword;

          LogP *bow = findBOW(context);
          if (bow) {
            file.fprintf("\t%.*lg", LogP_Precision,
                         (double)(*bow == LogP_Zero ? LogP_PseudoZero : *bow));
          }
        }

        file.fprintf("\n");
      }

      Vocab::reverse(context + 1);
    }
  }

  file.fprintf("\n\\end\\\n");

  return true;
}

/*
 * New binary format:
        magic string \n
        max order \n
        vocabulary index map
        probs-and-bow-trie (binary)
 */
bool Ngram::writeBinary(File &file) {
  /*
   * Magic string
   */
  file.fprintf("%s", Ngram_BinaryFormatString);

  /*
   * Maximal count order
   */
  file.fprintf("maxorder %u\n", order);

  long long offset = file.ftell();

  // detect if file is not seekable
  if (offset < 0) {
    file.position() << strerror(errno) << std::endl;
    return false;
  }

  /*
   * Context trie data
   */
  return writeBinaryNode(contexts, 1, file, offset);
}

/*
 * Binary format for context trie:
        length-of-subtrie
        back-off-weight
        number-of-probs
        word1
        prob1
        word2
        prob2
        ...
        word1
        subtrie1
        word2
        subtrie2
        ...
 */
bool Ngram::writeBinaryNode(BOtrie &node, unsigned level, File &file,
                               long long &offset) {
  BOnode &bo = node.value();

  // guess number of bytes needed for storing subtrie rooted at node
  // based on its depth (if we guess wrong we need to redo the whole
  // subtrie later)
  unsigned subtrieDepth = order - level;
  unsigned offsetBytes = subtrieDepth == 0 ? 2 : subtrieDepth <= 3 ? 4 : 8;

  long long startOffset = offset;  // remember start offset

retry:
  // write placeholder value
  unsigned nbytes = writeBinaryCount(file, (unsigned long long)0, offsetBytes);
  if (!nbytes)
    return false;
  offset += nbytes;

  // write backoff weight
  nbytes = writeBinaryCount(file, bo.bow);
  if (!nbytes)
    return false;
  offset += nbytes;

  // write probs
  unsigned numProbs = bo.probs.numEntries();

  nbytes = writeBinaryCount(file, numProbs);
  if (!nbytes)
    return false;
  offset += nbytes;

  VocabIndex word;
  LogP *pprob;

// write probabilities -- always in index-sorted order to ensure fast
// reading regardless of data structure used
#ifdef USE_SARRAY_TRIE
  PROB_ITER_T<VocabIndex, LogP> piter(bo.probs);
#else
  PROB_ITER_T<VocabIndex, LogP> piter(bo.probs, SArray_compareKey);
#endif

  while ((pprob = piter.next(word))) {
    nbytes = writeBinaryCount(file, word);
    if (!nbytes)
      return false;
    offset += nbytes;

    nbytes = writeBinaryCount(file, *pprob);
    if (!nbytes)
      return false;
    offset += nbytes;
  }

// write subtries
#ifdef USE_SARRAY_TRIE
  TrieIter<VocabIndex, BOnode> iter(node);
#else
  TrieIter<VocabIndex, BOnode> iter(node, SArray_compareKey);
#endif

  BOtrie *sub;
  while ((sub = iter.next(word))) {
    nbytes = writeBinaryCount(file, word);
    if (!nbytes)
      return false;
    offset += nbytes;

    if (!writeBinaryNode(*sub, level + 1, file, offset)) {
      return false;
    }
  }

  long long endOffset = offset;

  if (file.fseek(startOffset, SEEK_SET) < 0) {
    file.offset() << strerror(errno) << std::endl;
    return false;
  }

  // don't update offset since we're skipping back in file
  nbytes = writeBinaryCount(file, (unsigned long long)(endOffset - startOffset),
                            offsetBytes);
  if (!nbytes)
    return false;

  // now check that the number of bytes used for offset was actually ok
  if (nbytes > offsetBytes) {
    file.offset() << "increasing offset bytes from " << offsetBytes << " to "
                  << nbytes << " (order " << order << ","
                  << " level " << level << ")\n";

    offsetBytes = nbytes;

    if (file.fseek(startOffset, SEEK_SET) < 0) {
      file.offset() << strerror(errno) << std::endl;
      return false;
    }
    offset = startOffset;

    goto retry;
  }

  if (file.fseek(endOffset, SEEK_SET) < 0) {
    file.offset() << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

/*
 * Machine-independent binary format
 */
bool Ngram::readBinary(File &file) {
  char *line = file.getline();

  if (!line || strcmp(line, Ngram_BinaryFormatString) != 0) {
    file.position() << "bad binary format\n";
    return false;
  }

  /*
   * Maximal count order
   */
  line = file.getline();
  unsigned maxOrder;
  if (!line || (sscanf(line, "maxorder %u", &maxOrder) != 1)) {
    file.position() << "could not read ngram order\n";
    return false;
  }

  long long offset = file.ftell();

  // detect if file is not seekable
  if (offset < 0) {
    file.position() << strerror(errno) << std::endl;
    return false;
  }

  clear();

  /*
   * LM data
   */
  return readBinaryNode(contexts, this->order, maxOrder, file, offset)
    && setUnkProb();
}

bool Ngram::readBinaryNode(BOtrie &node, unsigned order, unsigned maxOrder,
                           File &file, long long &offset) {
  if (maxOrder == 0) {
    return true;
  } else {
    long long endOffset;
    unsigned long long trieLength;
    unsigned nbytes;

    nbytes = readBinaryCount(file, trieLength);
    if (!nbytes) {
      return false;
    }
    endOffset = offset + trieLength;
    offset += nbytes;

    if (order == 0) {
      if (file.fseek(endOffset, SEEK_SET) < 0) {
        file.offset() << strerror(errno) << std::endl;
        return false;
      }
      offset = endOffset;
    } else {
      BOnode &bo = node.value();

      // read backoff weight
      nbytes = readBinaryCount(file, bo.bow);
      if (!nbytes)
        return false;
      offset += nbytes;

      // read probs
      unsigned numProbs;
      nbytes = readBinaryCount(file, numProbs);
      if (!nbytes)
        return false;
      offset += nbytes;

      for (unsigned i = 0; i < numProbs; i++) {
        VocabIndex wid;

        nbytes = readBinaryCount(file, wid);
        if (!nbytes)
          return false;
        offset += nbytes;

        LogP prob;

        nbytes = readBinaryCount(file, prob);
        if (!nbytes)
          return false;
        offset += nbytes;

        if (wid != Vocab_None) {
          *bo.probs.insert(wid) = prob;
        }
      }

      // read subtries
      while (offset < endOffset) {
        VocabIndex wid;

        nbytes = readBinaryCount(file, wid);
        if (!nbytes)
          return false;
        offset += nbytes;

        if (wid == Vocab_None) {
          // skip subtrie
          if (!readBinaryNode(node, 0, maxOrder - 1, file, offset)) {
            return false;
          }
        } else {
          // create subtrie and read it
          BOtrie *child = node.insertTrie(wid);

          if (!readBinaryNode(*child, order - 1, maxOrder - 1, file, offset)) {
            return false;
          }
        }
      }

      if (offset != endOffset) {
        file.offset() << "data misaligned\n";
        return false;
      }
    }

    return true;
  }
}

Count Ngram::numNgrams(unsigned int order) const {
  if (order < 1) {
    return 0;
  } else {
    Count howmany = 0;

    VocabIndex context[order + 1];

    NgramBOsIter iter(*this, context, order - 1);
    BOnode *node;

    while ((node = iter.next())) {
      howmany += node->probs.numEntries();
    }

    return howmany;
  }
}

}

