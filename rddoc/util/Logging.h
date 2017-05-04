/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string.h>
#include "rddoc/util/LogBase.h"
#include "rddoc/util/Singleton.h"
#include "rddoc/util/String.h"

namespace rdd {
namespace logging {

class RDDLogger : public BaseLogger {
public:
  RDDLogger() : BaseLogger("rdd") {}
};

}
}

// printf interface macro helper; do not use directly

#ifndef RDDLOG_STREAM
#define RDDLOG_STREAM(severity)                                             \
  (::rdd::Singleton< ::rdd::logging::RDDLogger>::get()->level() > severity) \
    ? (void)0                                                               \
    : ::rdd::logging::LogMessageVoidify() &                                 \
      ::rdd::logging::LogMessage(                                           \
        ::rdd::Singleton< ::rdd::logging::RDDLogger>::get(),                \
        severity, __FILENAME__, __LINE__).stream()
#endif

#ifndef RDDLOG_STREAM_TRACE
#define RDDLOG_STREAM_TRACE(severity, traceid)                              \
  (::rdd::Singleton< ::rdd::logging::RDDLogger>::get()->level() > severity) \
    ? (void)0                                                               \
    : ::rdd::logging::LogMessageVoidify() &                                 \
      ::rdd::logging::LogMessage(                                           \
        ::rdd::Singleton< ::rdd::logging::RDDLogger>::get(),                \
        severity, __FILENAME__, __LINE__, traceid).stream()
#endif


#ifndef RDDLOG_STREAM_RAW
#define RDDLOG_STREAM_RAW(severity)                                         \
  (::rdd::Singleton< ::rdd::logging::RDDLogger>::get()->level() > severity) \
    ? (void)0                                                               \
    : ::rdd::logging::LogMessageVoidify() &                                 \
      ::rdd::logging::RawLogMessage(                                        \
        ::rdd::Singleton< ::rdd::logging::RDDLogger>::get()).stream()
#endif

///////////////////////////////////////////////////////////////////////////
// rdd framework
//
#define RDDLOG(severity) \
  RDDLOG_STREAM(::rdd::logging::LOG_##severity)
#define RDDRLOG(severity) \
  RDDLOG_STREAM_RAW(::rdd::logging::LOG_##severity)
#define RDDTLOG(severity, traceid) \
  RDDLOG_STREAM_TRACE(::rdd::logging::LOG_##severity, traceid)
#define RDDPLOG(severity) \
  RDDLOG(severity) << ::rdd::errnoStr(errno) << ", "

#define RDDLOG_IF(severity) \
  if (::rdd::Singleton< ::rdd::logging::RDDLogger>::get()->level() \
    <= ::rdd::logging::LOG_##severity)

#define RDDCHECK(condition) \
  (condition) ? (void)0 : RDDLOG(FATAL) << "Check " #condition " failed. "

#ifndef NDEBUG
#define DCHECK(condition) RDDCHECK(condition)
#define DCHECK_LT(a, b)   RDDCHECK((a) <  (b))
#define DCHECK_LE(a, b)   RDDCHECK((a) <= (b))
#define DCHECK_GT(a, b)   RDDCHECK((a) >  (b))
#define DCHECK_GE(a, b)   RDDCHECK((a) >= (b))
#define DCHECK_EQ(a, b)   RDDCHECK((a) == (b))
#define DCHECK_NE(a, b)   RDDCHECK((a) != (b))
#else
#define DCHECK(condition) while (false) RDDCHECK(condition)
#define DCHECK_LT(a, b)   while (false) RDDCHECK((a) <  (b))
#define DCHECK_LE(a, b)   while (false) RDDCHECK((a) <= (b))
#define DCHECK_GT(a, b)   while (false) RDDCHECK((a) >  (b))
#define DCHECK_GE(a, b)   while (false) RDDCHECK((a) >= (b))
#define DCHECK_EQ(a, b)   while (false) RDDCHECK((a) == (b))
#define DCHECK_NE(a, b)   while (false) RDDCHECK((a) != (b))
#endif

namespace rdd {

class CostLogger {
public:
  CostLogger(const char* name, const char* msg, uint64_t threshold = 0)
    : threshold_(threshold), name_(name), msg_(msg) {
    start_ = timestampNow();
  }
  ~CostLogger() {
    uint64_t cost = timePassed(start_);
    if (cost >= threshold_) {
      RDDLOG(INFO) << "cost[" << name_ << "]: " << cost << "us, " << msg_;
    }
  }

private:
  uint64_t start_;
  uint64_t threshold_;
  const char* name_;
  const char* msg_;
};

}

