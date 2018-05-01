/*
 * Copyright 2018 Yeolar
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

#include <map>

#include "raster/protocol/proto/plugin/Config.h"
#include "raster/protocol/proto/plugin/GeneratorHelpers.h"

namespace rpc_generator {

inline std::string DotsToColons(const std::string& name) {
  return rpc_generator::StringReplace(name, ".", "::");
}

inline std::string DotsToUnderscores(const std::string& name) {
  return rpc_generator::StringReplace(name, ".", "_");
}

inline std::string ClassName(const rpc::protobuf::Descriptor* descriptor,
                              bool qualified) {
  // Find "outer", the descriptor of the top-level message in which
  // "descriptor" is embedded.
  const rpc::protobuf::Descriptor* outer = descriptor;
  while (outer->containing_type() != NULL) outer = outer->containing_type();

  const std::string& outer_name = outer->full_name();
  std::string inner_name = descriptor->full_name().substr(outer_name.size());

  if (qualified) {
    return "::" + DotsToColons(outer_name) + DotsToUnderscores(inner_name);
  } else {
    return outer->name() + DotsToUnderscores(inner_name);
  }
}

// Get leading or trailing comments in a string. Comment lines start with "// ".
// Leading detached comments are put in in front of leading comments.
template <typename DescriptorType>
inline std::string GetCppComments(const DescriptorType* desc, bool leading) {
  return rpc_generator::GetPrefixedComments(desc, leading, "//");
}

}  // namespace rpc_generator
