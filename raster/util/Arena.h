/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <limits>
#include <stdexcept>
#include <boost/intrusive/slist.hpp>

#include "raster/util/Conv.h"
#include "raster/util/ThreadUtil.h"

namespace rdd {

class Arena {
 public:
  explicit Arena(size_t minBlockSize = kMinBlockSize,
                 size_t maxAlign = kDefaultMaxAlign)
    : ptr_(nullptr),
      end_(nullptr),
      totalAllocatedSize_(0),
      bytesUsed_(0),
      minBlockSize_(minBlockSize),
      maxAlign_(maxAlign) {
    if ((maxAlign_ & (maxAlign_ - 1)) || maxAlign_ > alignof(Block)) {
      throw std::invalid_argument(
          to<std::string>("Invalid maxAlign: ", maxAlign_));
    }
  }

  ~Arena() {
    while (!blocks_.empty()) {
      blocks_.pop_front_and_dispose(block_deallocate);
    }
  }

  void* allocate(size_t size) {
    size = roundUp(size);
    bytesUsed_ += size;
    if (LIKELY((size_t)(end_ - ptr_) >= size)) {
      char* r = ptr_;
      ptr_ += size;
      assert(isAligned(r));
      return r;
    }
    void* r = allocateSlow(size);
    assert(isAligned(r));
    return r;
  }

  void deallocate(void* /* p */) {}

  size_t totalSize() const {
    return totalAllocatedSize_ + sizeof(Arena);
  }

  size_t bytesUsed() const {
    return bytesUsed_;
  }

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

 private:
  typedef boost::intrusive::slist_member_hook<
    boost::intrusive::tag<Arena>> BlockLink;

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

    char* start() {
      return reinterpret_cast<char*>(this + 1);
    }
  };

  static void block_deallocate(Block* b) {
    b->deallocate();
  }

 public:
  static constexpr size_t kMinBlockSize = 4096 - sizeof(Block);
  static constexpr size_t kDefaultMaxAlign = alignof(Block);
  static constexpr size_t kBlockOverhead = sizeof(Block);

 private:
  bool isAligned(uintptr_t address) const {
    return (address & (maxAlign_ - 1)) == 0;
  }
  bool isAligned(void* p) const {
    return isAligned(reinterpret_cast<uintptr_t>(p));
  }

  size_t roundUp(size_t size) const {
    return (size + maxAlign_ - 1) & ~(maxAlign_ - 1);
  }

  // cache_last<true> makes the list keep a pointer to the last element,
  // so we have push_back()
  typedef boost::intrusive::slist<
    Block,
    boost::intrusive::member_hook<Block, BlockLink, &Block::link>,
    boost::intrusive::constant_time_size<false>,
    boost::intrusive::cache_last<true>> BlockList;

  void* allocateSlow(size_t size);

  BlockList blocks_;
  char* ptr_;
  char* end_;
  size_t totalAllocatedSize_;
  size_t bytesUsed_;
  const size_t minBlockSize_;
  const size_t maxAlign_;
};

class ThreadArena {
 public:
  explicit ThreadArena(size_t minBlockSize = Arena::kMinBlockSize,
                       size_t maxAlign = Arena::kDefaultMaxAlign)
    : minBlockSize_(minBlockSize),
      maxAlign_(maxAlign) {}

  void* allocate(size_t size) {
    Arena* arena = arena_.get();
    if (UNLIKELY(!arena)) {
      arena = allocateThreadLocalArena();
    }
    return arena->allocate(size);
  }

  void deallocate(void* /* p */) {}

  ThreadArena(const ThreadArena&) = delete;
  ThreadArena& operator=(const ThreadArena&) = delete;

  ThreadArena(ThreadArena&&) = delete;
  ThreadArena& operator=(ThreadArena&&) = delete;

 private:
  Arena* allocateThreadLocalArena();

  const size_t minBlockSize_;
  const size_t maxAlign_;
  ThreadLocalPtr<Arena> arena_;
};

} // namespace rdd
