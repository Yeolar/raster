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

#include <gflags/gflags.h>

#include "raster/enc/CharProperty.h"
#include "raster/util/Portability.h"

using namespace rdd;

static const char* VERSION = "1.0.0";

DEFINE_string(def, "char.def", "char property definition path");
DEFINE_string(output, "char.prop", "output path");
DEFINE_string(encoding, "ucs2", "encoding (ucs2 or gbk)");

int main(int argc, char* argv[]) {
  gflags::SetVersionString(VERSION);
  gflags::SetUsageMessage("Usage : ./CharPropertyBuilder");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  CharProperty::compile(FLAGS_def, FLAGS_output, FLAGS_encoding);

  gflags::ShutDownCommandLineFlags();
  return 0;
}
