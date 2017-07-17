/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/io/event/Event.h"
#include "rddoc/net/Processor.h"
#include "rddoc/protocol/binary/AsyncServer.h"
#include "rddoc/protocol/binary/Encoding.h"

namespace rdd {

template <class Req = std::vector<uint8_t>,
          class Res = std::vector<uint8_t>>
class BinaryProcessor : public Processor {
public:
  BinaryProcessor() {}
  virtual ~BinaryProcessor() {}

  virtual bool process(Res& response, const Req& request) = 0;

  virtual bool decodeData(Event* event) {
    return binary::decodeData(event->rbuf(), &ibuf_);
  }

  virtual bool encodeData(Event* event) {
    return binary::encodeData(event->wbuf(), &obuf_);
  }

  virtual bool run() {
    try {
      return process(obuf_, ibuf_);
    } catch (std::exception& e) {
      RDDLOG(WARN) << "catch exception: " << e.what();
    } catch (...) {
      RDDLOG(WARN) << "catch unknown exception";
    }
    return false;
  }

private:
  Req ibuf_;
  Res obuf_;
};

template <class P>
class BinaryProcessorFactory : public ProcessorFactory {
public:
  BinaryProcessorFactory() {}
  virtual ~BinaryProcessorFactory() {}

  virtual std::shared_ptr<Processor> create() {
    return std::shared_ptr<Processor>(new P());
  }
};

}

