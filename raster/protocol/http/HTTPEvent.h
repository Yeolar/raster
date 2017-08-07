/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/3rd/http_parser/http_parser.h"
#include "raster/net/Event.h"

namespace rdd {

class HTTPEvent : public Event {
public:
  HTTPEvent(const std::shared_ptr<Channel>& channel,
            const std::shared_ptr<Socket>& socket = std::shared_ptr<Socket>())
    : Event(channel, socket) {
    http_parser_init(&parser_, HTTP_REQUEST);
    parser_.data = this;
  }

  virtual ~HTTPEvent() {}

  size_t parse() {
    return http_parser_execute(&parser_,
                               &kParserSettings,
                               (const char*)rbuf()->data(),
                               rbuf()->length());
    parserError_ = (HTTP_PARSER_ERRNO(&parser_) != HPE_OK) &&
                   (HTTP_PARSER_ERRNO(&parser_) != HPE_PAUSED);
    if (parserError_) {
      onParserError();
    }
  }

  size_t bodyLength() const {
    return 0;
  }

  bool readingHeaders{true};

private:
  void onParserError(const char* what = nullptr);

  // Parser callbacks
  int onMessageBegin();
  int onURL(const char* buf, size_t len);
  int onReason(const char* buf, size_t len);
  int onHeaderField(const char* buf, size_t len);
  int onHeaderValue(const char* buf, size_t len);
  int onHeadersComplete(size_t len);
  int onBody(const char* buf, size_t len);
  int onChunkHeader(size_t len);
  int onChunkComplete();
  int onMessageComplete();

  http_parser parser_;
  bool parserError_{false};
  bool headersComplete_{false};

  // C-callable wrappers for the http_parser callbacks
  static int onMessageBeginCB(http_parser* parser);
  static int onPathCB(http_parser* parser, const char* buf, size_t len);
  static int onQueryStringCB(http_parser* parser, const char* buf, size_t len);
  static int onUrlCB(http_parser* parser, const char* buf, size_t len);
  static int onReasonCB(http_parser* parser, const char* buf, size_t len);
  static int onHeaderFieldCB(http_parser* parser, const char* buf, size_t len);
  static int onHeaderValueCB(http_parser* parser, const char* buf, size_t len);
  static int onHeadersCompleteCB(http_parser* parser,
                                 const char* buf, size_t len);
  static int onBodyCB(http_parser* parser, const char* buf, size_t len);
  static int onChunkHeaderCB(http_parser* parser);
  static int onChunkCompleteCB(http_parser* parser);
  static int onMessageCompleteCB(http_parser* parser);

  static void initParserSettings() __attribute__ ((__constructor__));

  static http_parser_settings kParserSettings;
};

}
