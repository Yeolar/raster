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

#include <memory>
#include <sstream>

#include "raster/protocol/proto/plugin/Config.h"
#include "raster/protocol/proto/plugin/RpcGenerator.h"
#include "raster/protocol/proto/plugin/GeneratorHelpers.h"

#include "src/compiler/protobuf_plugin.h"

class CpprpcGenerator : public rpc::protobuf::compiler::CodeGenerator {
 public:
  CpprpcGenerator() {}
  virtual ~CpprpcGenerator() {}

  virtual bool Generate(const rpc::protobuf::FileDescriptor* file,
                        const std::string& parameter,
                        rpc::protobuf::compiler::GeneratorContext* context,
                        std::string* error) const {
    if (file->options().cc_generic_services()) {
      *error =
          "cpp rpc proto compiler plugin does not work with generic "
          "services. To generate cpp rpc APIs, please set \""
          "cc_generic_service = false\".";
      return false;
    }

    rpc_generator::Parameters generator_parameters;
    generator_parameters.use_system_headers = true;
    generator_parameters.generate_mock_code = false;

    ProtoBufFile pbfile(file);

    if (!parameter.empty()) {
      std::vector<std::string> parameters_list =
          rpc_generator::tokenize(parameter, ",");
      for (auto parameter_string = parameters_list.begin();
           parameter_string != parameters_list.end(); parameter_string++) {
        std::vector<std::string> param =
            rpc_generator::tokenize(*parameter_string, "=");
        if (param[0] == "services_namespace") {
          generator_parameters.services_namespace = param[1];
        } else if (param[0] == "use_system_headers") {
          if (param[1] == "true") {
            generator_parameters.use_system_headers = true;
          } else if (param[1] == "false") {
            generator_parameters.use_system_headers = false;
          } else {
            *error = std::string("Invalid parameter: ") + *parameter_string;
            return false;
          }
        } else if (param[0] == "rpc_search_path") {
          generator_parameters.rpc_search_path = param[1];
        } else if (param[0] == "generate_mock_code") {
          if (param[1] == "true") {
            generator_parameters.generate_mock_code = true;
          } else if (param[1] != "false") {
            *error = std::string("Invalid parameter: ") + *parameter_string;
            return false;
          }
        } else if (param[0] == "gmock_search_path") {
          generator_parameters.gmock_search_path = param[1];
        } else if (param[0] == "additional_header_includes") {
          generator_parameters.additional_header_includes =
              rpc_generator::tokenize(param[1], ":");
        } else {
          *error = std::string("Unknown parameter: ") + *parameter_string;
          return false;
        }
      }
    }

    std::string file_name = rpc_generator::StripProto(file->name());

    std::string header_code =
        rpc_generator::GetHeaderPrologue(&pbfile, generator_parameters) +
        rpc_generator::GetHeaderIncludes(&pbfile, generator_parameters) +
        rpc_generator::GetHeaderServices(&pbfile, generator_parameters) +
        rpc_generator::GetHeaderEpilogue(&pbfile, generator_parameters);
    std::unique_ptr<rpc::protobuf::io::ZeroCopyOutputStream> header_output(
        context->Open(file_name + ".rpc.pb.h"));
    rpc::protobuf::io::CodedOutputStream header_coded_out(header_output.get());
    header_coded_out.WriteRaw(header_code.data(), header_code.size());

    std::string source_code =
        rpc_generator::GetSourcePrologue(&pbfile, generator_parameters) +
        rpc_generator::GetSourceIncludes(&pbfile, generator_parameters) +
        rpc_generator::GetSourceServices(&pbfile, generator_parameters) +
        rpc_generator::GetSourceEpilogue(&pbfile, generator_parameters);
    std::unique_ptr<rpc::protobuf::io::ZeroCopyOutputStream> source_output(
        context->Open(file_name + ".rpc.pb.cc"));
    rpc::protobuf::io::CodedOutputStream source_coded_out(source_output.get());
    source_coded_out.WriteRaw(source_code.data(), source_code.size());

    if (!generator_parameters.generate_mock_code) {
      return true;
    }
    std::string mock_code =
        rpc_generator::GetMockPrologue(&pbfile, generator_parameters) +
        rpc_generator::GetMockIncludes(&pbfile, generator_parameters) +
        rpc_generator::GetMockServices(&pbfile, generator_parameters) +
        rpc_generator::GetMockEpilogue(&pbfile, generator_parameters);
    std::unique_ptr<rpc::protobuf::io::ZeroCopyOutputStream> mock_output(
        context->Open(file_name + "_mock.rpc.pb.h"));
    rpc::protobuf::io::CodedOutputStream mock_coded_out(mock_output.get());
    mock_coded_out.WriteRaw(mock_code.data(), mock_code.size());

    return true;
  }

 private:
  // Insert the given code into the given file at the given insertion point.
  void Insert(rpc::protobuf::compiler::GeneratorContext* context,
              const std::string& filename, const std::string& insertion_point,
              const std::string& code) const {
    std::unique_ptr<rpc::protobuf::io::ZeroCopyOutputStream> output(
        context->OpenForInsert(filename, insertion_point));
    rpc::protobuf::io::CodedOutputStream coded_out(output.get());
    coded_out.WriteRaw(code.data(), code.size());
  }
};

int main(int argc, char* argv[]) {
  CpprpcGenerator generator;
  return rpc::protobuf::compiler::PluginMain(argc, argv, &generator);
}
