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

#include "raster/io/Path.h"

#include <deque>
#include <iterator>
#include <sys/types.h>
#include <unistd.h>

#include "raster/util/Exception.h"
#include "raster/util/String.h"

namespace rdd {

Path& Path::operator/=(StringPiece path) {
  append(path);
  return *this;
}

Path& Path::operator+=(StringPiece path) {
  Path(path_ + path.str()).swap(*this);
  return *this;
}

Path Path::parent() const {
  return *this / "..";
}

std::string Path::name() const {
  StringPiece sp(path_);
  auto p = sp.rfind('/');
  if (p != StringPiece::npos) {
    sp = sp.subpiece(p+1);
  }
  return sp.str();
}

std::string Path::base() const {
  auto s = name();
  if (s == "." || s == "..") {
    return s;
  }
  StringPiece sp(s);
  auto p = sp.rfind('.');
  if (p != StringPiece::npos && p != 0 && p+1 != sp.size()) {
    sp = sp.subpiece(0, p);
  }
  return sp.str();
}

std::string Path::ext() const {
  auto s = name();
  if (s == "." || s == "..") {
    return "";
  }
  StringPiece sp(s);
  auto p = sp.rfind('.');
  if (p != StringPiece::npos && p != 0 && p+1 != sp.size()) {
    sp = sp.subpiece(p);
  } else {
    return "";
  }
  return sp.str();
}

Path Path::replaceExt(const std::string& ext) const {
  Path p = parent();
  auto b = base();
  return p / (b + ext);
}

void Path::append(StringPiece sp) {
  if (sp.empty()) {
    return;
  }

  std::deque<StringPiece> q1, q2;
  if (!sp.startsWith('/')) {  // retain path_
    splitTo<StringPiece>('/', path_, std::back_inserter(q1), true);
  }
  splitTo<StringPiece>('/', sp, std::back_inserter(q2), true);

  while (!q1.empty() && q1.front() == ".") {
    q1.pop_front();
  }
  while (!q2.empty() && q2.front() == ".") {
    q2.pop_front();
  }

  while (!q2.empty()) {
    auto p = q2.front();
    if (p == "..") {
      if (q1.empty() || q1.back() == "..") {
        // q1 is formatted, so last item equal to ".." means all ".."
        q1.push_back(p);
      } else {
        q1.pop_back();
      }
    } else if (p != ".") {
      q1.push_back(p);  // skip "."
    }
    q2.pop_front();
  }

  bool abs = false;
  if (isAbsolute() || sp.startsWith('/')) {
    abs = true;
    while (!q1.empty() && q1.front() == "..") {
      // absolute path should not with ".." heads
      q1.pop_front();
    }
  }

  if (abs) {
    path_ = "/" + join('/', q1);
  } else {
    if (q1.empty()) {
      path_ = ".";
    } else {
      path_ = join('/', q1);
    }
  }
}

bool Path::checkExistOrMode(int mode) const {
  struct stat stat;
  if (::stat(path_.c_str(), &stat) == -1) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return false;
    }
    throwSystemError("stat(", path_, ") failed");
  }
  return mode == 0 || mode == (stat.st_mode & S_IFMT);
}

bool Path::accessible(int mode) const {
  if (::access(path_.c_str(), mode) == -1) {
    if (errno == ENOENT || errno == ENOTDIR || errno == EACCES) {
      return false;
    }
    throwSystemError("access(", path_, ") failed");
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const Path& path) {
  os << path.str();
  return os;
}

} // namespace rdd
