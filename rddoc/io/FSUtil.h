/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <boost/filesystem.hpp>

namespace fs {  // shortcut

using namespace ::boost::filesystem;

inline path canonical_parent(const path& p,
                             const path& base = current_path()) {
  return canonical(p.parent_path(), base) / p.filename();
}

}

