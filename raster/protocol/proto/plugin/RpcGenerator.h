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

#include <memory>
#include <string>
#include <vector>

#include "raster/protocol/proto/plugin/Config.h"
#include "raster/protocol/proto/plugin/SchemaInterface.h"

namespace rpc_generator {

// Contains all the parameters that are parsed from the command line.
struct Parameters {
  // Puts the service into a namespace
  std::string services_namespace;
  // Use system includes (<>) or local includes ("")
  bool use_system_headers;
  // Prefix to any rpc include
  std::string rpc_search_path;
  // *EXPERIMENTAL* Additional include files in rpc.pb.h
  std::vector<std::string> additional_header_includes;
};

// Return the prologue of the generated header file.
std::string GetHeaderPrologue(rpc_generator::File* file,
                              const Parameters& params);

// Return the includes needed for generated header file.
std::string GetHeaderIncludes(rpc_generator::File* file,
                              const Parameters& params);

// Return the includes needed for generated source file.
std::string GetSourceIncludes(rpc_generator::File* file,
                              const Parameters& params);

// Return the epilogue of the generated header file.
std::string GetHeaderEpilogue(rpc_generator::File* file,
                              const Parameters& params);

// Return the prologue of the generated source file.
std::string GetSourcePrologue(rpc_generator::File* file,
                              const Parameters& params);

// Return the services for generated header file.
std::string GetHeaderServices(rpc_generator::File* file,
                              const Parameters& params);

// Return the services for generated source file.
std::string GetSourceServices(rpc_generator::File* file,
                              const Parameters& params);

// Return the epilogue of the generated source file.
std::string GetSourceEpilogue(rpc_generator::File* file,
                              const Parameters& params);

} // namespace rpc_generator
