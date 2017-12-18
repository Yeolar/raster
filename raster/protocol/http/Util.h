/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>

#include "raster/protocol/http/HTTPHeaders.h"
#include "raster/protocol/http/ParseQuery.h"
#include "raster/util/Range.h"

namespace rdd {

bool isValidResponseCode(int code);

const std::string& getResponseW3CName(int code);

/**
 * Join a base URL and a possibly relative URL to form an absolute
 * interpretation of the latter.
 */
std::string urlJoin(const std::string& base, const std::string& url);

/**
 * Concatenate url and query regardless of whether url has existing
 * query parameters.
 */
std::string urlConcat(const std::string& url, const URLQuery& query);

/**
 * Represents an HTTP file.
 *
 * The contentType comes from the provided HTTP header and should not be
 * trusted outright given that it can be easily forged.
 */
struct HTTPFile {
  std::string filename;
  std::string body;
  std::string contentType;
};

/**
 * Parses a form request body.
 *
 * Supports "application/x-www-form-urlencoded" and "multipart/form-data".
 * The arguments and files parameters will be updated with the parsed contents.
 */
void parseBodyArguments(
    StringPiece contentType,
    StringPiece body,
    URLQuery& arguments,
    std::multimap<std::string, HTTPFile>& files);

/**
 * Parses a multipart/form-data body.
 *
 * The arguments and files parameters will be updated with the contents
 * of the body.
 */
void parseMultipartFormData(
    StringPiece boundary,
    StringPiece data,
    URLQuery& arguments,
    std::multimap<std::string, HTTPFile>& files);

} // namespace rdd
