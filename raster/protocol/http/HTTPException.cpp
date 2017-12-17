/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/Util.h"

namespace rdd {

std::ostream& operator<<(std::ostream& os, const HTTPException& ex) {
  os << "HTTP " << ex.getCode() << ": " << getResponseW3CName(ex.getCode());
  if (!ex.emptyMessage()) {
    os << " (" << ex.what() << ")";
  }
  return os;
}

} // namespace rdd
