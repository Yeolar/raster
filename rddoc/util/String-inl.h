/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdexcept>
#include <iterator>

namespace rdd {

template <class InputString>
bool hexlify(const InputString& input, std::string& output,
             bool append_output) {
  if (!append_output)
    output.clear();

  static char hexValues[] = "0123456789abcdef";
  auto j = output.size();
  output.resize(2 * input.size() + output.size());
  for (size_t i = 0; i < input.size(); ++i) {
    int ch = input[i];
    output[j++] = hexValues[(ch >> 4) & 0xf];
    output[j++] = hexValues[ch & 0xf];
  }
  return true;
}

inline int unhex(char c) {
  return c >= '0' && c <= '9' ? c - '0' :
         c >= 'A' && c <= 'F' ? c - 'A' + 10 :
         c >= 'a' && c <= 'f' ? c - 'a' + 10 :
         -1;
};

template <class InputString>
bool unhexlify(const InputString& input, std::string& output) {
  if (input.size() % 2 != 0) {
    return false;
  }
  output.resize(input.size() / 2);
  int j = 0;
  for (size_t i = 0; i < input.size(); i += 2) {
    int highBits = unhex(input[i]);
    int lowBits = unhex(input[i + 1]);
    if (highBits < 0 || lowBits < 0) {
      return false;
    }
    output[j++] = (highBits << 4) + lowBits;
  }
  return true;
}

namespace detail {

/*
 * The following functions are type-overloaded helpers for
 * internalSplit().
 */
inline size_t delimSize(char) { return 1; }
inline size_t delimSize(StringPiece s) { return s.size(); }
inline bool atDelim(const char* s, char c) { return *s == c; }
inline bool atDelim(const char* s, StringPiece sp) {
  return !std::memcmp(s, sp.start(), sp.size());
}

/*
 * The following functions are type-overloaded helpers for
 * internalSplitAny().
 */
inline size_t delimAnySize(char) { return 1; }
inline size_t delimAnySize(StringPiece s) { return std::min(s.size(), 1ul); }
inline bool atDelimAny(const char* s, char c) { return *s == c; }
inline bool atDelimAny(const char* s, StringPiece sp) {
  return sp.contains(*s);
}

// These are used to short-circuit internalSplit() in the case of
// 1-character strings.
inline char delimFront(char c) {
  // This one exists only for compile-time; it should never be called.
  std::abort();
  return c;
}
inline char delimFront(StringPiece s) {
  assert(!s.empty() && s.start() != nullptr);
  return *s.start();
}

/*
 * These output conversion templates allow us to support multiple
 * output string types, even when we are using an arbitrary
 * OutputIterator.
 */
template <class OutStringT>
struct OutputConverter {};

template <>
struct OutputConverter<std::string> {
  std::string operator()(StringPiece sp) const {
    return sp.toString();
  }
};

template <>
struct OutputConverter<StringPiece> {
  StringPiece operator()(StringPiece sp) const {
    return sp;
  }
};

/*
 * Shared implementation for all the split() overloads.
 *
 * This uses some external helpers that are overloaded to let this
 * algorithm be more performant if the deliminator is a single
 * character instead of a whole string.
 *
 * @param ignoreEmpty iff true, don't copy empty segments to output
 */
template <class OutStringT, class DelimT, class OutputIterator>
void internalSplit(DelimT delim, StringPiece sp, OutputIterator out,
                   bool ignoreEmpty) {
  assert(sp.empty() || sp.start() != nullptr);

  const char* s = sp.start();
  const size_t strSize = sp.size();
  const size_t dSize = delimSize(delim);

  OutputConverter<OutStringT> conv;

  if (dSize > strSize || dSize == 0) {
    if (!ignoreEmpty || strSize > 0) {
      *out++ = conv(sp);
    }
    return;
  }
  if (std::is_same<DelimT, StringPiece>::value && dSize == 1) {
    // Call the char version because it is significantly faster.
    return internalSplit<OutStringT>(delimFront(delim), sp, out, ignoreEmpty);
  }

  size_t tokenStartPos = 0;
  size_t tokenSize = 0;
  for (size_t i = 0; i <= strSize - dSize; ++i) {
    if (atDelim(&s[i], delim)) {
      if (!ignoreEmpty || tokenSize > 0) {
        *out++ = conv(StringPiece(&s[tokenStartPos], tokenSize));
      }

      tokenStartPos = i + dSize;
      tokenSize = 0;
      i += dSize - 1;
    } else {
      ++tokenSize;
    }
  }
  tokenSize = strSize - tokenStartPos;
  if (!ignoreEmpty || tokenSize > 0) {
    *out++ = conv(StringPiece(&s[tokenStartPos], tokenSize));
  }
}

/*
 * Shared implementation for all the splitAny() overloads.
 *
 * @param ignoreEmpty iff true, don't copy empty segments to output
 */
template <class OutStringT, class DelimT, class OutputIterator>
void internalSplitAny(DelimT delim, StringPiece sp, OutputIterator out,
                      bool ignoreEmpty) {
  assert(sp.empty() || sp.start() != nullptr);

  const char* s = sp.start();
  const size_t strSize = sp.size();
  const size_t dSize = delimAnySize(delim);

  OutputConverter<OutStringT> conv;

  if (strSize == 0 || dSize == 0) {
    if (!ignoreEmpty || strSize > 0) {
      *out++ = conv(sp);
    }
    return;
  }

  size_t tokenStartPos = 0;
  size_t tokenSize = 0;
  for (size_t i = 0; i < strSize; ++i) {
    if (atDelimAny(&s[i], delim)) {
      if (!ignoreEmpty || tokenSize > 0) {
        *out++ = conv(StringPiece(&s[tokenStartPos], tokenSize));
      }

      tokenStartPos = i + 1;
      tokenSize = 0;
    } else {
      ++tokenSize;
    }
  }
  tokenSize = strSize - tokenStartPos;
  if (!ignoreEmpty || tokenSize > 0) {
    *out++ = conv(StringPiece(&s[tokenStartPos], tokenSize));
  }
}

template <class String>
StringPiece prepareDelim(const String& s) {
  return StringPiece(s);
}
inline char prepareDelim(char c) { return c; }

template <class Dst>
struct convertTo {
  template <class Src>
  static Dst from(const Src& src) {
    return rdd::to<Dst>(src);
  }
  static Dst from(const Dst& src) {
    return src;
  }
};

template <bool exact, class Delim, class OutputType>
typename std::enable_if<IsSplitTargetType<OutputType>::value, bool>::type
splitFixed(const Delim& delimiter, StringPiece input, OutputType& out) {
  if (exact && std::string::npos != input.find(delimiter)) {
    return false;
  }
  out = convertTo<OutputType>::from(input);
  return true;
}

template <bool exact, class Delim, class OutputType, class... OutputTypes>
typename std::enable_if<IsSplitTargetType<OutputType>::value, bool>::type
splitFixed(const Delim& delimiter, StringPiece input,
           OutputType& outHead, OutputTypes&... outTail) {
  size_t cut = input.find(delimiter);
  if (cut == std::string::npos) {
    return false;
  }
  StringPiece head(input.begin(), input.begin() + cut);
  StringPiece tail(input.begin() + cut + detail::delimSize(delimiter),
                   input.end());
  if (splitFixed<exact>(delimiter, tail, outTail...)) {
    outHead = convertTo<OutputType>::from(head);
    return true;
  }
  return false;
}

} // namespace detail

//////////////////////////////////////////////////////////////////////

template <class Delim, class String, class OutputType>
void split(const Delim& delimiter, const String& input,
           std::vector<OutputType>& out, bool ignoreEmpty) {
  detail::internalSplit<OutputType>(detail::prepareDelim(delimiter),
                                    StringPiece(input),
                                    std::back_inserter(out), ignoreEmpty);
}

template <class OutputValueType, class Delim, class String,
          class OutputIterator>
void splitTo(const Delim& delimiter, const String& input, OutputIterator out,
             bool ignoreEmpty) {
  detail::internalSplit<OutputValueType>(
    detail::prepareDelim(delimiter), StringPiece(input), out, ignoreEmpty);
}

template <bool exact, class Delim, class OutputType, class... OutputTypes>
typename std::enable_if<IsSplitTargetType<OutputType>::value, bool>::type split(
  const Delim& delimiter, StringPiece input, OutputType& outHead,
  OutputTypes&... outTail) {
  return detail::splitFixed<exact>(detail::prepareDelim(delimiter), input,
                                   outHead, outTail...);
}

template <class Delim, class String, class OutputType>
void splitAny(const Delim& delimiters, const String& input,
              std::vector<OutputType>& out, bool ignoreEmpty) {
  detail::internalSplitAny<OutputType>(detail::prepareDelim(delimiters),
                                       StringPiece(input),
                                       std::back_inserter(out), ignoreEmpty);
}

template <class OutputValueType, class Delim, class String,
          class OutputIterator>
void splitAnyTo(const Delim& delimiters, const String& input, OutputIterator out,
                bool ignoreEmpty) {
  detail::internalSplitAny<OutputValueType>(
    detail::prepareDelim(delimiters), StringPiece(input), out, ignoreEmpty);
}

namespace detail {

/*
 * If a type can have its string size determined cheaply, we can more
 * efficiently append it in a loop (see internalJoinAppend). Note that the
 * struct need not conform to the std::string api completely (ex. does not need
 * to implement append()).
 */
template <class T>
struct IsSizableString {
  enum {
    value = std::is_same<T, std::string>::value ||
        std::is_same<T, StringPiece>::value
  };
};

template <class Iterator>
struct IsSizableStringContainerIterator
  : IsSizableString<typename std::iterator_traits<Iterator>::value_type> {};

template <class Delim, class Iterator>
void internalJoinAppend(Delim delimiter, Iterator begin, Iterator end,
                        std::string& output) {
  assert(begin != end);
  if (std::is_same<Delim, StringPiece>::value && delimSize(delimiter) == 1) {
    internalJoinAppend(delimFront(delimiter), begin, end, output);
    return;
  }
  toAppend(*begin, &output);
  while (++begin != end) {
    toAppend(delimiter, &output);
    toAppend(*begin, &output);
  }
}

template <class Delim, class Iterator>
typename std::enable_if<IsSizableStringContainerIterator<Iterator>::value>::type
internalJoin(Delim delimiter, Iterator begin, Iterator end,
             std::string& output) {
  output.clear();
  if (begin == end) {
    return;
  }
  const size_t dsize = delimSize(delimiter);
  Iterator it = begin;
  size_t size = it->size();
  while (++it != end) {
    size += dsize + it->size();
  }
  output.reserve(size);
  internalJoinAppend(delimiter, begin, end, output);
}

template <class Delim, class Iterator>
typename std::enable_if<
  !IsSizableStringContainerIterator<Iterator>::value>::type
internalJoin(Delim delimiter, Iterator begin, Iterator end,
             std::string& output) {
  output.clear();
  if (begin == end) {
    return;
  }
  internalJoinAppend(delimiter, begin, end, output);
}

template <class Delim, class Pair>
void toAppendPair(Delim delimiter, Pair pair, std::string& output) {
  toAppend(pair.first, &output);
  toAppend(delimiter, &output);
  toAppend(pair.second, &output);
}

template <class Delim, class Iterator>
void internalMapJoinAppend(Delim delimiter, Delim pairDelimiter, Iterator begin,
                           Iterator end, std::string& output) {
  assert(begin != end);
  toAppendPair(pairDelimiter, *begin, output);
  while (++begin != end) {
    toAppend(delimiter, &output);
    toAppendPair(pairDelimiter, *begin, output);
  }
}

} // namespace detail

template <class Delim, class Iterator>
void join(const Delim& delimiter, Iterator begin, Iterator end,
          std::string& output) {
  detail::internalJoin(detail::prepareDelim(delimiter), begin, end, output);
}

template <class Delim, class Iterator>
void joinMap(const Delim& delimiter, const Delim& pairDelimiter, Iterator begin,
             Iterator end, std::string& output) {
  output.clear();
  if (begin == end) {
    return;
  }
  detail::internalMapJoinAppend(delimiter, pairDelimiter, begin, end, output);
}

} // namespace rdd
