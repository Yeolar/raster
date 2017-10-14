/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

/**
 * RecordIO: self-synchronizing stream of variable length records
 *
 * RecordIO gives you the ability to write a stream of variable length records
 * and read them later even in the face of data corruption -- randomly inserted
 * or deleted chunks of the file, or modified data.  When reading, you may lose
 * corrupted records, but the stream will resynchronize automatically.
 */

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <boost/iterator/iterator_facade.hpp>

#include "raster/io/File.h"
#include "raster/io/IOBuf.h"
#include "raster/util/MemoryMapping.h"
#include "raster/util/Range.h"
#include "raster/util/SpookyHashV2.h"

namespace rdd {

namespace recordio_helpers {

// We're exposing the guts of the RecordIO implementation for two reasons:
// 1. It makes unit testing easier, and
// 2. It allows you to build different RecordIO readers / writers that use
// different storage systems underneath (not standard files)

namespace detail {

struct Header {
  // First 4 bytes of SHA1("rdd"), big-endian
  // Any values will do, except that the sequence must not have a
  // repeated prefix (that is, if we see kMagic, we know that the next
  // occurrence must start at least 4 bytes later)
  static constexpr uint32_t kMagic = 0x9379e54d;
  uint32_t magic;
  uint8_t  version;       // backwards incompatible version, currently 0
  uint8_t  hashFunction;  // 0 = SpookyHashV2
  uint16_t flags;         // reserved (must be 0)
  uint32_t fileId;        // unique file ID
  uint32_t dataLength;
  uint64_t dataHash;
  uint32_t headerHash;  // must be last
} __attribute__((__packed__));

static_assert(offsetof(Header, headerHash) + sizeof(Header::headerHash) ==
              sizeof(Header), "invalid header layout");

} // namespace detail

/**
 * Header size.
 */
constexpr size_t headerSize() { return sizeof(detail::Header); }

/**
 * Write a header in the buffer.  We will prepend the header to the front
 * of the chain.  Do not write the buffer if empty (we don't allow empty
 * records).  Returns the total length, including header (0 if empty)
 * (same as buf->computeChainDataLength(), but likely faster)
 *
 * The fileId should be unique per stream and allows you to have RecordIO
 * headers stored inside the data (for example, have an entire RecordIO
 * file stored as a record inside another RecordIO file).  The fileId may
 * not be 0.
 */
size_t prependHeader(std::unique_ptr<IOBuf>& buf, uint32_t fileId = 1);

/**
 * Search for the first valid record that begins in searchRange (which must be
 * a subrange of wholeRange).  Returns the record data (not the header) if
 * found, ByteRange() otherwise.
 *
 * The fileId may be 0, in which case we'll return the first valid record for
 * *any* fileId, or non-zero, in which case we'll only look for records with
 * the requested fileId.
 */
struct RecordInfo {
  uint32_t fileId;
  ByteRange record;
};
RecordInfo findRecord(ByteRange searchRange,
                      ByteRange wholeRange,
                      uint32_t fileId);

/**
 * Search for the first valid record in range.
 */
inline RecordInfo findRecord(ByteRange range, uint32_t fileId) {
  return findRecord(range, range, fileId);
}

/**
 * Check if there is a valid record at the beginning of range.  Returns the
 * record data (not the header) if the record is valid, ByteRange() otherwise.
 */
RecordInfo validateRecord(ByteRange range, uint32_t fileId);

} // namespace recordio_helpers

/**
 * Class to write a stream of RecordIO records to a file.
 *
 * RecordIOWriter is thread-safe
 */
class RecordIOWriter {
 public:
  /**
   * Create a RecordIOWriter around a file; will append to the end of
   * file if it exists.
   *
   * Each file must have a non-zero file id, which is embedded in all
   * record headers.  Readers will only return records with the requested
   * file id (or, if the reader is created with fileId=0 in the constructor,
   * the reader will return all records).  File ids are only used to allow
   * resynchronization if you store RecordIO records (with headers) inside
   * other RecordIO records (for example, if a record consists of a fragment
   * from another RecordIO file).  If you're not planning to do that,
   * the defaults are fine.
   */
  explicit RecordIOWriter(File file, uint32_t fileId = 1);

  /**
   * Write a record.  We will use at most headerSize() bytes of headroom,
   * you might want to arrange that before copying your data into it.
   */
  void write(std::unique_ptr<IOBuf> buf);

  /**
   * Return the position in the file where the next byte will be written.
   * Conservative, as stuff can be written at any time from another thread.
   */
  off_t filePos() const { return filePos_; }

 private:
  File file_;
  uint32_t fileId_;
  std::unique_lock<File> writeLock_;
  std::atomic<off_t> filePos_;
};

/**
 * Class to read from a RecordIO file.  Will skip invalid records.
 */
class RecordIOReader {
 public:
  class Iterator : public boost::iterator_facade<
      Iterator,
      const std::pair<ByteRange, off_t>,
      boost::forward_traversal_tag> {
    friend class boost::iterator_core_access;
    friend class RecordIOReader;
   private:
    Iterator(ByteRange range, uint32_t fileId, off_t pos);

    reference dereference() const { return recordAndPos_; }
    bool equal(const Iterator& other) const { return range_ == other.range_; }
    void increment() {
      size_t skip = recordio_helpers::headerSize() + recordAndPos_.first.size();
      recordAndPos_.second += off_t(skip);
      range_.advance(skip);
      advanceToValid();
    }

    void advanceToValid();
    ByteRange range_;
    uint32_t fileId_;
    // stored as a pair so we can return by reference in dereference()
    std::pair<ByteRange, off_t> recordAndPos_;
  };

  /**
   * RecordIOReader is iterable, returning pairs of ByteRange (record content)
   * and position in file where the record (including header) begins.
   * Note that the position includes the header, that is, it can be passed back
   * to seek().
   */
  typedef Iterator iterator;
  typedef Iterator const_iterator;
  typedef std::pair<ByteRange, off_t> value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  /**
   * A record reader with a fileId of 0 will return all records.
   * A record reader with a non-zero fileId will only return records where
   * the fileId matches.
   */
  explicit RecordIOReader(File file, uint32_t fileId = 0);

  Iterator cbegin() const { return seek(0); }
  Iterator begin() const { return cbegin(); }
  Iterator cend() const { return seek(off_t(-1)); }
  Iterator end() const { return cend(); }

  /**
   * Create an iterator to the first valid record after pos.
   */
  Iterator seek(off_t pos) const {
    return Iterator(map_.range(), fileId_, pos);
  }

 private:
  MemoryMapping map_;
  uint32_t fileId_;
};

} // namespace rdd

