/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include <memory>

#include "raster/io/FileUtil.h"
#include "raster/util/Logging.h"
#include "raster/util/Random.h"

namespace rdd {

namespace {

void readRandomDevice(void* data, size_t size) {
  // Keep the random device open for the duration of the program.
  static int randomFd = ::open("/dev/urandom", O_RDONLY);
  RDDPCHECK(randomFd >= 0);
  auto bytesRead = readFull(randomFd, data, size);
  RDDPCHECK(bytesRead >= 0 && size_t(bytesRead) == size);
}

class BufferedRandomDevice {
public:
  static constexpr size_t kDefaultBufferSize = 128;

  explicit BufferedRandomDevice(size_t bufferSize = kDefaultBufferSize)
    : bufferSize_(bufferSize),
      buffer_(new unsigned char[bufferSize]),
      ptr_(buffer_.get() + bufferSize) {  // refill on first use
  }

  void get(void* data, size_t size) {
    if (LIKELY(size <= remaining())) {
      memcpy(data, ptr_, size);
      ptr_ += size;
    } else {
      getSlow(static_cast<unsigned char*>(data), size);
    }
  }

private:
  void getSlow(unsigned char* data, size_t size) {
    if (size >= bufferSize_) {
      // Just read directly.
      readRandomDevice(data, size);
      return;
    }

    size_t copied = remaining();
    memcpy(data, ptr_, copied);
    data += copied;
    size -= copied;

    // refill
    readRandomDevice(buffer_.get(), bufferSize_);
    ptr_ = buffer_.get();

    memcpy(data, ptr_, size);
    ptr_ += size;
  }

  size_t remaining() const {
    return size_t(buffer_.get() + bufferSize_ - ptr_);
  }

  const size_t bufferSize_;
  std::unique_ptr<unsigned char[]> buffer_;
  unsigned char* ptr_;
};

} // namespace

void Random::secureRandom(void* data, size_t size) {
  static ThreadLocal<BufferedRandomDevice> bufferedRandomDevice;
  bufferedRandomDevice.get()->get(data, size);
}

ThreadLocal<std::default_random_engine> Random::rng;

} // namespace rdd
