/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "accelerator/concurrency/CPUThreadPoolExecutor.h"
#include "raster/net/NetUtil.h"
#include "raster/protocol/thrift/SyncClient.h"
#include "accelerator/Algorithm.h"
#include "accelerator/Logging.h"
#include "accelerator/Portability.h"
#include "gen-cpp/Empty.h"

static const char* VERSION = "1.1.0";

DEFINE_string(addr, "127.0.0.1:8000", "HOST:PORT");
DEFINE_int32(threads, 8, "concurrent threads");
DEFINE_int32(count, 100, "request count");

using namespace acc;
using namespace rdd;
using namespace rdd::empty;

bool request(const ClientOption& opt) {
  Query req;
  req.__set_traceid("rddt");
  req.__set_query("query");
  Result res;

  TSyncClient<EmptyClient> client(opt);
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

int main(int argc, char* argv[]) {
  gflags::SetVersionString(VERSION);
  gflags::SetUsageMessage("Usage : ./empty-bench");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  CPUThreadPoolExecutor pool(FLAGS_threads);
  std::atomic<size_t> count(0);
  std::vector<uint64_t> costs(FLAGS_count);

  pool.subscribeToTaskStats(
      [&](ThreadPoolExecutor::TaskStats stats) {
        costs[count++] = stats.runTime;
      });

  ClientOption opt;
  opt.peer.setFromIpPort(FLAGS_addr);
  opt.timeout.ctimeout = 10000000;
  opt.timeout.rtimeout = 10000000;
  opt.timeout.wtimeout = 10000000;

  for (int i = 0; i < FLAGS_count; i++) {
    pool.add(std::bind(request, opt));
  }

  while (pool.getPoolStats().pendingTaskCount > 0) {
    sleep(1);
    ACCRLOG(INFO) << "handled: " << count;
  }
  pool.join();

  ACCRLOG(INFO) << "FINISH";
  ACCRLOG(INFO) << "total: " << count;

  if (count > 0) {
    std::sort(costs.begin(), costs.end());
    uint64_t cost10 = costs[count    /10];
    uint64_t cost50 = costs[count * 5/10];
    uint64_t cost90 = costs[count * 9/10];
    uint64_t cost_sum = sum(costs);
    uint64_t cost_avg = cost_sum / count;

    ACCRLOG(INFO) << " cost10: " << cost10   / 1000.0 << " ms";
    ACCRLOG(INFO) << " cost50: " << cost50   / 1000.0 << " ms";
    ACCRLOG(INFO) << " cost90: " << cost90   / 1000.0 << " ms";
    ACCRLOG(INFO) << "avgcost: " << cost_avg / 1000.0 << " ms";
    ACCRLOG(INFO) << "    qps: " << 1000000. / cost_avg * FLAGS_threads;
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

  gflags::ShutDownCommandLineFlags();
  return 0;
}
