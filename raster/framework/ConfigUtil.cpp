/*
 * Copyright (C) 2017, Yeolar
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
