使用教程
========

raster的依赖包括 Boost CURL GFlags ICU OpenSSL Protobuf ZLIB。

编译安装::

    $ mkdir build && cd build
    $ make -j8
    $ sudo make install

运行demo::

    $ ./examples/empty/empty -conf ../examples/empty/server.json
    $ ./examples/empty/empty-bench -count 1000

