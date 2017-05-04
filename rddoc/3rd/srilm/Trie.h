/*
 * Trie.h -- Trie structures.
 *
 * Trie<KeyT,DataT> is a template container class that implements a mapping
 * from sequences of _keys_ (type KeyT *) to data items or _values_
 * (type DataT).  It is built as an extension of Map and has a similar
 * interface.
 * Most functions accept either a single KeyT (a sequence of length 1),
 * or variable-length vector of KeyT elements.  In the latter case,
 * the KeyT * argument is terminated by a key element for which
 * Map_noKeyP() holds true.  For convenience, the KeyT * sequence can be
 * the null pointer to refer to the current Trie node.  This is also the default
 * argument value.  For example, find() will return a pointer to the data
 * stored at the current node.
 *
 * DataT *find(const KeyT *keys, bool &foundP)
 * DataT *find(KeyT key, bool &foundP)
 * DataT *find(const KeyT *keys)
 * DataT *find(KeyT key)
 *  Returns a pointer to the data item found under key, or null if
 *  the key is not in the trie.
 *  With this and the other functions, the foundP argument is optional
 *  and returns whether the key was found.
 *
 * DataT *findPrefix(const KeyT *keys, unsigned &depth)
 * DataT *findPrefix(const KeyT *keys)
 *  Returns a pointer to the data item corresponding to a maximal
 *  prefix of keys.  This maybe the item at the root if keys[0] doesn't
 *  match any root entry.
 *
 * DataT *insert(const KeyT *keys, bool &foundP)
 * DataT *insert(KeyT key, bool &foundP)
 * DataT *insert(const KeyT *keys)
 * DataT *insert(KeyT key)
 *  Returns a pointer to the data item for key, creating a new
 *  entry if necessary (indicated by foundP == false).
 *  New data items are zero-initialized.
 *
 * bool remove(const KeyT *keys, DataT *removedData = 0)
 * bool remove(KeyT key, DataT *removedData = 0)
 *  Deletes the entry associated with key from the trie.
 *      If removedData is not null it will be populated with
 *      the previously stored value, if any.
 *
 * Trie *findTrie(const KeyT *keys, bool &foundP)
 * Trie *findTrie(KeyT key, bool &foundP)
 * Trie *findTrie(const KeyT *keys)
 * Trie *findTrie(KeyT key)
 *  Returns a pointer to the trie node found under key, or null if
 *  the key is no in the trie.
 *
 * Trie *findPrefixTrie(const KeyT *keys, unsigned &depth)
 * Trie *findPrefixTrie(const KeyT *keys)
 *  Returns a pointer to the trie node indexed by the maximal prefix of
 *  keys.
 *
 * Trie *insertTrie(const KeyT *keys, bool &foundP)
 * Trie *insertTrie(KeyT key, bool &foundP)
 * Trie *insertTrie(const KeyT *keys)
 * Trie *insertTrie(KeyT key, bool)
 *  Returns a pointer to the trie node found for key, creating a new
 *  node if necessary (indicated by foundP == false).
 *  The data for the new node is zero-initialized, and has no
 *  child nodes.
 *
 * voild clear()
 *  Removes all entries.
 *
 * unsigned int numEntries(const KeyT *keys)
 *  Returns the current number of keys (i.e., entries) in a subtrie,
 *  or at the current level (if keys = 0).
 *
 * DataT &value()
 *  Return the current data item (by reference, not pointer!).
 *
 * The DataT * pointers returned by find() and insert() are
 * valid only until the next operation on the Trie object.  It is left
 * to the user to assign actual values by dereferencing the pointers
 * returned.  The main benefit is that only one key lookup is needed
 * for a find-and-change operation.
 *
 * TrieIter<KeyT,DataT> provids iterators over the child nodes of one
 * Trie node.
 *
 * TrieIter(Trie<KeyT,DataT> &trie)
 *  Creates and initializes an iteration over trie.
 *
 * void init()
 *  Reset an interation to the first element.
 *
 * Trie<KeyT,DataT> *next(KeyT &key)
 *  Steps the iteration and returns a pointer to the next subtrie,
 *  or null if the iteration is finished.  key is set to the associated
 *  Key value.
 *
 * Note that the iterator returns pointers to the Trie nodes, not the
 * stored data items.  Those can be accessed with value(), see above.
 */

#pragma once

#include <new>

#undef TRIE_INDEX_T
#undef TRIE_ITER_T

#ifdef USE_SARRAY_TRIE

#define TRIE_INDEX_T SArray
#define TRIE_ITER_T SArrayIter
#include "SArray.h"

#else /* ! USE_SARRAY_TRIE */

#define TRIE_INDEX_T LHash
#define TRIE_ITER_T LHashIter
#include "LHash.h"

#endif /* USE_SARRAY_TRIE */

namespace srilm {

template <class KeyT, class DataT>
class TrieIter;  // forward declaration
template <class KeyT, class DataT>
class TrieIter2;  // forward declaration

template <class KeyT, class DataT>
class Trie {
  friend class TrieIter<KeyT, DataT>;
  friend class TrieIter2<KeyT, DataT>;

public:
  Trie(unsigned size = 0);
  ~Trie();

  DataT &value() { return data; };

