/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/FlatDict.h"
#include "raster/io/MMapIOAlloc.h"

namespace rdd {

template <
  typename Key,
  typename Hash = std::hash<Key>,
  typename KeyEqual = std::equal_to<Key>,
  typename IndexType = uint32_t,
  typename Allocator = MMapAlloc>
class MMapFlatDict {
public:
  using Block =
    typename FlatDict<Key, Hash, KeyEqual, uint64_t, Allocator>::Block;

public:
  explicit MMapFlatDict(size_t maxSize, std::unique_ptr<MMapIOAlloc>&& pool)
    : map_(maxSize),
      pool_(std::move(pool)) {
    for (auto& mio : pool_) {
      for (auto& chunk : mio) {
        size_t n = chunk.first.size();
        std::unique_ptr<IOBuf> buf(
            IOBuf::takeOwnership(
                chunk.first.data(), n,
                [&](void* p, void*) { pool_->deallocate(p, n); }));
        Key key = *TypedIOBuf<Key>(buf.get()).data();
        update(key, std::move(buf));
      }
    }
  }

  ~MMapFlatDict() {}

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
    size_t n = Block::kHeadSize + range.size();
    std::unique_ptr<IOBuf> buf(
        IOBuf::takeOwnership(
            pool_->allocate(n), n,
            [&](void* p, void*) { pool_->deallocate(p, n); }));
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
  std::unique_ptr<MMapIOAlloc> pool_;
  fs::path path_;
};

template <
  typename Key,
  typename Hash = std::hash<Key>,
  typename KeyEqual = std::equal_to<Key>,
  typename Allocator = MMapAlloc>
using MMapFlatDict64 = MMapFlatDict<Key, Hash, KeyEqual, uint64_t, Allocator>;

} // namespace rdd
