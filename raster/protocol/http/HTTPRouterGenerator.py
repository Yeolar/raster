#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2017, Yeolar
#

import sys


with open(sys.argv[1]) as fp:
    routers = fp.readlines()

for router in routers:
    router = router.strip()
    if not router or router.startswith('#'):
        continue
    handler, _, uri = router.partition('=')
    d = {
        'handler': handler.strip(),
        'uri': uri.strip(),
    }
    print """\
class %(handler)sHandler : public HTTPRequestHandler {
public:
  %(handler)sHandler() {}
  ~%(handler)sHandler() {}

  void GET(const std::string& uri) {
  }
  void POST(const std::string& uri) {
  }
};
""" % d
