/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/coroutine/GenericExecutor.h"

namespace rdd {

class JobExecutor : public GenericExecutor {
public:
  struct Context {
    virtual ~Context() {}
  };
  typedef std::shared_ptr<Context> ContextPtr;

  virtual ~JobExecutor() {}

  void setName(const std::string& name) {
    name_ = name;
  }

  void setContext(const ContextPtr& ctx) {
    context_ = ctx;
  }

protected:
  std::string name_;
  ContextPtr context_;
};

}

