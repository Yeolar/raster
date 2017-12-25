/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "raster/concurrency/CPUThreadPool.h"
#include "raster/net/NetUtil.h"
#include "raster/protocol/binary/SyncClient.h"
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "Helper.h"
#include "table_generated.h"

static const char* VERSION = "1.0.0";

DEFINE_string(addr, "127.0.0.1:8000", "HOST:PORT");
DEFINE_string(forward, "", "HOST:PORT");
DEFINE_int32(threads, 8, "concurrent threads");
DEFINE_int32(count, 100, "request count");

namespace rdd {
namespace fbs {

bool request(const ClientOption& opt) {
  ::flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(
      CreateQuery(fbb,
                  fbb.CreateString("rddt"),
                  fbb.CreateString("query"),
                  fbb.CreateString(FLAGS_forward)));

  BinarySyncClient client(opt);
  if (!client.connect()) {
    return false;
  }
  try {
    ByteRange req(fbb.GetBufferPointer(), fbb.GetSize());
    ByteRange data;
    client.fetch(data, req);
    auto res = ::flatbuffers::GetRoot<Result>(data.data());
    DCHECK(verifyFlatbuffer(res, data));
    if (res->code() != 0) {
      return false;
    }
  }
  catch (...) {
    return false;
  }
  return true;
}

}
}

using namespace rdd;

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./flatbuffers-bench");
  google::ParseCommandLineFlags(&argc, &argv, true);

  CPUThreadPool pool(FLAGS_threads);
  std::atomic<size_t> count(0);
  std::vector<uint64_t> costs(FLAGS_count);

  auto s = pool.subscribeToTaskStats(
      Observer<Task::Stats>::create(
          [&](Task::Stats stats) {
        costs[count++] = stats.runTime;
      }));

  ClientOption opt;
  opt.peer.setFromIpPort(FLAGS_addr);
  opt.timeout.ctimeout = 10000000;
  opt.timeout.rtimeout = 10000000;
  opt.timeout.wtimeout = 10000000;

  for (int i = 0; i < FLAGS_count; i++) {
    pool.add(std::bind(fbs::request, opt));
  }

  while (pool.getStats().pendingTaskCount > 0) {
    sleep(1);
    RDDRLOG(INFO) << "handled: " << count;
  }
  pool.join();

  RDDRLOG(INFO) << "FINISH";
  RDDRLOG(INFO) << "total: " << count;

  if (count > 0) {
    std::sort(costs.begin(), costs.end());
    uint64_t cost10 = costs[count    /10];
    uint64_t cost50 = costs[count * 5/10];
    uint64_t cost90 = costs[count * 9/10];
    uint64_t cost_sum = sum(costs);
    uint64_t cost_avg = cost_sum / count;

    RDDRLOG(INFO) << " cost10: " << cost10   / 1000.0 << " ms";
    RDDRLOG(INFO) << " cost50: " << cost50   / 1000.0 << " ms";
    RDDRLOG(INFO) << " cost90: " << cost90   / 1000.0 << " ms";
    RDDRLOG(INFO) << "avgcost: " << cost_avg / 1000.0 << " ms";
    RDDRLOG(INFO) << "    qps: " << 1000000. / cost_avg * FLAGS_threads;
  }

  google::ShutDownCommandLineFlags();
  return 0;
}