  DataT *find(const KeyT *keys, bool &foundP) const {
    Trie<KeyT, DataT> *node = findTrie(keys, foundP);
    return node ? &(node->data) : 0;
  };
  DataT *find(KeyT key, bool &foundP) const {
    Trie<KeyT, DataT> *node = findTrie(key, foundP);
    return node ? &(node->data) : 0;
  };
  DataT *find(const KeyT *keys) const {
    bool found;
    return find(keys, found);
  };
  DataT *find(KeyT key) const {
    bool found;
    return find(key, found);
  };

  DataT *findPrefix(const KeyT *keys, unsigned &depth) const {
    return &(findPrefixTrie(keys, depth)->data);
  };
  DataT *findPrefix(const KeyT *keys) const {
    unsigned depth;
    return findPrefix(keys, depth);
  };

  DataT *insert(const KeyT *keys, bool &foundP) {
    return &(insertTrie(keys, foundP)->data);
  };
  DataT *insert(KeyT key, bool &foundP) {
    return &(insertTrie(key, foundP)->data);
  };
  DataT *insert(const KeyT *keys) {
    bool found;
    return insert(keys, found);
  };
  DataT *insert(KeyT key) {
    bool found;
    return insert(key, found);
  };

  bool remove(const KeyT *keys = 0, DataT *removedData = 0) {
    Trie<KeyT, DataT> node;
    bool ret = removeTrie(keys, &node);
    if (removedData != 0)
      memcpy(removedData, &(node.data), sizeof(DataT));
    return ret;
  };
  bool remove(KeyT key, DataT *removedData = 0) {
    Trie<KeyT, DataT> node;
    bool ret = removeTrie(key, &node);
    if (removedData != 0)
      memcpy(removedData, &(node.data), sizeof(DataT));
    return ret;
  };
  Trie<KeyT, DataT> *findTrie(const KeyT *keys, bool &foundP) const;
  Trie<KeyT, DataT> *findTrie(KeyT key, bool &foundP) const {
    return sub.find(key, foundP);
  };
  Trie<KeyT, DataT> *findTrie(const KeyT *keys) const {
    bool found;
    return findTrie(keys, found);
  };
  Trie<KeyT, DataT> *findTrie(const KeyT key) const {
    bool found;
    return findTrie(key, found);
  };

  Trie<KeyT, DataT> *findPrefixTrie(const KeyT *keys, unsigned &depth) const;
  Trie<KeyT, DataT> *findPrefixTrie(const KeyT *keys) const {
    unsigned depth;
    return findPrefixTrie(keys, depth);
  };

  Trie<KeyT, DataT> *insertTrie(const KeyT *keys, bool &foundP);
  Trie<KeyT, DataT> *insertTrie(KeyT key, bool &foundP) {
    Trie<KeyT, DataT> *subtrie = sub.insert(key, foundP);
    if (!foundP)
      new (&subtrie->sub) KeyT(0);
    return subtrie;
  }
  Trie<KeyT, DataT> *insertTrie(const KeyT *keys) {
    bool found;
    return insertTrie(keys, found);
  };
  Trie<KeyT, DataT> *insertTrie(KeyT key) {
    bool found;
    return insertTrie(key, found);
  };

  bool removeTrie(const KeyT *keys = 0, Trie<KeyT, DataT> *removedData = 0);
  bool removeTrie(KeyT key, Trie<KeyT, DataT> *removedData = 0) {
    KeyT keys[2];
    keys[0] = key, Map_noKey(keys[1]);
    return removeTrie(keys, removedData);
  };

  void clear() { sub.clear(0); };

  unsigned int numEntries(const KeyT *keys = 0) const;

private:
  TRIE_INDEX_T<KeyT, Trie<KeyT, DataT> > sub
#ifdef USE_PACKED_TRIE
      __attribute__((packed))
#endif
      ;       /* LHash of child nodes */
  DataT data; /* data stored at this node */
};

/*
 * Iteration over immediate child nodes
 */
template <class KeyT, class DataT>
class TrieIter {
public:
  TrieIter(const Trie<KeyT, DataT> &trie, int (*sort)(KeyT, KeyT) = 0)
      : myIter(trie.sub, sort){};

  void init() { myIter.init(); };
  Trie<KeyT, DataT> *next(KeyT &key) { return myIter.next(key); };

private:
  TRIE_ITER_T<KeyT, Trie<KeyT, DataT> > myIter;
};

/*
 * Iteration over all nodes at a given depth in the trie
 */
template <class KeyT, class DataT>
class TrieIter2 {
public:
  TrieIter2(const Trie<KeyT, DataT> &trie, KeyT *keys, unsigned int level,
            int (*sort)(KeyT, KeyT) = 0);
  ~TrieIter2();

  void init();               /* re-initialize */
  Trie<KeyT, DataT> *next(); /* next element -- sets keys */

private:
  const Trie<KeyT, DataT> &myTrie; /* Node being iterated over */
  KeyT *keys;                      /* array passed to hold keys */
  int level;                       /* depth of nodes enumerated */
  int (*sort)(KeyT, KeyT);         /* key comparison function for sort */
  TRIE_ITER_T<KeyT, Trie<KeyT, DataT> > myIter;
  /* iterator over the immediate
   * child nodes */
  TrieIter2<KeyT, DataT> *subIter; /* recursive iteration over
                                    * children's children etc. */
  bool done;                       /* flag for level=0 iterator */
};

}

#include "Trie-inl.h"

