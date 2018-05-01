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

#include <string>
#include <vector>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

namespace rdd {
namespace protobuf {

namespace compiler {
typedef ::google::protobuf::compiler::CodeGenerator CodeGenerator;
typedef ::google::protobuf::compiler::GeneratorContext GeneratorContext;
static inline int PluginMain(int argc, char* argv[],
                             const CodeGenerator* generator) {
  return ::google::protobuf::compiler::PluginMain(argc, argv, generator);
}
static inline void ParseGeneratorParameter(
    const std::string& parameter,
    std::vector<std::pair<std::string, std::string>>* options) {
  ::google::protobuf::compiler::ParseGeneratorParameter(parameter, options);
}
} // namespace compiler

namespace io {
typedef ::google::protobuf::io::Printer Printer;
typedef ::google::protobuf::io::CodedOutputStream CodedOutputStream;
typedef ::google::protobuf::io::StringOutputStream StringOutputStream;
} // namespace io

} // namespace protobuf
} // namespace rdd

namespace rpc_generator {

static const char* const kCppGeneratorMessageHeaderExt = ".pb.h";
static const char* const kCppGeneratorServiceHeaderExt = ".rpc.pb.h";

} // namespace rpc_generator
