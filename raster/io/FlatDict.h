/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/Cursor.h"
#include "raster/io/FSUtil.h"
#include "raster/io/RecordIO.h"
#include "raster/io/TypedIOBuf.h"
#include "raster/util/AtomicUnorderedMap.h"
#include "raster/util/SpinLock.h"

namespace rdd {

namespace detail {

struct MutableLockedIOBuf {
  explicit MutableLockedIOBuf(std::unique_ptr<IOBuf>&& init)
    : buf_(std::move(init)) {}

  std::unique_ptr<IOBuf> clone() const {
    SpinLockGuard guard(lock_);
    return buf_ ? buf_->cloneOne() : nullptr;
  }

  void reset(std::unique_ptr<IOBuf>&& other) const {
    SpinLockGuard guard(lock_);
    buf_ = std::move(other);
  }

private:
  mutable std::unique_ptr<IOBuf> buf_;
  mutable SpinLock lock_;
};

} // namespace detail

/**
 * A synchronized buffer dictionary based on AtomicUnorderedMap.
 */
template <class Key, class IndexType = uint32_t>
class FlatDict {
  typedef AtomicUnorderedInsertMap<
    Key,
    detail::MutableLockedIOBuf,
    std::hash<Key>,
    std::equal_to<Key>,
    (boost::has_trivial_destructor<Key>::value &&
     boost::has_trivial_destructor<detail::MutableLockedIOBuf>::value),
    IndexType> Map;

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
  explicit FlatDict(size_t maxSize, fs::path path)
    : map_(maxSize),
      path_(path),
      sync_(false) {
  }

  ~FlatDict() {
    sync();
  }

  void setSyncPath(fs::path path) {
    path_ = path;
  }

  void load(fs::path path) {
    if (!sync_.exchange(true)) {
      RecordIOReader reader(File(path.string()));
      for (auto& record : reader) {
        std::unique_ptr<IOBuf> buf(IOBuf::copyBuffer(record.first));
        Key key = *TypedIOBuf<Key>(buf.get()).data();
        update(key, std::move(buf));
      }
      sync_ = false;
    }
  }

  void sync() {
    if (!path_.empty() && !sync_.exchange(true)) {
      std::unique_ptr<IOBuf> head(IOBuf::create(1));
      auto it = map_.cbegin();
      while (it != map_.cend()) {
        auto buf = (it++)->second.clone();
        if (buf) {
          head->prependChain(std::move(buf));
        }
      }
      RecordIOWriter writer(File(path_.string(), O_RDWR | O_CREAT));
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
    std::unique_ptr<IOBuf> buf(IOBuf::create(Block::kHeadSize + range.size()));
    io::Appender appender(buf.get(), 0);
    appender.write(key);
    appender.write(ts);
    appender.push(range);
    update(key, std::move(buf));
  }

private:
  void update(Key key, std::unique_ptr<IOBuf>&& buf) {
    auto p = map_.emplace(key, std::move(buf));
    if (!p.second) {
      p.first->second.reset(std::move(buf));
    }
  }

  Map map_;
  fs::path path_;
  std::atomic<bool> sync_;
};

} // namespace rdd
