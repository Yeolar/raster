/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "rddoc/net/NetUtil.h"
#include "rddoc/protocol/thrift/SyncClient.h"
#include "rddoc/util/Algorithm.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/Thread.h"
#include "rddoc/util/ThreadPool.h"
#include "rddoc/util/Time.h"
#include "gen-cpp/Empty.h"

static const char* VERSION = "1.0.0";

DEFINE_string(addr, "127.0.0.1:8000", "HOST:PORT");
DEFINE_string(forward, "", "HOST:PORT");
DEFINE_int32(threads, 8, "concurrent threads");
DEFINE_int32(count, 100, "request count");

namespace rdd {
namespace empty {

class BenchTask : public ThreadTask {
public:
  BenchTask(const ClientOption& opt) : ThreadTask(), opt_(opt) {}
  virtual ~BenchTask() {}

  virtual void run() {
    uint64_t t = timestampNow();
    success_ = request();
    timecost_ = timePassed(t);
  }

  bool request() {
    Query req;
    req.__set_traceid("rddt");
    req.__set_query("query");
    req.__set_forward(FLAGS_forward);
    Result res;

    TSyncClient<EmptyClient> client(opt_);
    if (!client.connect()) {
      return false;
    }
    try {
      client.fetch(&EmptyClient::run, res, req);
      if (res.code != 0) {
        return false;
      }
    }
    catch (...) {
      return false;
    }
    return true;
  }

private:
  ClientOption opt_;
};

class BenchThread : public ThreadBase {
public:
  typedef ThreadPoolBase<BenchThread> Pool;

  BenchThread(Pool* pool) : pool_(pool) {}
  virtual ~BenchThread() {}

  virtual void run() {
    BenchTask* task = nullptr;
    while ((task = (BenchTask*)pool_->getTask())) {
      task->run();
      pool_->finishTask(task);
      delete task;
    }
  }

private:
  Pool* pool_;
};

}
}

using namespace rdd;

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./empty-bench");
  google::ParseCommandLineFlags(&argc, &argv, true);

  empty::BenchThread::Pool pool(FLAGS_threads);
  ClientOption opt;
  opt.peer = Peer(FLAGS_addr);
  opt.timeout.ctimeout = 10000000;
  opt.timeout.rtimeout = 10000000;
  opt.timeout.wtimeout = 10000000;

  for (int i = 0; i < FLAGS_count; i++) {
    pool.addTask(new empty::BenchTask(opt));
  }

  pool.start();
  while (pool.waitingCount() > 0) {
    sleep(1);
    RDDRLOG(INFO) << "handled: " << pool.successCount() + pool.failCount();
  }
  pool.finish();

  RDDRLOG(INFO) << "FINISH";
  RDDRLOG(INFO) << "success: " << pool.successCount();
  RDDRLOG(INFO) << "   fail: " << pool.failCount();

  if (pool.successCount() > 0) {
    auto costs = pool.taskCosts().values();
    std::sort(costs.begin(), costs.end());
    uint64_t cost10 = costs[pool.successCount()    /10];
    uint64_t cost50 = costs[pool.successCount() * 5/10];
    uint64_t cost90 = costs[pool.successCount() * 9/10];
    uint64_t cost_sum = sum(costs);
    uint64_t cost_avg = cost_sum / pool.successCount();

    RDDRLOG(INFO) << " cost10: " << cost10   / 1000.0 << " ms";
    RDDRLOG(INFO) << " cost50: " << cost50   / 1000.0 << " ms";
    RDDRLOG(INFO) << " cost90: " << cost90   / 1000.0 << " ms";
    RDDRLOG(INFO) << "avgcost: " << cost_avg / 1000.0 << " ms";
    RDDRLOG(INFO) << "    qps: " << 1000000. / cost_avg * FLAGS_threads;
  }

  /*
   * Intel(R) Xeon(R) CPU E3-1225 V2 @ 3.20GHz
   * Linux yhost 3.16.0-4-amd64 #1 SMP Debian 3.16.7-ckt11-1+deb8u3 (2015-08-04) x86_64 GNU/Linux
   *
   * success: 1000000
   *    fail: 0
   *  cost10: 0.13 ms
   *  cost50: 0.184 ms
   *  cost90: 0.273 ms
   * avgcost: 0.203 ms
   *     qps: 39408.9
   */

  google::ShutDownCommandLineFlags();
  return 0;
}
