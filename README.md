rddoc-framework
===============

rddoc-framewok是一个完整的高性能C++协程网络框架，支持thrift协议，未来将支持更多协议。

该框架借鉴了Facebook的folly基础库的一些思想和内容，但更加注重于轻量、易用、扩展性，同时，已经支持了监控统计、存储、数据库等实用需求。

框架的开发初衷是可以快速完成C++服务的开发，目前用于 rddoc.com 的搜索、持久化KV存储、代理服务等。另外它的早期版本也曾在一家互联网公司用于线上超过2000个服务节点。

依赖包括 Boost CURL GFlags ICU LevelDB Libmemcached MySQL OpenSSL ZLIB，其中部分是扩展插件的依赖。

编译

    $ mkdir build && cd build
    $ make -j8

运行demo

    $ ./empty-thrift/empty-thrift -conf ../empty-thrift/server.json

TODO: 未来会在性能、协议等方面进一步扩展。
