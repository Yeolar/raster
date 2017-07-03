/*
 * Copyright (C) 2017, Yeolar
 */

#include <thread>
#include "rddoc/concurrency/CPUThreadPool.h"
#include "rddoc/concurrency/IOThreadPool.h"
#include <gtest/gtest.h>

using namespace rdd;

static VoidFunc burnMs(uint64_t ms) {
  return [ms]() { usleep(ms); };
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

class TestObserver : public ThreadPool::Observer {
 public:
  void threadStarted(Thread*) { threads_++; }
  void threadStopped(Thread*) { threads_--; }
  void threadPreviouslyStarted(Thread*) {
    threads_++;
  }
  void threadNotYetStopped(Thread*) {
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

