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

#include <vector>

#include "raster/protocol/proto/plugin/Config.h"
#include "raster/protocol/proto/plugin/GeneratorHelpers.h"
#include "raster/protocol/proto/plugin/SchemaInterface.h"

// Get leading or trailing comments in a string.
template <typename DescriptorType>
inline std::string GetCommentsHelper(const DescriptorType* desc, bool leading,
                                     const std::string& prefix) {
  return rpc_generator::GetPrefixedComments(desc, leading, prefix);
}

class ProtoBufMethod : public rpc_generator::Method {
 public:
  ProtoBufMethod(const ::google::protobuf::MethodDescriptor* method)
      : method_(method) {}

  std::string name() const { return method_->name(); }

  std::string input_type_name() const {
    return rpc_generator::ClassName(method_->input_type(), true);
  }
  std::string output_type_name() const {
    return rpc_generator::ClassName(method_->output_type(), true);
  }

  std::string get_input_type_name() const {
    return method_->input_type()->file()->name();
  }
  std::string get_output_type_name() const {
    return method_->output_type()->file()->name();
  }

  bool NoStreaming() const {
    return !method_->client_streaming() && !method_->server_streaming();
  }

  bool ClientStreaming() const { return method_->client_streaming(); }

  bool ServerStreaming() const { return method_->server_streaming(); }

  bool BidiStreaming() const {
    return method_->client_streaming() && method_->server_streaming();
  }

  std::string GetLeadingComments(const std::string prefix) const {
    return GetCommentsHelper(method_, true, prefix);
  }

  std::string GetTrailingComments(const std::string prefix) const {
    return GetCommentsHelper(method_, false, prefix);
  }

 private:
  const ::google::protobuf::MethodDescriptor* method_;
};

class ProtoBufService : public rpc_generator::Service {
 public:
  ProtoBufService(const ::google::protobuf::ServiceDescriptor* service)
      : service_(service) {}

  std::string name() const { return service_->name(); }

  int method_count() const { return service_->method_count(); };
  std::unique_ptr<const rpc_generator::Method> method(int i) const {
    return std::unique_ptr<const rpc_generator::Method>(
        new ProtoBufMethod(service_->method(i)));
  };

  std::string GetLeadingComments(const std::string prefix) const {
    return GetCommentsHelper(service_, true, prefix);
  }

  std::string GetTrailingComments(const std::string prefix) const {
    return GetCommentsHelper(service_, false, prefix);
  }

 private:
  const ::google::protobuf::ServiceDescriptor* service_;
};

class ProtoBufPrinter : public rpc_generator::Printer {
 public:
  ProtoBufPrinter(std::string* str)
      : output_stream_(str), printer_(&output_stream_, '$') {}

  void Print(const std::map<std::string, std::string>& vars,
             const char* string_template) {
    printer_.Print(vars, string_template);
  }

  void Print(const char* string) { printer_.Print(string); }
  void PrintRaw(const char* string) { printer_.PrintRaw(string); }
  void Indent() { printer_.Indent(); }
  void Outdent() { printer_.Outdent(); }

 private:
  ::google::protobuf::io::StringOutputStream output_stream_;
  ::google::protobuf::io::Printer printer_;
};

class ProtoBufFile : public rpc_generator::File {
 public:
  ProtoBufFile(const ::google::protobuf::FileDescriptor* file) : file_(file) {}

  std::string filename() const { return file_->name(); }
  std::string filename_without_ext() const {
    return rpc_generator::StripProto(filename());
  }

  std::string package() const { return file_->package(); }
  std::vector<std::string> package_parts() const {
    return rpc_generator::tokenize(package(), ".");
  }

  std::string additional_headers() const { return ""; }

  int service_count() const { return file_->service_count(); };
  std::unique_ptr<const rpc_generator::Service> service(int i) const {
    return std::unique_ptr<const rpc_generator::Service>(
        new ProtoBufService(file_->service(i)));
  }

  std::unique_ptr<rpc_generator::Printer> CreatePrinter(
      std::string* str) const {
    return std::unique_ptr<rpc_generator::Printer>(new ProtoBufPrinter(str));
  }

  std::string GetLeadingComments(const std::string prefix) const {
    return GetCommentsHelper(file_, true, prefix);
  }

  std::string GetTrailingComments(const std::string prefix) const {
    return GetCommentsHelper(file_, false, prefix);
  }

 private:
  const ::google::protobuf::FileDescriptor* file_;
};

