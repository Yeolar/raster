/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/enc/CharProperty.h"

using namespace rdd;

static const char* VERSION = "1.0.0";

DEFINE_string(def, "char.def", "char property definition path");
DEFINE_string(output, "char.prop", "output path");
DEFINE_string(encoding, "ucs2", "encoding (ucs2 or gbk)");

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./CharPropertyBuilder");
  google::ParseCommandLineFlags(&argc, &argv, true);

  CharProperty::compile(FLAGS_def, FLAGS_output, FLAGS_encoding);

  google::ShutDownCommandLineFlags();
  return 0;
}
