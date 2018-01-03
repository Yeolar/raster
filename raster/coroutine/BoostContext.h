/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <boost/version.hpp>
#if BOOST_VERSION >= 106100
#include <boost/context/detail/fcontext.hpp>
#else
#include <boost/context/fcontext.hpp>
#endif

#include "raster/util/Function.h"

/**
 * Wrappers for different versions of boost::context library
 * API reference for different versions
 * Boost 1.51:
 * http://www.boost.org/doc/libs/1_51_0/libs/context/doc/html/context/context/boost_fcontext.html
 * Boost 1.52:
 * http://www.boost.org/doc/libs/1_52_0/libs/context/doc/html/context/context/boost_fcontext.html
 * Boost 1.56:
 * http://www.boost.org/doc/libs/1_56_0/libs/context/doc/html/context/context/boost_fcontext.html
 * Boost 1.61:
 * https://github.com/boostorg/context/blob/boost-1.61.0/include/boost/context/detail/fcontext.hpp
 */

namespace rdd {

class Context {
#if BOOST_VERSION >= 106100
  using FiberContext = boost::context::detail::fcontext_t;
#elif BOOST_VERSION >= 105600
  using FiberContext = boost::context::fcontext_t;
#elif BOOST_VERSION >= 105200
  using FiberContext = boost::context::fcontext_t*;
#else
  using FiberContext = boost::ctx::fcontext_t;
#endif

#if BOOST_VERSION >= 106100
  using MainContext = boost::context::detail::fcontext_t;
#elif BOOST_VERSION >= 105600
  using MainContext = boost::context::fcontext_t;
#elif BOOST_VERSION >= 105200
  using MainContext = boost::context::fcontext_t;
#else
  using MainContext = boost::ctx::fcontext_t;
#endif

 public:
  Context(VoidFunc&& func,
          unsigned char* stackLimit,
          size_t stackSize)
    : func_(std::move(func)) {
    auto stackBase = stackLimit + stackSize;
#if BOOST_VERSION >= 106100
    fiberContext_ =
        boost::context::detail::make_fcontext(stackBase, stackSize, &fiberFunc);
#elif BOOST_VERSION >= 105200
    fiberContext_ =
        boost::context::make_fcontext(stackBase, stackSize, &fiberFunc);
#else
    fiberContext_.fc_stack.limit = stackLimit;
    fiberContext_.fc_stack.base = stackBase;
    make_fcontext(&fiberContext_, &fiberFunc);
#endif
  }

  void activate() {
#if BOOST_VERSION >= 106100
    auto transfer = boost::context::detail::jump_fcontext(fiberContext_, this);
    fiberContext_ = transfer.fctx;
    auto context = reinterpret_cast<intptr_t>(transfer.data);
#elif BOOST_VERSION >= 105200
    auto context = boost::context::jump_fcontext(
        &mainContext_, fiberContext_, reinterpret_cast<intptr_t>(this));
#else
    auto context = jump_fcontext(
        &mainContext_, &fiberContext_, reinterpret_cast<intptr_t>(this));
#endif
  }

  void deactivate() {
#if BOOST_VERSION >= 106100
    auto transfer = boost::context::detail::jump_fcontext(mainContext_, 0);
    mainContext_ = transfer.fctx;
    auto context = reinterpret_cast<intptr_t>(transfer.data);
#elif BOOST_VERSION >= 105600
    auto context =
        boost::context::jump_fcontext(&fiberContext_, mainContext_, 0);
#elif BOOST_VERSION >= 105200
    auto context =
        boost::context::jump_fcontext(fiberContext_, &mainContext_, 0);
#else
    auto context = jump_fcontext(&fiberContext_, &mainContext_, 0);
#endif
  }

 private:
#if BOOST_VERSION >= 106100
  static void fiberFunc(boost::context::detail::transfer_t transfer) {
    auto ctx = reinterpret_cast<Context*>(transfer.data);
    ctx->mainContext_ = transfer.fctx;
    ctx->func_();
  }
#else
  static void fiberFunc(intptr_t arg) {
    auto ctx = reinterpret_cast<Context*>(arg);
    ctx->func_();
  }
#endif

  VoidFunc func_;
  FiberContext fiberContext_;
  MainContext mainContext_;
};

} // namespace rdd
