/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPException.h"

#include <sstream>

namespace rdd {

std::string HTTPException::describe() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const HTTPException& ex) {
  os << "what=\"" << ex.what()
     << "\", direction=" << static_cast<int>(ex.getDirection())
     << ", netError=" << getNetErrorString(ex.getNetError())
     << ", statusCode=" << ex.getStatusCode();
  return os;
}

} // namespace rdd
