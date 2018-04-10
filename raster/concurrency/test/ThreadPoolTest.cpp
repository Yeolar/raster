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

#include <thread>
#include "raster/concurrency/CPUThreadPool.h"
#include "raster/concurrency/IOThreadPool.h"
#include <gtest/gtest.h>

using namespace rdd;

static acc::VoidFunc burnMs(uint64_t ms) {
  return [ms]() { usleep(ms * 1000); };
}

template <class Pool>
static void basic() {
  // Create and destroy
  Pool pool(10);
}

TEST(ThreadPoolTest, CPUBasic) {
  basic<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOBasic) {
  basic<IOThreadPool>();
}

template <class Pool>
static void resize() {
  Pool pool(100);
  EXPECT_EQ(100, pool.numThreads());
  pool.setNumThreads(50);
  EXPECT_EQ(50, pool.numThreads());
  pool.setNumThreads(150);
  EXPECT_EQ(150, pool.numThreads());
}

TEST(ThreadPoolTest, CPUResize) {
  resize<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOResize) {
  resize<IOThreadPool>();
}

template <class Pool>
static void stop() {
  Pool pool(1);
  std::atomic<int> completed(0);
  auto f = [&](){
    burnMs(10)();
    completed++;
  };
  for (int i = 0; i < 1000; i++) {
    pool.add(f);
  }
  pool.stop();
  EXPECT_GT(1000, completed);
}

/*
// IOThreadPool's stop() behaves like join().
template <>
void stop<IOThreadPool>() {
  IOThreadPool pool(1);
  std::atomic<int> completed(0);
  auto f = [&](){
    burnMs(10)();
    completed++;
  };
  for (int i = 0; i < 10; i++) {
    pool.add(f);
  }
  pool.stop();
  EXPECT_EQ(10, completed);
}
*/

TEST(ThreadPoolTest, CPUStop) {
  stop<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOStop) {
  stop<IOThreadPool>();
}

template <class Pool>
static void join() {
  Pool pool(10);
  std::atomic<int> completed(0);
  auto f = [&](){
    burnMs(1)();
    completed++;
  };
  for (int i = 0; i < 1000; i++) {
    pool.add(f);
  }
  pool.join();
  EXPECT_EQ(1000, completed);
}

TEST(ThreadPoolTest, CPUJoin) {
  join<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOJoin) {
  join<IOThreadPool>();
}

template <class Pool>
static void resizeUnderLoad() {
  Pool pool(10);
  std::atomic<int> completed(0);
  auto f = [&](){
    burnMs(1)();
    completed++;
  };
  for (int i = 0; i < 1000; i++) {
    pool.add(f);
  }
  pool.setNumThreads(5);
  pool.setNumThreads(15);
  pool.join();
  EXPECT_EQ(1000, completed);
}

TEST(ThreadPoolTest, CPUResizeUnderLoad) {
  resizeUnderLoad<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOResizeUnderLoad) {
  resizeUnderLoad<IOThreadPool>();
}

template <class Pool>
static void poolStats() {
  Baton startBaton, endBaton;
  Pool pool(1);
  auto stats = pool.getStats();
  EXPECT_EQ(1, stats.threadCount);
  EXPECT_EQ(1, stats.idleThreadCount);
  EXPECT_EQ(0, stats.activeThreadCount);
  EXPECT_EQ(0, stats.pendingTaskCount);
  EXPECT_EQ(0, stats.totalTaskCount);
  pool.add([&](){ startBaton.post(); endBaton.wait(); });
  pool.add([&](){});
  startBaton.wait();
  stats = pool.getStats();
  EXPECT_EQ(1, stats.threadCount);
  EXPECT_EQ(0, stats.idleThreadCount);
  EXPECT_EQ(1, stats.activeThreadCount);
  EXPECT_EQ(1, stats.pendingTaskCount);
  EXPECT_EQ(2, stats.totalTaskCount);
  endBaton.post();
}

TEST(ThreadPoolTest, CPUPoolStats) {
  poolStats<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOPoolStats) {
  poolStats<IOThreadPool>();
}

template <class Pool>
static void taskStats() {
  Pool pool(1);
  std::atomic<int> c(0);
  auto s = pool.subscribeToTaskStats(
      Observer<Task::Stats>::create(
          [&](Task::Stats stats) {
        int i = c++;
        EXPECT_LT(0, stats.runTime);
        if (i == 1) {
          EXPECT_LT(0, stats.waitTime);
        }
      }));
  pool.add(burnMs(10));
  pool.add(burnMs(10));
  pool.join();
  EXPECT_EQ(2, c);
}

TEST(ThreadPoolTest, CPUTaskStats) {
  taskStats<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOTaskStats) {
  taskStats<IOThreadPool>();
}

template <class Pool>
static void expiration() {
  Pool pool(1);
  std::atomic<int> statCbCount(0);
  auto s = pool.subscribeToTaskStats(
      Observer<Task::Stats>::create(
          [&](Task::Stats stats) {
        int i = statCbCount++;
        if (i == 0) {
          EXPECT_FALSE(stats.expired);
        } else if (i == 1) {
          EXPECT_TRUE(stats.expired);
        } else {
          FAIL();
        }
      }));
  std::atomic<int> expireCbCount(0);
  auto expireCb = [&] () { expireCbCount++; };
  pool.add(burnMs(10), 60000000, expireCb);
  pool.add(burnMs(10), 10000, expireCb);
  pool.join();
  EXPECT_EQ(2, statCbCount);
  EXPECT_EQ(1, expireCbCount);
}

TEST(ThreadPoolTest, CPUExpiration) {
  expiration<CPUThreadPool>();
}

TEST(ThreadPoolTest, IOExpiration) {
  expiration<IOThreadPool>();
}

class TestObserver : public ThreadPool::Observer {
public:
  void threadStarted(ThreadPool::ThreadHandle*) { threads_++; }
  void threadStopped(ThreadPool::ThreadHandle*) { threads_--; }
  void threadPreviouslyStarted(ThreadPool::ThreadHandle*) {
    threads_++;
  }
  void threadNotYetStopped(ThreadPool::ThreadHandle*) {
    threads_--;
  }
  void checkCalls() {
    ASSERT_EQ(threads_, 0);
  }
private:
  std::atomic<int> threads_{0};
};

TEST(ThreadPoolTest, CPUObserver) {
  auto observer = std::make_shared<TestObserver>();
  {
    CPUThreadPool pool(10);
    pool.addObserver(observer);
    pool.setNumThreads(3);
    pool.setNumThreads(0);
    pool.setNumThreads(7);
    pool.removeObserver(observer);
    pool.setNumThreads(10);
  }
  observer->checkCalls();
}

TEST(ThreadPoolTest, IOObserver) {
  auto observer = std::make_shared<TestObserver>();

  {
    IOThreadPool pool(10);
    pool.addObserver(observer);
    pool.setNumThreads(3);
    pool.setNumThreads(0);
    pool.setNumThreads(7);
    pool.removeObserver(observer);
    pool.setNumThreads(10);
  }

  observer->checkCalls();
}

