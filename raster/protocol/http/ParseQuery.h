/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>

#include "raster/util/Range.h"

namespace rdd {

typedef std::multimap<std::string, std::string> URLQuery;

/**
 * Encode a query into a URL query string.
 */
std::string encodeQuery(const URLQuery& query);

/**
 * Parse a query given as a string argument.
 */
void parseQuery(URLQuery& query, StringPiece sp);

} // namespace rdd
