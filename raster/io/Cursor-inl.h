/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

namespace rdd {
namespace io {
namespace detail {

/*
 * Helper classes for use with CursorBase::readWhile()
 */
class CursorStringAppender {
 public:
  void append(ByteRange bytes) {
    str_.append(reinterpret_cast<char const*>(bytes.data()), bytes.size());
  }
  std::string extractString() {
    return std::move(str_);
  }

 private:
  std::string str_;
};

class CursorNoopAppender {
 public:
  void append(ByteRange) {}
};

template <class Derived, class BufType>
std::string CursorBase<Derived, BufType>::readTerminatedString(
    char termChar,
    size_t maxLength) {
  size_t bytesRead{0};
  auto keepReading = [&bytesRead, termChar, maxLength](uint8_t byte) {
    if (byte == termChar) {
      return false;
    }
    ++bytesRead;
    if (bytesRead >= maxLength) {
      throw std::length_error("string overflow");
    }
    return true;
  };

  auto result = readWhile(keepReading);
  // skip over the terminator character
  if (isAtEnd()) {
    std::__throw_out_of_range("terminator not found");
  }
  skip(1);

  return result;
}

template <class Derived, class BufType>
template <typename Predicate>
std::string CursorBase<Derived, BufType>::readWhile(
    const Predicate& predicate) {
  CursorStringAppender s;
  readWhile(predicate, s);
  return s.extractString();
}

template <class Derived, class BufType>
template <typename Predicate, typename Output>
void CursorBase<Derived, BufType>::readWhile(
    const Predicate& predicate,
    Output& out) {
  while (true) {
    auto peeked = peekBytes();
    if (peeked.empty()) {
      return;
    }
    for (size_t idx = 0; idx < peeked.size(); ++idx) {
      if (!predicate(peeked[idx])) {
        peeked.reset(peeked.data(), idx);
        out.append(peeked);
        skip(idx);
        return;
      }
    }
    out.append(peeked);
    skip(peeked.size());
  }
}

template <class Derived, class BufType>
template <typename Predicate>
void CursorBase<Derived, BufType>::skipWhile(const Predicate& predicate) {
  CursorNoopAppender appender;
  readWhile(predicate, appender);
}

} // namespace detail
} // namespace io
} // namespace rdd
