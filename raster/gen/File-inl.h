/*
 * Copyright (C) 2017, Yeolar
 */

#ifndef RDD_GEN_FILE_H
#error This file may only be included from raster/gen/File.h
#endif

#include <system_error>

#include "raster/gen/String.h"

namespace rdd {
namespace gen {
namespace detail {

class FileReader : public GenImpl<ByteRange, FileReader> {
 public:
  FileReader(File file, std::unique_ptr<IOBuf> buffer)
    : file_(std::move(file)),
      buffer_(std::move(buffer)) {
    buffer_->clear();
  }

  template <class Body>
  bool apply(Body&& body) const {
    for (;;) {
      ssize_t n;
      do {
        n = ::read(file_.fd(), buffer_->writableTail(), buffer_->capacity());
      } while (n == -1 && errno == EINTR);
      if (n == -1) {
        throw std::system_error(errno, std::system_category(), "read failed");
      }
      if (n == 0) {
        return true;
      }
      if (!body(ByteRange(buffer_->tail(), n))) {
        return false;
      }
    }
  }

  // Technically, there could be infinite files (e.g. /dev/random), but people
  // who open those can do so at their own risk.
  static constexpr bool infinite = false;

 private:
  File file_;
  std::unique_ptr<IOBuf> buffer_;
};

class FileWriter : public Operator<FileWriter> {
 public:
  FileWriter(File file, std::unique_ptr<IOBuf> buffer)
    : file_(std::move(file)),
      buffer_(std::move(buffer)) {
    if (buffer_) {
      buffer_->clear();
    }
  }

  template <class Source, class Value>
  void compose(const GenImpl<Value, Source>& source) const {
    auto fn = [&](ByteRange v) {
      if (!this->buffer_ || v.size() >= this->buffer_->capacity()) {
        this->flushBuffer();
        this->write(v);
      } else {
        if (v.size() > this->buffer_->tailroom()) {
          this->flushBuffer();
        }
        memcpy(this->buffer_->writableTail(), v.data(), v.size());
        this->buffer_->append(v.size());
      }
    };

    // Iterate
    source.foreach(std::move(fn));

    flushBuffer();
    file_.close();
  }

 private:
  void write(ByteRange v) const {
    ssize_t n;
    while (!v.empty()) {
      do {
        n = ::write(file_.fd(), v.data(), v.size());
      } while (n == -1 && errno == EINTR);
      if (n == -1) {
        throw std::system_error(errno, std::system_category(),
                                "write() failed");
      }
      v.advance(n);
    }
  }

  void flushBuffer() const {
    if (buffer_ && buffer_->length() != 0) {
      write(ByteRange(buffer_->data(), buffer_->length()));
      buffer_->clear();
    }
  }

  mutable File file_;
  std::unique_ptr<IOBuf> buffer_;
};

}  // !detail

/**
 * Generator which reads lines from a file.
 * Note: This produces StringPieces which reference temporary strings which are
 * only valid during iteration.
 */
inline auto byLine(File file, char delim = '\n')
    -> decltype(fromFile(std::move(file))
                | eachAs<StringPiece>()
                | resplit(delim)) {
  return fromFile(std::move(file))
       | eachAs<StringPiece>()
       | resplit(delim);
}

inline auto byLine(int fd, char delim = '\n')
  -> decltype(byLine(File(fd), delim)) { return byLine(File(fd), delim); }

inline auto byLine(const char* f, char delim = '\n')
  -> decltype(byLine(File(f), delim)) { return byLine(File(f), delim); }

}}  // !rdd::gen
