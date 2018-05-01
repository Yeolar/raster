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

namespace rpc_generator {

// A common interface for objects having comments in the source.
// Return formatted comments to be inserted in generated code.
struct CommentHolder {
  virtual ~CommentHolder() {}
  virtual std::string GetLeadingComments(const std::string prefix) const = 0;
  virtual std::string GetTrailingComments(const std::string prefix) const = 0;
  virtual std::vector<std::string> GetAllComments() const = 0;
};

// An abstract interface representing a method.
struct Method : public CommentHolder {
  virtual ~Method() {}

  virtual std::string name() const = 0;

  virtual std::string input_type_name() const = 0;
  virtual std::string output_type_name() const = 0;

  virtual bool get_module_and_message_path_input(
      std::string* str, std::string generator_file_name,
      bool generate_in_pb2_rpc, std::string import_prefix) const = 0;
  virtual bool get_module_and_message_path_output(
      std::string* str, std::string generator_file_name,
      bool generate_in_pb2_rpc, std::string import_prefix) const = 0;

  virtual std::string get_input_type_name() const = 0;
  virtual std::string get_output_type_name() const = 0;
  virtual bool NoStreaming() const = 0;
  virtual bool ClientStreaming() const = 0;
  virtual bool ServerStreaming() const = 0;
  virtual bool BidiStreaming() const = 0;
};

// An abstract interface representing a service.
struct Service : public CommentHolder {
  virtual ~Service() {}

  virtual std::string name() const = 0;

  virtual int method_count() const = 0;
  virtual std::unique_ptr<const Method> method(int i) const = 0;
};

struct Printer {
  virtual ~Printer() {}

  virtual void Print(const std::map<std::string, std::string>& vars,
                     const char* template_string) = 0;
  virtual void Print(const char* string) = 0;
  virtual void PrintRaw(const char* string) = 0;
  virtual void Indent() = 0;
  virtual void Outdent() = 0;
};

// An interface that allows the source generated to be output using various
// libraries/idls/serializers.
struct File : public CommentHolder {
  virtual ~File() {}

  virtual std::string filename() const = 0;
  virtual std::string filename_without_ext() const = 0;
  virtual std::string package() const = 0;
  virtual std::vector<std::string> package_parts() const = 0;
  virtual std::string additional_headers() const = 0;

  virtual int service_count() const = 0;
  virtual std::unique_ptr<const Service> service(int i) const = 0;

  virtual std::unique_ptr<Printer> CreatePrinter(std::string* str) const = 0;
};

} // namespace rpc_generator
