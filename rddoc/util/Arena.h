/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <limits>
#include <stdexcept>
#include <boost/intrusive/slist.hpp>
#include "rddoc/util/noncopyable.h"
#include "rddoc/util/ThreadUtil.h"

namespace rdd {

class Arena : noncopyable {
private:
  typedef boost::intrusive::slist_member_hook<boost::intrusive::tag<Arena>>
    BlockLink;

  struct RDD_ALIGNED(16) Block {
    BlockLink link;

    static std::pair<Block*, size_t> allocate(size_t size) {
      void* mem = ::malloc(sizeof(Block) + size);
      if (!mem)
        throw std::bad_alloc();
      return std::make_pair(new (mem) Block(), size);
    }
    void deallocate() {
      this->~Block();
      ::free(this);
    }

    char* start() { return reinterpret_cast<char*>(this + 1); }
  };

  static void block_deallocate(Block* b) { b->deallocate(); }

public:
  static const size_t BLOCK_OVERHEAD = sizeof(Block);
  static const size_t MIN_BLOCK_SIZE = 4096 - sizeof(Block);

  explicit Arena(size_t minBlockSize = MIN_BLOCK_SIZE)
    : ptr_(nullptr),
      end_(nullptr),
      totalAllocatedSize_(0),
      bytesUsed_(0),
      minBlockSize_(minBlockSize) {}

  ~Arena() {
    while (!blocks_.empty()) {
      blocks_.pop_front_and_dispose(block_deallocate);
    }
  }

  void* allocate(size_t size) {
    size = roundUp(size);
    bytesUsed_ += size;
    if ((size_t)(end_ - ptr_) >= size) {
      char* r = ptr_;
      ptr_ += size;
      return r;
    }
    void* r = allocateSlow(size);
    return r;
  }

  void deallocate(void* p) {}

  size_t totalSize() const { return totalAllocatedSize_ + sizeof(Arena); }
  size_t bytesUsed() const { return bytesUsed_; }

private:
  size_t roundUp(size_t size) const {
    return (size + 15) & ~15;
  }

  // cache_last<true> makes the list keep a pointer to the last element,
  // so we have push_back()
  typedef boost::intrusive::slist<
    Block, boost::intrusive::member_hook<Block, BlockLink, &Block::link>,
    boost::intrusive::constant_time_size<false>,
    boost::intrusive::cache_last<true>> BlockList;

  void* allocateSlow(size_t size);

  BlockList blocks_;
  char* ptr_;
  char* end_;
  size_t totalAllocatedSize_;
  size_t bytesUsed_;
  const size_t minBlockSize_;
};

class ThreadArena : noncopyable {
public:
  explicit ThreadArena(size_t minBlockSize = Arena::MIN_BLOCK_SIZE)
    : minBlockSize_(minBlockSize) {}

  void* allocate(size_t size) {
    Arena* arena = arena_.get();
    if (!arena) {
      arena = allocateThreadLocalArena();
    }
    return arena->allocate(size);
  }

  void deallocate(void* p) {}

private:
  Arena* allocateThreadLocalArena() {
    Arena* arena = new Arena(minBlockSize_);
    arena_.reset(arena);
    return arena;
  }

  const size_t minBlockSize_;
  ThreadLocalPtr<Arena> arena_;
};

}  // namespace rdd
