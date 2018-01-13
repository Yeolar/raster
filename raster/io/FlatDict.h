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

#include "raster/io/Cursor.h"
#include "raster/io/RecordIO.h"
#include "raster/io/TypedIOBuf.h"
#include "raster/thread/AtomicUnorderedMap.h"
#include "raster/thread/SharedMutex.h"
#include "raster/util/Time.h"

namespace rdd {

namespace detail {

struct MutableLockedIOBuf {
  explicit MutableLockedIOBuf(std::unique_ptr<IOBuf>&& init)
    : buf_(std::move(init)) {}

  std::unique_ptr<IOBuf> clone() const {
    SharedMutex::ReadHolder guard(lock_);
    return buf_ ? buf_->cloneOne() : nullptr;
  }

  void reset(std::unique_ptr<IOBuf>&& other) const {
    SharedMutex::WriteHolder guard(lock_);
    buf_ = std::move(other);
  }

 private:
  mutable std::unique_ptr<IOBuf> buf_;
  mutable SharedMutex lock_;
};

} // namespace detail

/**
 * A synchronized buffer dictionary based on AtomicUnorderedMap.
 */
template <
  typename Key,
  typename Hash = std::hash<Key>,
  typename KeyEqual = std::equal_to<Key>,
  typename IndexType = uint32_t,
  typename Allocator = MMapAlloc>
class FlatDict {
 public:
  struct Block {
    Block(std::unique_ptr<IOBuf>&& buf) : buf_(std::move(buf)) {
      if (buf_) {
        io::Cursor cursor(buf_.get());
        key = cursor.read<Key>();
        ts = cursor.read<uint64_t>();
        data.reset(cursor.data(), cursor.length());
      }
    }

    Key key{0};
    uint64_t ts{0};
    ByteRange data;

    explicit operator bool() const {
      return buf_.get() != nullptr;
    }

    static constexpr size_t kHeadSize = sizeof(Key) + sizeof(uint64_t);

   private:
    std::unique_ptr<IOBuf> buf_;
  };

 public:
  explicit FlatDict(size_t maxSize, const std::string& path = "")
    : map_(maxSize),
      path_(path),
      sync_(false) {
  }

  ~FlatDict() {
    sync();
  }

  void setSyncPath(const std::string& path) {
    path_ = path;
  }

  void load(const std::string& path) {
    if (!sync_.exchange(true)) {
      RecordIOReader reader(std::move(File(path)));
      for (auto& record : reader) {
        std::unique_ptr<IOBuf> buf(IOBuf::createCombined(record.first.size()));
        io::Appender appender(buf.get(), 0);
        appender.push(record.first);
        Key key = *TypedIOBuf<Key>(buf.get()).data();
        update(key, std::move(buf));
      }
      sync_ = false;
    }
  }

  void sync() {
    if (!path_.empty() && !sync_.exchange(true)) {
      std::unique_ptr<IOBuf> head(IOBuf::createCombined(1));
      auto it = map_.cbegin();
      while (it != map_.cend()) {
        auto buf = (it++)->second.clone();
        if (buf) {
          head->prependChain(std::move(buf));
        }
      }
      RecordIOWriter writer(File(path_, O_WRONLY | O_CREAT | O_TRUNC));
      writer.write(std::move(head->pop()));
      sync_ = false;
    }
  }

  Block get(Key key) {
    std::unique_ptr<IOBuf> buf;
    auto it = map_.find(key);
    if (it != map_.cend()) {
      buf = std::move(it->second.clone());
    }
    return Block(std::move(buf));
  }

  void erase(Key key) {
    std::unique_ptr<IOBuf> buf;
    auto it = map_.find(key);
    if (it != map_.cend()) {
      it->second.reset(std::move(buf));
    }
  }

  void update(Key key, ByteRange range, uint64_t ts = timestampNow()) {
    std::unique_ptr<IOBuf> buf(
        IOBuf::createCombined(Block::kHeadSize + range.size()));
    io::Appender appender(buf.get(), 0);
    appender.write(key);
    appender.write(ts);
    appender.push(range);
    update(key, std::move(buf));
  }

  size_t size() {
    size_t n = 0;
    auto it = map_.cbegin();
    while (it != map_.cend()) {
      ++n;
      ++it;
    }
    return n;
  }

 private:
  void update(Key key, std::unique_ptr<IOBuf>&& buf) {
    auto p = map_.emplace(key, std::move(buf));
    if (!p.second) {
      p.first->second.reset(std::move(buf));
    }
  }

  AtomicUnorderedInsertMap<
    Key,
    detail::MutableLockedIOBuf,
    Hash,
    KeyEqual,
    (boost::has_trivial_destructor<Key>::value &&
     boost::has_trivial_destructor<detail::MutableLockedIOBuf>::value),
    IndexType,
    Allocator> map_;
  std::string path_;
  std::atomic<bool> sync_;
};

template <
  typename Key,
  typename Hash = std::hash<Key>,
  typename KeyEqual = std::equal_to<Key>,
  typename Allocator = MMapAlloc>
using FlatDict64 = FlatDict<Key, Hash, KeyEqual, uint64_t, Allocator>;

} // namespace rdd
