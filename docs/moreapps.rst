.. _moreapps:

更多的应用示例
==============

在 :ref:`tutorial` 中，从raster支持的协议的角度，给出了一些简单的应用示例。

在本节中，再通过两个示例讲解一下异步请求和并行计算的使用方法。

代理服务
--------

代理服务是一种很常见的服务类型。比如Nginx、Twemproxy，它们分别对后端的HTTP服务和Redis服务提供代理转发。在代理服务中，可以实现包括分发、负载均衡、AB测试、数据过滤或归并等等功能，因此应用极为广泛。

在代理服务或者后端架构中的中间层服务中，一个常见的开发模式是接收请求后，再请求后端服务，将返回结果处理后返回。

在同步模型下，对后端的请求会阻塞处理线程，使可用线程减少。这造成了资源的低效率使用，同时服务的吞吐能力较小。raster采用异步协程的模型，接收请求和发送请求都是异步模式的，解决了这一问题。使用异步模型的框架有很多，基于libevent也可以很快速地实现一个，但是通常的异步框架是采用回调函数注册的编程范式，在代码的开发和阅读上都不方便。raster底层使用协程，事件的调度建立在协程之上，这样仍可以保持同步的编程范式，实现异步效果。

:file:`examples/proxy/` 包含了一个采用thrift协议的代理服务示例。这里简单讲解一下，细节可以参考源码。

proxy服务是在empty服务的基础上修改而来。首先，修改查询Query的结构以包含转发地址：

.. code-block:: thrift

    struct Query {
        1: required string traceid;
        2: optional string query;
        3: optional string forward;
    }

修改 :file:`Server.cpp` 中 ``run`` 接口的实现：

.. code-block:: c++

  void run(Result& _return, const Query& query) {
    _return.__set_traceid(generateUuid(query.traceid, "rdde"));
    _return.__set_code(ResultCode::OK);

    // 转发请求
    if (!query.forward.empty()) {
      Peer peer(query.forward);
      Query q;
      q.__set_traceid(query.traceid);
      q.__set_query(query.query);
      TAsyncClient<ProxyClient> client(peer.host, peer.port);
      client.setKeepAlive();
      if (!client.connect() ||
          !client.fetch(&ProxyClient::recv_run, _return,
                        &ProxyClient::send_run, q)) {
        _return.__set_code(ResultCode::E_BACKEND_FAILURE);
      }
    }
  }

其中，使用 ``TAsyncClient`` 构建了一个thrift异步客户端请求，然后 ``connect`` 后端服务，再通过 ``fetch`` 完成请求。

这个示例比较简单，只是为了演示异步请求的使用方法，实际中代理服务需要考虑的问题要多很多。

如果是对后端的一连串请求，可以编写多个异步客户端。那么如果是一组同类请求呢？是否可以把它们并行发起处理？答案是肯定的。比如请求后端多个数据分片进行合并，请求之间互相无依赖关系，可以把它们并发处理，这样单个请求的耗时可以大大缩短。

并发请求可以使用 ``MultiAsyncClient`` 创建。需要注意，它不支持 ``fetch`` 方法，而是分别调用 ``send`` 、 ``yield`` 、 ``recv`` 来完成请求。

举例如下（到同一个后端的并发请求）：

.. code-block:: c++

    MultiAsyncClient<TAsyncClient<Client>> clients(n, host, port);
    if (!clients.connect()) {
      return;
    }
    std::vector<Result> res_list;
    res_list.resize(n);

    // 发起请求
    for (size_t i = 0; i < n; ++i) {
      clients.send(i, &Client::send, req);
    }
    // 挂起当前协程任务
    clients.yield();
    // 接收结果（协程任务已经恢复执行）
    for (size_t i = 0; i < n; ++i) {
      if (!clients.recv(i, &Client::recv, res_list[i])) {
        RDDLOG(WARN) << "receive failed for request no." << i;
      }
    }

``MultiAsyncClient`` 的模板要求客户端需要是同一类型。更一般的情况，对于不同类型的客户端请求，可以在调用它们的 ``send`` 之后，调用 ``yieldMultiTask`` ，再调用它们的 ``recv`` ，完成并发请求。

并行计算
--------


