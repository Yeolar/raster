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

#pragma once

#include <iostream>

namespace rdd {

class Waker {
 public:
  Waker();

  ~Waker() {
    close();
  }

  int fd() const {
    return pipeFds_[0];
  }

  int fd2() const {
    return pipeFds_[1];
  }

  void wake() const;

  void consume() const;

 private:
  void close();

  int pipeFds_[2];
};

std::ostream& operator<<(std::ostream& os, const Waker& waker);

} // namespace rdd
