/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/FalconSender.h"

#include <curl/curl.h>
#include "raster/net/NetUtil.h"
#include "raster/util/json.h"
#include "raster/util/Logging.h"

namespace rdd {

const char* FalconSender::URL = "http://127.0.0.1:1988/v1/push";

FalconSender::FalconSender(const std::string& url)
  : url_(url) {
  curl_global_init(CURL_GLOBAL_ALL);
}

FalconSender::~FalconSender() {
  curl_global_cleanup();
}

// ts=`date +%s`; curl -X POST -d "[{\"metric\": \"metric.demo\", \"endpoint\": \"qd-open-falcon-judge01.hd\", \"timestamp\": $ts,\"step\": 60,\"value\": 9,\"counterType\": \"GAUGE\",\"tags\": \"project=falcon,module=judge\"}]" http://127.0.0.1:1988/v1/push

bool FalconSender::send(const Monitor::MonMap& value) {
  if (value.empty()) {
    return false;
  }
  dynamic all = dynamic::array;
  std::string node = getNodeName();
  for (auto& kv : value) {
    dynamic j = dynamic::object;
    j["metric"] = kv.first;
    j["value"] = kv.second;
    j["endpoint"] = node;
    j["timestamp"] = (int)time(nullptr);
    j["step"] = 60;
    j["counterType"] = "GAUGE";   // GAUGE or COUNTER
    j["tags"] = "";
    all.push_back(j);
  }
  return post(toJson(all));
}

static size_t writeFn(char *ptr, size_t size, size_t nmemb, void *userdata) {
  return nmemb * size;
}

bool FalconSender::post(const std::string& data) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    RDDLOG(ERROR) << "curl_easy_init failed";
    return false;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFn);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    RDDLOG(ERROR) << "curl_easy_perform failed: " << curl_easy_strerror(res);
  }
  return res == CURLE_OK;
}

} // namespace rdd
