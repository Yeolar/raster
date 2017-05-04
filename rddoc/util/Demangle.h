/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <typeinfo>

namespace rdd {

std::string demangle(const char* name);
inline std::string demangle(const std::type_info& type) {
  return demangle(type.name());
}

}
