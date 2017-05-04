/*
 * Ngram.h -- N-gram backoff language models
 */

#pragma once

#include <stdio.h>
#include "Counts.h"
#include "Prob.h"
#include "Trie.h"
#include "VocabStatic.h"

#ifdef USE_SARRAY

#define PROB_INDEX_T SArray
#define PROB_ITER_T SArrayIter

#else /* ! USE_SARRAY */

#define PROB_INDEX_T LHash
#define PROB_ITER_T LHashIter

#endif /* USE_SARRAY */

namespace srilm {

typedef struct {
  LogP bow;                             /* backoff weight */
  PROB_INDEX_T<VocabIndex, LogP> probs; /* word probabilities */
} BOnode;

typedef Trie<VocabIndex, BOnode> BOtrie;

const unsigned defaultNgramOrder = 3;
const unsigned maxNgramOrder = 100;     /* Used in allocating various
                                         * data structures.  For all
                                         * practical purposes, this
                                         * should infinite. */

const unsigned unkIndex = 10000000;     /* <unk> 对应的索引值 */
const unsigned sosIndex = 10000001;     /* <s> 对应的索引值 */
const unsigned eosIndex = 10000002;     /* </s> 对应的索引值 */

#ifndef NUM_ENG
#define NUM_ENG
const unsigned numIndex = 10000003;     /* <num> 对应的索引值 */
const unsigned engIndex = 10000004;     /* <eng> 对应的索引值 */
const unsigned pnIndex = 10000005;      /* pn 对应的索引值 */
const unsigned lnIndex = 10000006;      /* ln 对应的索引值 */
#endif

class Ngram {
  friend class NgramBOsIter;

public:
  Ngram(unsigned order = defaultNgramOrder);
  virtual ~Ngram(){};

  unsigned setorder(unsigned neworder = 0); /* change/return ngram order */

  /*
   * LM interface
   */
  virtual LogP wordProb(VocabIndex word, const VocabIndex *context);
  virtual LogP wordProb(VocabIndex word, const VocabIndex *context,
                        unsigned int clen);

  virtual bool read(File &file);
  bool readBinary(File &file);

  virtual bool write(File &file);
  bool writeBinary(File &file);

  /*
   * Statistics
   */
  virtual Count numNgrams(unsigned int n) const;

  /*
   * Low-level access
   */
  LogP *findBOW(const VocabIndex *context) const;
  LogP *insertBOW(const VocabIndex *context);
  LogP *findProb(VocabIndex word, const VocabIndex *context) const;
  LogP *insertProb(VocabIndex word, const VocabIndex *context);
  void removeBOW(const VocabIndex *context);
  void removeProb(VocabIndex word, const VocabIndex *context);

  void clear(); /* remove all parameters */

protected:
  BOtrie contexts;    /* n-1 gram context trie */
  unsigned int order; /* maximal ngram order */

  LogP logp_unk_word;

  /*
   * Helper functions
   */
  bool setUnkProb();

  /*
   * Binary format support
   */
  bool writeBinaryNode(BOtrie &node, unsigned level, File &file,
                       long long &offset);
  bool readBinaryNode(BOtrie &node, unsigned order, unsigned maxOrder,
                      File &file, long long &offset);
};

/*
 * Iteration over all backoff nodes of a given order
 */
class NgramBOsIter {
public:
  NgramBOsIter(const Ngram &lm, VocabIndex *keys, unsigned order,
               int (*sort)(VocabIndex, VocabIndex) = 0)
      : myIter(lm.contexts, keys, order, sort){};

  void init() { myIter.init(); };
  BOnode *next() {
    Trie<VocabIndex, BOnode> *node = myIter.next();
    return node ? &(node->value()) : 0;
  }

private:
  TrieIter2<VocabIndex, BOnode> myIter;
};

/*
 * Iteration over all probs at a backoff node
 */
class NgramProbsIter {
public:
  NgramProbsIter(const BOnode &bonode, int (*sort)(VocabIndex, VocabIndex) = 0)
      : myIter(bonode.probs, sort){};

  void init() { myIter.init(); };
  LogP *next(VocabIndex &word) { return myIter.next(word); };

private:
  PROB_ITER_T<VocabIndex, LogP> myIter;
};

}

