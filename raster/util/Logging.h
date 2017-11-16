/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string.h>

#include "raster/util/LogBase.h"
#include "raster/util/Singleton.h"
#include "raster/util/String.h"

namespace rdd {
namespace logging {

class RDDLogger : public BaseLogger {
public:
  RDDLogger() : BaseLogger("rdd") {}
};

} // namespace logging
} // namespace rdd

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

#define RDDLOG_ON(severity) \
  if (::rdd::Singleton< ::rdd::logging::RDDLogger>::get()->level() \
    <= ::rdd::logging::LOG_##severity)

#define RDDLOG_IF(severity, condition) \
  (!(condition)) ? (void)0 : RDDLOG(severity)
#define RDDPLOG_IF(severity, condition) \
  (!(condition)) ? (void)0 : RDDPLOG(severity)

#define RDDCHECK(condition) \
  (condition) ? (void)0 : RDDLOG(FATAL) << "Check " #condition " failed. "
#define RDDCHECK_LT(a, b) RDDCHECK((a) <  (b))
#define RDDCHECK_LE(a, b) RDDCHECK((a) <= (b))
#define RDDCHECK_GT(a, b) RDDCHECK((a) >  (b))
#define RDDCHECK_GE(a, b) RDDCHECK((a) >= (b))
#define RDDCHECK_EQ(a, b) RDDCHECK((a) == (b))
#define RDDCHECK_NE(a, b) RDDCHECK((a) != (b))

#define RDDPCHECK(condition) \
  RDDCHECK(condition) << ::rdd::errnoStr(errno) << ", "

#ifndef NDEBUG
#define DCHECK(condition) RDDCHECK(condition)
#define DCHECK_LT(a, b)   RDDCHECK_LT(a, b)
#define DCHECK_LE(a, b)   RDDCHECK_LE(a, b)
#define DCHECK_GT(a, b)   RDDCHECK_GT(a, b)
#define DCHECK_GE(a, b)   RDDCHECK_GE(a, b)
#define DCHECK_EQ(a, b)   RDDCHECK_EQ(a, b)
#define DCHECK_NE(a, b)   RDDCHECK_NE(a, b)
#else
#define DCHECK(condition) while (false) RDDCHECK(condition)
#define DCHECK_LT(a, b)   while (false) RDDCHECK_LT(a, b)
#define DCHECK_LE(a, b)   while (false) RDDCHECK_LE(a, b)
#define DCHECK_GT(a, b)   while (false) RDDCHECK_GT(a, b)
#define DCHECK_GE(a, b)   while (false) RDDCHECK_GE(a, b)
#define DCHECK_EQ(a, b)   while (false) RDDCHECK_EQ(a, b)
#define DCHECK_NE(a, b)   while (false) RDDCHECK_NE(a, b)
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

} // namespace rdd
