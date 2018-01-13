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

#include "raster/protocol/http/Util.h"
#include "raster/util/MapUtil.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(urlConcat, all) {
  std::string url;

  url = urlConcat("https://localhost/path", {{"y", "y"}, {"z", "z"}});
  EXPECT_STREQ("https://localhost/path?y=y&z=z", url.c_str());
  url = urlConcat("https://localhost/path", {{"y", "/y"}, {"z", "z"}});
  EXPECT_STREQ("https://localhost/path?y=%2Fy&z=z", url.c_str());
  url = urlConcat("https://localhost/path?", {{"y", "y"}, {"z", "z"}});
  EXPECT_STREQ("https://localhost/path?y=y&z=z", url.c_str());
  url = urlConcat("https://localhost/path?x", {{"y", "y"}, {"z", "z"}});
  EXPECT_STREQ("https://localhost/path?x&y=y&z=z", url.c_str());
  url = urlConcat("https://localhost/path?x&", {{"y", "y"}, {"z", "z"}});
  EXPECT_STREQ("https://localhost/path?x&y=y&z=z", url.c_str());
  url = urlConcat("https://localhost/path?a=1&b=2", {{"y", "y"}, {"z", "z"}});
  EXPECT_STREQ("https://localhost/path?a=1&b=2&y=y&z=z", url.c_str());
  url = urlConcat("https://localhost/path?r=1&t=2", {});
  EXPECT_STREQ("https://localhost/path?r=1&t=2", url.c_str());
}

/*
TEST(MultipartFormData, file_upload) {
  URLQuery args;
  std::multimap<std::string, HTTPFile> files;
  parseMultipartFormData(
      "1234",
      "--1234\r\n"
      "Content-Disposition: form-data; name=\"files\"; filename=\"ab.txt\"\r\n"
      "\r\n"
      "Foo\r\n"
      "--1234--",
      args,
      files);
  auto v = get_all<std::vector<HTTPFile>>(files, "files");
  EXPECT_STREQ("ab.txt", v[0].filename.c_str());
  EXPECT_STREQ("Foo", v[0].body.c_str());
}

TEST(MultipartFormData, unquoted_names) {
  URLQuery args;
  std::multimap<std::string, HTTPFile> files;
  parseMultipartFormData(
      "1234",
      "--1234\r\n"
      "Content-Disposition: form-data; name=files; filename=ab.txt\r\n"
      "\r\n"
      "Foo\r\n"
      "--1234--",
      args,
      files);
  auto v = get_all<std::vector<HTTPFile>>(files, "files");
  EXPECT_STREQ("ab.txt", v[0].filename.c_str());
  EXPECT_STREQ("Foo", v[0].body.c_str());
}

TEST(MultipartFormData, missing_headers) {
  URLQuery args;
  std::multimap<std::string, HTTPFile> files;
  parseMultipartFormData(
      "1234",
      "--1234\r\n"
      "\r\n"
      "Foo\r\n"
      "--1234--",
      args,
      files);
  EXPECT_TRUE(files.empty());
}

TEST(MultipartFormData, invalid_content_disposition) {
  URLQuery args;
  std::multimap<std::string, HTTPFile> files;
  parseMultipartFormData(
      "1234",
      "--1234\r\n"
      "Content-Disposition: invalid; name=\"files\"; filename=\"ab.txt\"\r\n"
      "\r\n"
      "Foo\r\n"
      "--1234--",
      args,
      files);
  EXPECT_TRUE(files.empty());
}

TEST(MultipartFormData, does_not_end_with_correct_line_break) {
  URLQuery args;
  std::multimap<std::string, HTTPFile> files;
  parseMultipartFormData(
      "1234",
      "--1234\r\n"
      "Content-Disposition: form-data; name=\"files\"; filename=\"ab.txt\"\r\n"
      "\r\n"
      "Foo--1234--",
      args,
      files);
  EXPECT_TRUE(files.empty());
}

TEST(MultipartFormData, disposition_header_without_name_parameter) {
  URLQuery args;
  std::multimap<std::string, HTTPFile> files;
  parseMultipartFormData(
      "1234",
      "--1234\r\n"
      "Content-Disposition: form-data; filename=\"ab.txt\"\r\n"
      "\r\n"
      "Foo\r\n"
      "--1234--",
      args,
      files);
  EXPECT_TRUE(files.empty());
}

TEST(MultipartFormData, data_after_final_boundary) {
  URLQuery args;
  std::multimap<std::string, HTTPFile> files;
  parseMultipartFormData(
      "1234",
      "--1234\r\n"
      "Content-Disposition: form-data; name=\"files\"; filename=\"ab.txt\"\r\n"
      "\r\n"
      "Foo\r\n"
      "--1234--\r\n",
      args,
      files);
  auto v = get_all<std::vector<HTTPFile>>(files, "files");
  EXPECT_STREQ("ab.txt", v[0].filename.c_str());
  EXPECT_STREQ("Foo", v[0].body.c_str());
}
*/
