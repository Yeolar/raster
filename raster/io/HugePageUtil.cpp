/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include <iostream>
#include <stdexcept>
#include <gflags/gflags.h>

#include "raster/io/FSUtil.h"
#include "raster/io/HugePages.h"
#include "raster/util/MemoryMapping.h"

DEFINE_bool(cp, false, "Copy file");

using namespace rdd;

namespace {

void copy(const char* srcFile, const char* dest) {
  Path destPath(dest);
  if (!destPath.isAbsolute()) {
    auto hp = getHugePageSize();
    RDDCHECK(hp) << "no huge pages available";
    destPath = canonical(destPath.parent(), hp->mountPoint) / destPath.name();
  }

  mmapFileCopy(srcFile, destPath.c_str());
}

void list() {
  for (const auto& p : getHugePageSizes()) {
    std::cout << p.size << " " << p.mountPoint << "\n";
  }
}

} // namespace

int main(int argc, char *argv[]) {
  std::string usage =
    "Usage:\n"
    "  hugepageutil\n"
    "    list all huge page sizes and their mount points\n"
    "  hugepageutil -cp <src_file> <dest_nameprefix>\n"
    "    copy src_file to a huge page file\n";

  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_cp) {
    if (argc != 3) {
      std::cerr << usage;
      exit(1);
    }
    copy(argv[1], argv[2]);
  } else {
    if (argc != 1) {
      std::cerr << usage;
      exit(1);
    }
    list();
  }

  google::ShutDownCommandLineFlags();

  return 0;
}
