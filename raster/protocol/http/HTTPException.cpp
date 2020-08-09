/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/protocol/http/HTTPException.h"

#include <sstream>

namespace raster {

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

} // namespace raster
