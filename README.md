raster
======

raster是一个完整的高性能C++协程服务框架。

该框架借鉴了Facebook的folly基础库的一些思想和内容，但更加注重于轻量、易用、扩展性，同时，已经支持了监控统计、存储、数据库等实用需求。

框架主要特性：

- 支持异步，协程
- 支持多个后端并发请求
- 支持binary、thrift、pbrpc协议
- 支持并行计算
- 支持监控统计、存储、数据库等扩展功能

在开发本框架之前，我曾经开发过另两套服务框架：

- [ctornado](https://github.com/Yeolar/ctornado) - 非常流行的一款Python HTTP异步网络框架Tornado的C++版本，出于练习目的，并未实际使用到生产环境。
- sf（未开源） - 一套C++的异步RPC服务框架，目前在中国国内一家一线互联网公司用于线上超过2000个服务节点。

raster是我针对目前的微服务的概念，参考之前的项目以及folly等开源库，重新开发的一套服务框架。

框架的开发初衷是可以快速完成C++服务的开发，目前用于 [rddoc.com](https://www.rddoc.com/) 的搜索、持久化KV存储、代理服务等。

依赖包括 Boost CURL GFlags ICU LevelDB Libmemcached MySQL OpenSSL Protobuf ZLIB，其中部分是扩展插件的依赖。

编译安装

    $ mkdir build && cd build
    $ make -j8
    $ sudo make install

运行demo

    $ ./examples/empty/empty -conf ../examples/empty/server.json
    $ ./examples/empty/empty-bench -count 1000

TODO: 未来会在性能、协议等方面进一步优化和扩展。

