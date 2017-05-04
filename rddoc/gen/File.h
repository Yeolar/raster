/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once
#define RDD_GEN_FILE_H

#include "rddoc/gen/Base.h"
#include "rddoc/io/File.h"
#include "rddoc/io/IOBuf.h"

namespace rdd {
namespace gen {

namespace detail {
class FileReader;
class FileWriter;
}  // namespace detail

/**
 * Generator that reads from a file with a buffer of the given size.
 * Reads must be buffered (the generator interface expects the generator
 * to hold each value).
 */
template <class S = detail::FileReader>
S fromFile(File file, size_t bufferSize=4096) {
  return S(std::move(file), IOBuf::create(bufferSize));
}

/**
 * Generator that reads from a file using a given buffer.
 */
template <class S = detail::FileReader>
S fromFile(File file, std::unique_ptr<IOBuf> buffer) {
  return S(std::move(file), std::move(buffer));
}

/**
 * Sink that writes to a file with a buffer of the given size.
 * If bufferSize is 0, writes will be unbuffered.
 */
template <class S = detail::FileWriter>
S toFile(File file, size_t bufferSize=4096) {
  return S(std::move(file), bufferSize ? nullptr : IOBuf::create(bufferSize));
}

/**
 * Sink that writes to a file using a given buffer.
 * If the buffer is nullptr, writes will be unbuffered.
 */
template <class S = detail::FileWriter>
S toFile(File file, std::unique_ptr<IOBuf> buffer) {
  return S(std::move(file), std::move(buffer));
}

}}  // !rdd::gen

#include "rddoc/gen/File-inl.h"

