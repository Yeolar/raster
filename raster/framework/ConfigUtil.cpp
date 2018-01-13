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

#include <iostream>
#include <gflags/gflags.h>

#include "raster/framework/Config.h"

DEFINE_bool(gen, false, "Generate default config");

using namespace rdd;

namespace {

void genDefault() {
  std::cout << generateDefault() << "\n";
}

}

int main(int argc, char* argv[]) {
  std::string usage =
    "Usage:\n"
    "  configutil\n"
    "    operate config\n"
    "  configutil -gen\n"
    "    generate default config\n";

  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_gen) {
    genDefault();
  } else {
    std::cerr << usage;
    exit(1);
  }

  google::ShutDownCommandLineFlags();

  return 0;
}
