/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/protocol/http/HTTPEvent.h"

namespace rdd {

void HTTPEvent::onParserError(const char* what) {
  /*
  inRecvLastChunk_ = false;
  http_errno parser_errno = HTTP_PARSER_ERRNO(&parser_);
  HTTPException error(HTTPException::Direction::INGRESS,
                      what ? what : folly::to<std::string>(
                        "Error parsing message: ",
                        http_errno_description(parser_errno)
                      ));
  // generate a string of parsed headers so that we can pass it to callback
  if (msg_) {
    error.setPartialMsg(std::move(msg_));
  }
  // store the ingress buffer
  if (currentIngressBuf_) {
    error.setCurrentIngressBuf(std::move(currentIngressBuf_->clone()));
  }
  if (transportDirection_ == TransportDirection::DOWNSTREAM &&
      egressTxnID_ < ingressTxnID_) {
    error.setHttpStatusCode(400);
  } // else we've already egressed a response for this txn, don't attempt a 400
  // See http_parser.h for what these error codes mean
  if (parser_errno == HPE_INVALID_EOF_STATE) {
    error.setProxygenError(kErrorEOF);
  } else if (parser_errno == HPE_HEADER_OVERFLOW ||
             parser_errno == HPE_INVALID_CONSTANT ||
             (parser_errno >= HPE_INVALID_VERSION &&
              parser_errno <= HPE_HUGE_CONTENT_LENGTH)) {
    error.setProxygenError(kErrorParseHeader);
  } else if (parser_errno == HPE_INVALID_CHUNK_SIZE ||
             parser_errno == HPE_HUGE_CHUNK_SIZE) {
    error.setProxygenError(kErrorParseBody);
  } else {
    error.setProxygenError(kErrorUnknown);
  }
  callback_->onError(ingressTxnID_, error);
  */
}

int HTTPEvent::onMessageBegin() {
  return 0;
}

int HTTPEvent::onURL(const char* buf, size_t len) {
  return 0;
}

int HTTPEvent::onReason(const char* buf, size_t len) {
  return 0;
}

int HTTPEvent::onHeaderField(const char* buf, size_t len) {
  return 0;
}

int HTTPEvent::onHeaderValue(const char* buf, size_t len) {
  return 0;
}

int HTTPEvent::onHeadersComplete(size_t len) {
  return 0;
}

int HTTPEvent::onBody(const char* buf, size_t len) {
  return 0;
}

int HTTPEvent::onChunkHeader(size_t len) {
  return 0;
}

int HTTPEvent::onChunkComplete() {
  return 0;
}

int HTTPEvent::onMessageComplete() {
  return 0;
}

int HTTPEvent::onMessageBeginCB(http_parser* parser) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onMessageBegin();
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onUrlCB(http_parser* parser, const char* buf, size_t len) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onURL(buf, len);
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onReasonCB(http_parser* parser, const char* buf, size_t len) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onReason(buf, len);
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onHeaderFieldCB(http_parser* parser,
                               const char* buf, size_t len) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onHeaderField(buf, len);
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onHeaderValueCB(http_parser* parser,
                               const char* buf, size_t len) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onHeaderValue(buf, len);
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onHeadersCompleteCB(http_parser* parser,
                                   const char* buf, size_t len) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onHeadersComplete(len);
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 3;
  }
}

int HTTPEvent::onBodyCB(http_parser* parser, const char* buf, size_t len) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onBody(buf, len);
  } catch (const std::exception& ex) {
    // Note: http_parser appears to completely ignore the return value from the
    // on_body() callback.  There seems to be no way to abort parsing after an
    // error in on_body().
    //
    // We handle this by checking if error_ is set after each call to
    // http_parser_execute().
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onChunkHeaderCB(http_parser* parser) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onChunkHeader(parser->content_length);
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onChunkCompleteCB(http_parser* parser) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onChunkComplete();
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

int HTTPEvent::onMessageCompleteCB(http_parser* parser) {
  HTTPEvent* event = static_cast<HTTPEvent*>(parser->data);
  try {
    return event->onMessageComplete();
  } catch (const std::exception& ex) {
    event->onParserError(ex.what());
    return 1;
  }
}

void HTTPEvent::initParserSettings() {
  kParserSettings.on_message_begin = HTTPEvent::onMessageBeginCB;
  kParserSettings.on_url = HTTPEvent::onUrlCB;
  kParserSettings.on_header_field = HTTPEvent::onHeaderFieldCB;
  kParserSettings.on_header_value = HTTPEvent::onHeaderValueCB;
  kParserSettings.on_headers_complete = HTTPEvent::onHeadersCompleteCB;
  kParserSettings.on_body = HTTPEvent::onBodyCB;
  kParserSettings.on_message_complete = HTTPEvent::onMessageCompleteCB;
  kParserSettings.on_reason = HTTPEvent::onReasonCB;
  kParserSettings.on_chunk_header = HTTPEvent::onChunkHeaderCB;
  kParserSettings.on_chunk_complete = HTTPEvent::onChunkCompleteCB;
}

}

