/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <map>
#include <string>

#include "raster/protocol/http/HTTPHeaders.h"
#include "raster/protocol/http/HTTPMethod.h"
#include "raster/util/Range.h"

namespace rdd {

namespace RFC2616 {

/**
 * The HTTP request as defined in RFC 2616 may or may not have a body. In some
 * cases they MUST NOT have a body. In other cases, the body has no semantic
 * meaning and so is not defined. Finally, for some methods, the body is well
 * defined. Please see Section 9 and 4.3 for details on this.
 */
enum class BodyAllowed {
  DEFINED,
  NOT_DEFINED,
  NOT_ALLOWED,
};
BodyAllowed isRequestBodyAllowed(HTTPMethod method);

/**
 * Some status codes imply that there MUST NOT be a response body.  See section
 * 4.3: "All 1xx (informational), 204 (no content), and 304 (not modified)
 * responses MUST NOT include a message-body."
 */
bool responseBodyMustBeEmpty(unsigned status);

/**
 * Returns true if the headers imply that a body will follow. Note that in some
 * situations a body may come even if this function returns false (e.g. a 1.0
 * response body's length can be given implicitly by closing the connection).
 */
bool bodyImplied(const HTTPHeaders& headers);

/**
 * Parse a string containing tokens and qvalues, such as the RFC strings for
 * Accept-Charset, Accept-Encoding and Accept-Language.  It won't work for
 * complex Accept: headers because it doesn't return parameters or
 * accept-extension.
 *
 * See RFC sections 14.2, 14.3, 14.4 for definitions of these header values
 *
 * TODO: optionally sort by qvalue descending
 *
 * Return true if the string was well formed according to the RFC.  Note it can
 * return false but still populate output with best-effort parsing.
 */
typedef std::pair<StringPiece, double> TokenQPair;

bool parseQvalues(StringPiece value, std::vector<TokenQPair> &output);

} // namespace RFC2616

/**
 * Join a base URL and a possibly relative URL to form an absolute
 * interpretation of the latter.
 */
std::string urlJoin(const std::string& base, const std::string& url);

/**
 * Concatenate url and query regardless of whether url has existing
 * query parameters.
 */
std::string urlConcat(const std::string& url,
                      const std::map<std::string, std::string>& params);

#if 0
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
#endif

} // namespace rdd
