/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <vector>
#include "rddoc/coroutine/FiberManager.h"
#include "rddoc/net/Actor.h"
#include "rddoc/parallel/DAG.h"
#include "rddoc/parallel/JobExecutor.h"
#include "rddoc/util/ReflectObject.h"

namespace rdd {

class ActorDAG : public DAG {
public:
  void execute(const ExecutorPtr& executor) {
    Singleton<Actor>::get()->execute(executor);
  }
};

struct Graph {
  typedef std::string Node;
  typedef std::vector<Node> Next;

  std::vector<std::pair<Node, Next>> graph;

  void add(const Node& node, const Next& next) {
    graph.emplace_back(node, next);
  }
};

class JobGraphManager {
public:
  JobGraphManager() {}

  Graph& getGraph(const std::string& key) {
    return graphs_[key];
  }

private:
  std::map<std::string, Graph> graphs_;
};

class JobScheduler {
public:
  JobScheduler(const Graph& graph,
               const JobExecutor::ContextPtr& ctx = nullptr) {
    init(graph, ctx);
  }
  JobScheduler(const std::string& key,
               const JobExecutor::ContextPtr& ctx = nullptr) {
    init(Singleton<JobGraphManager>::get()->getGraph(key), ctx);
  }

  void run() {
    if (!dag_.empty()) {
      ExecutorPtr executor = getCurrentExecutor();
      executor->addCallback(
          std::bind(&ActorDAG::schedule, &dag_, dag_.go(executor)));
      FiberManager::yield();
    }
  }

private:
  void init(const Graph& graph, const JobExecutor::ContextPtr& ctx) {
    std::map<Graph::Node, DAG::Key> m;
    for (auto& p : graph.graph) {
      auto clsname = p.first.substr(0, p.first.find(':'));
      auto executor = makeReflectObject<JobExecutor>(clsname);
      executor->setName(p.first);
      executor->setContext(ctx);
      m[p.first] = dag_.add(ExecutorPtr(executor));
    }
    for (auto& p : graph.graph) {
      for (auto& q : p.second) {
        dag_.dependency(m[p.first], m[q]);
      }
    }
  }

  ActorDAG dag_;
  std::shared_ptr<JobExecutor::Context> ctx_;
};

}

