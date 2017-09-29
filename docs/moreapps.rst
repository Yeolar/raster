.. _moreapps:

更多的应用示例
==============

在 :ref:`tutorial` 中，从raster支持的协议的角度，给出了一些简单的应用示例。

在本节中，再通过两个示例讲解一下异步请求和并行计算的使用方法。

代理服务
--------

代理服务是一种很常见的服务类型。比如Nginx、Twemproxy，它们分别对后端的HTTP服务和Redis服务提供代理转发。在代理服务中，可以实现包括分发、负载均衡、AB测试、白名单、数据过滤或归并等等功能，因此应用极为广泛。

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

在某些业务场景，一个请求所需要的计算任务量可能会很大，比如搜索和推荐中都包含的排序功能。这时请求的耗时较长，对于用户来说体验很差，另外这也使工作线程被占用的时间段比较长，从调度层面看，可能会造成队列阻塞。

解决这个问题可以通过并行计算来做。首先区分一下概念，这里的并行计算指的是拆分任务在单机多线程执行，以缩短任务的执行时间。另外一个概念是分布式计算，它同样是拆分任务并行执行，但是它把子任务分布到多台机器组成的计算集群上。如果是大规模计算任务，一般需要通过分布式计算解决。

回到并行计算，它可以优化重计算的请求。虽然单机吞吐量没有变化，但是每个请求的耗时缩短了。raster框架中以协程调度的方式实现了并行计算，并且是基于图来做依赖关系的调度的，提供非常灵活的使用方式。

:file:`examples/parallel/` 是一个并行计算的示例，它也是从empty服务的基础上修改而来，thrift接口文件基本一致。

对任务做并行计算需要对它做拆分，拆分有多种形式，下面分别以函数和配置两种形式为例作介绍。

动态函数范式
~~~~~~~~~~~~

并行计算使用 ``JobExecutor`` 派生的任务执行器来作为调度单元。对于函数形式，已经做了通用的封装。创建函数形式的并行计算，首先定义好子任务的函数，然后将它们添加到创建的 ``Scheduler`` ，调用 ``run`` 方法启动并行调度。

.. code-block:: c++

    // 子任务函数
    void parallelFunc(int id) {
      RDDLOG(INFO) << "handle in parallelFunc " << id;
    }

    void run(Result& _return, const Query& query) {
      // ...
      // 函数形式的并行计算
      auto scheduler = make_unique<Scheduler>();
      for (size_t i = 1; i <= 4; i++) {
        scheduler->add(std::bind(parallelFunc, i));
      }
      scheduler->run();
      // ...
    }

配置范式
~~~~~~~~

配置形式的并行计算要复杂一些。因为需要从配置信息关联到任务执行器，这是通过反射机制来实现的。目前raster提供了类的反射机制，因此需要将子任务定义为类的形式，并且这个类的构造函数必须无参数，可以通过派生 ``Context`` 类来传递需要的数据。

定义了任务执行器类之后，需要通过 ``RDD_RF_REG()`` 宏注册它。

调度配置形式的子任务同样通过 ``Scheduler`` 完成。

.. code-block:: c++

    // 子任务执行器
    class ParallelJobExecutor
      : public ReflectObject<JobExecutor, ParallelJobExecutor> {
    public:
      struct MyContext : public Context {
        parallel::Result* result;
      };

      virtual ~ParallelJobExecutor() {}

      void handle() {
        if (context_) {
          RDDLOG(INFO) << "handle in " << name_ << ": code="
            << std::dynamic_pointer_cast<MyContext>(context_)->result->code;
        } else {
          RDDLOG(INFO) << "handle in " << name_;
        }
      }
    };
    // 注册执行器
    RDD_RF_REG(JobExecutor, ParallelJobExecutor);

    void run(Result& _return, const Query& query) {
      // ...
      // 配置形式的并行计算
      auto jobctx = new ParallelJobExecutor::MyContext();
      jobctx->result = &_return;
      make_unique<Scheduler>("graph1", JobExecutor::ContextPtr(jobctx))->run();
      // ...
    }

配置形式的并行计算需要提供配置，比如如下的配置：

.. code-block:: json

  "job": {
    "graph": {
      "graph1": [
        {"name": "ParallelJobExecutor:1", "next": ["ParallelJobExecutor:3"]},
        {"name": "ParallelJobExecutor:2", "next": ["ParallelJobExecutor:3"]},
        {"name": "ParallelJobExecutor:3", "next": []},
        {"name": "ParallelJobExecutor:4", "next": []}
      ]
    }
  }

配置了名为 ``graph1`` 的并行计算调度策略，它包含了4个子任务，分别是 ``ParallelJobExecutor:1`` 、 ``ParallelJobExecutor:2`` 、 ``ParallelJobExecutor:3`` 、 ``ParallelJobExecutor:4`` ，其中 ``ParallelJobExecutor:3`` 依赖于 ``ParallelJobExecutor:1`` 和 ``ParallelJobExecutor:2`` 。因此上面的配置的调度图为：

::

             ParallelJobExecutor:1
           /                       \
          /                          ParallelJobExecutor:3
         /                         /
    START -- ParallelJobExecutor:2
         \
          \  ParallelJobExecutor:4

子任务的命名规则是：冒号分隔之前的部分为类名。可以定义 ``ParallelJobExecutor`` 、 ``ParallelJobExecutor:suffix`` ，这样虽然是同一个类，但视作不同的子任务。

最后，使用配置形式的并行计算需要增加加载配置的钩子：

.. code-block:: c++

  config(FLAGS_conf.c_str(), {
         // ...
         {configJobGraph, "job.graph"}
         });

