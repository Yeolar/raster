/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _THRIFT_THRIFT_H_
#define _THRIFT_THRIFT_H_ 1

#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <string>
#include <map>
#include <list>
#include <set>
#include <vector>
#include <exception>
#include <typeinfo>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_convertible.hpp>

#define T_VIRTUAL_CALL() //fprintf(stderr, "[%s,%d] virtual call\n", __FILE__, __LINE__)

#define THRIFT_UNUSED_VARIABLE(x) ((void)(x))

namespace apache {
namespace thrift {

class TEnumIterator
    : public std::iterator<std::forward_iterator_tag, std::pair<int, const char*> > {
public:
  TEnumIterator(int n, int* enums, const char** names)
    : ii_(0), n_(n), enums_(enums), names_(names) {}

  int operator++() { return ++ii_; }

  bool operator!=(const TEnumIterator& end) {
    THRIFT_UNUSED_VARIABLE(end);
    assert(end.n_ == -1);
    return (ii_ != n_);
  }

  std::pair<int, const char*> operator*() const { return std::make_pair(enums_[ii_], names_[ii_]); }

private:
  int ii_;
  const int n_;
  int* enums_;
  const char** names_;
};

class TException : public std::exception {
public:
  TException() : message_() {}

  TException(const std::string& message) : message_(message) {}

  virtual ~TException() throw() {}

  virtual const char* what() const throw() {
    if (message_.empty()) {
      return "Default TException.";
    } else {
      return message_.c_str();
    }
  }

protected:
  std::string message_;
};

class TDelayedException {
public:
  template <class E>
  static TDelayedException* delayException(const E& e);
  virtual void throw_it() = 0;
  virtual ~TDelayedException(){};
};

template <class E>
class TExceptionWrapper : public TDelayedException {
public:
  TExceptionWrapper(const E& e) : e_(e) {}
  virtual void throw_it() {
    E temp(e_);
    delete this;
    throw temp;
  }

private:
  E e_;
};

template <class E>
TDelayedException* TDelayedException::delayException(const E& e) {
  return new TExceptionWrapper<E>(e);
}

}
} // apache::thrift

#endif // #ifndef _THRIFT_THRIFT_H_
