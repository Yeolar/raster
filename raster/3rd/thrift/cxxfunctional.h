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

#ifndef _THRIFT_CXXFUNCTIONAL_H_
#define _THRIFT_CXXFUNCTIONAL_H_ 1

#include <functional>

namespace apache { namespace thrift { namespace stdcxx {
  using ::std::function;
  using ::std::bind;

  namespace placeholders {
    using ::std::placeholders::_1;
    using ::std::placeholders::_2;
    using ::std::placeholders::_3;
    using ::std::placeholders::_4;
    using ::std::placeholders::_5;
    using ::std::placeholders::_6;
  } // apache::thrift::stdcxx::placeholders
}}} // apache::thrift::stdcxx

// Alias for thrift c++ compatibility namespace
namespace tcxx = apache::thrift::stdcxx;

#endif // #ifndef _THRIFT_CXXFUNCTIONAL_H_
