/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <vector>
#include <boost/iterator/iterator_facade.hpp>

#include "raster/io/Path.h"
#include "raster/thread/RWSpinLock.h"
#include "raster/util/MemoryMapping.h"

namespace rdd {

class MMapIOAlloc {
 public:
  virtual ~MMapIOAlloc() {}

  virtual void* allocate(size_t size) = 0;
  virtual void deallocate(void* p, size_t size) = 0;
};

namespace detail {

struct ChunkHead {
  enum { INIT, FREE, USED };

  uint16_t index;
  uint16_t flag;
  uint32_t size;
};

ByteRange findChunk(ByteRange range);

} // namespace detail

class MMapIO : public MMapIOAlloc {
 public:
  enum { INIT, FREE, FULL };

  struct Head {
    uint16_t index;
    uint16_t flag;
    uint32_t used;
    uint32_t tail;
  };

  class Iterator : public boost::iterator_facade<
      Iterator,
      const std::pair<ByteRange, off_t>,
      boost::forward_traversal_tag> {
    friend class boost::iterator_core_access;
    friend class MMapIO;
   private:
    Iterator(ByteRange range, off_t pos);

    reference dereference() const { return chunkAndPos_; }
    bool equal(const Iterator& other) const { return range_ == other.range_; }

    void increment() {
      size_t skip = sizeof(detail::ChunkHead) + chunkAndPos_.first.size();
      chunkAndPos_.second += off_t(skip);
      range_.advance(skip);
      advanceToValid();
    }

    void advanceToValid();

    ByteRange range_;
    // stored as a pair so we can return by reference in dereference()
    std::pair<ByteRange, off_t> chunkAndPos_;
  };

  typedef Iterator iterator;
  typedef Iterator const_iterator;
  typedef std::pair<ByteRange, off_t> value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  MMapIO(const Path& path, size_t size)
    : map_(path.c_str(), 0, size, MemoryMapping::writable()) {}

  ~MMapIO() override {}

  void init(uint16_t i);

  void* allocate(size_t size) override;

  void deallocate(void* p, size_t size) override;

  uint16_t index() const;
  bool isFree() const;
  bool isFull() const;
  float useRate() const;

  Iterator cbegin() const { return Iterator(map_.range(), sizeof(Head)); }
  Iterator begin() const { return cbegin(); }
  Iterator cend() const { return Iterator(ByteRange(), 0); }
  Iterator end() const { return cend(); }

 private:
  Head* head() const {
    return (Head*) map_.writableRange().data();
  }

  uint8_t* data() const {
    return map_.writableRange().data() + sizeof(Head);
  }

  size_t headSize() const { return sizeof(Head); }
  size_t dataSize() const { return map_.range().size() - sizeof(Head); }

  MemoryMapping map_;
  mutable RWSpinLock lock_;
};

class MMapIOPool : public MMapIOAlloc {
 public:
  typedef typename std::unique_ptr<MMapIO> MMapIOPtr;
  typedef typename std::vector<MMapIOPtr>::iterator iterator;
  typedef typename std::vector<MMapIOPtr>::const_iterator const_iterator;

  MMapIOPool(const Path& dir, size_t blockSize, uint16_t initCount = 0);

  ~MMapIOPool() override {}

  void* allocate(size_t size) override;

  void deallocate(void* p, size_t size) override;

  iterator begin() { return pool_.begin(); }
  const_iterator cbegin() const { return pool_.cbegin(); }

  iterator end() { return pool_.end(); }
  const_iterator cend() const { return pool_.cend(); }

 private:
  void loadOrCreate(uint16_t i);

  void* allocateOnFixed(size_t size);

  Path dir_;
  size_t blockSize_;
  std::vector<MMapIOPtr> pool_;
  std::deque<uint16_t> frees_;
  std::mutex lock_;
};

} // namespace rdd
