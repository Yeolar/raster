使用教程
========

编译安装
--------

raster使用C++11开发，目前可以在Linux平台下工作。在Debian8环境下通过测试。

依赖包括 Boost CURL GFlags ICU OpenSSL Protobuf ZLIB。

通过以下命令完成编译安装::

    $ mkdir build && cd build
    $ make -j8
    $ sudo make install

这将会安装raster到 :file:`/usr/local/` 路径下。

使用方法
--------

raster可以支持thift协议、pbrpc协议，还可以自己扩展以支持其他的协议。下面通过示例，具体描述一下使用方法。

thrift协议
~~~~~~~~~~

raster可以和Apache thrift兼容。

以一个简单的查询服务为例，定义如下thrift接口文件 :file:`Empty.thrift`` ：

.. code-block:: thrift

    // 查询Query结构
    struct Query {
        1: required string traceid;     // 请求跟踪ID
        2: optional string query;       // 请求查询词
    }

    enum ResultCode {
        OK, // 0
    }

    // 结果结构
    struct Result {
        1: required string traceid;     // 请求跟踪ID
        2: optional ResultCode code;    // 结果码
    }

    // 服务接口
    service Empty {
        Result run(1: Query query);
    }

运行 ``thrift --gen cpp Empty.thrift`` 生成thrift的序列化和路由相关的代码。

和thrift类似，编写 :file:`Server.cpp` ，实现定义的接口。

.. code-block:: c++

    class EmptyHandler : virtual public EmptyIf {
    public:
      EmptyHandler() {
      }

      // 实现接口方法
      void run(Result& _return, const Query& query) {
        _return.__set_traceid(generateUuid(query.traceid, "rdde"));
        _return.__set_code(ResultCode::OK);
      }
    };

在 :file:`Server.cpp` 中实现 ``main`` 函数。

.. code-block:: c++

    int main(int argc, char* argv[]) {
      // 使用gflags处理命令行参数
      google::ParseCommandLineFlags(&argc, &argv, true);
      // 处理信号
      setupIgnoreSignal(SIGPIPE);
      setupShutdownSignal(SIGINT);
      setupShutdownSignal(SIGTERM);
      // 定义Empty的thrift异步服务，添加到全局的Actor调度器
      std::shared_ptr<Service> empty(
          new TAsyncServer<empty::EmptyHandler, empty::EmptyProcessor>());
      Singleton<Actor>::get()->addService("Empty", empty);
      // 根据配置文件配置服务
      config(FLAGS_conf.c_str(), {
             {configLogging, "logging"},
             {configActor, "actor"},
             {configService, "service"},
             {configThreadPool, "thread"},
             {configMonitor, "monitor"},
             });
      // 运行调度器以启动服务
      RDDLOG(INFO) << "rdd start ... ^_^";
      Singleton<Actor>::get()->start();
      // 程序结束
      google::ShutDownCommandLineFlags();

      return 0;
    }

我们已经基本完成了这个 ``Empty`` 服务的开发。

接下来了解一下服务的配置文件。raster采用JSON格式文件来配置，基本的配置包括调度器、线程、服务、日志、监控五个部分。下面是一个配置示例：

.. code-block:: json

    {
      "logging": {                  // 日志配置
        "logfile": "log/empty.log", // 日志文件路径
        "level": 1,                 // 日志级别
        "async": true               // 是否开启异步日志
      },
      "actor": {                    // 调度器配置
        "stack_size": 65536,        // 协程栈的大小（字节）
        "conn_limit": 100000,       // 总并发连接数限制
        "task_limit": 4000,         // 总并发任务数限制
        "poll_size": 1024,          // epoll大小
        "poll_timeout": 1000,       // epoll轮询超时（毫秒）
        "forwarding": false         // 是否开启请求转发
      },
      "service": {                  // 服务配置
        "8000": {                   // 8000端口
          "service": "Empty",       // 服务名
          "conn_timeout": 100000,   // 请求连接超时（微秒）
          "recv_timeout": 300000,   // 请求接收超时（微秒）
          "send_timeout": 1000000   // 请求发送超时（微秒）
        }
      },
      "thread": {                   // 线程配置
        "io": {                     // IO线程池
          "thread_count": 4,        // 线程数
          "bindcpu": false          // 是否绑定CPU
        },
        "0": {                      // 0号线程池，作为默认的工作线程
          "thread_count": 4,        // 线程数
          "bindcpu": false          // 是否绑定CPU
        }
      },
      "monitor": {                  // 监控配置
        "open": false,              // 是否开启
        "prefix": "empty"           // 监控项的前缀
      }
    }

完整的代码可以在 :file:`examples/empty/` 下找到，其中还包括一个基准测试工具 :file:`Bench.cpp` 。编译raster时会同时编译 :file:`examples` 下的示例。

运行 ``Empty`` 示例可以通过下面的命令::

    $ ./examples/empty/empty -conf ../examples/empty/server.json
    $ ./examples/empty/empty-bench -count 1000

上面的命令分别会启动 ``empty`` 和使用 ``empty-bench`` 压测。

:file:`Bench.cpp` 中使用同步客户端 ``TSyncClient`` 建立短连接请求，可以作为创建同步客户端请求的示例来参考。

pbrpc协议
~~~~~~~~~

如果您使用过protobuf v2，可能会知道它的proto接口文件提供了 ``service`` 语义。raster实现了这一语义，因此它可以支持protobuf的RPC。

:file:`examples/pbrpc/` 下包含了一个使用protobuf作为RPC通信的数据格式的示例。这个例子稍微复杂一点，它实现了一个代理服务，对应地， :file:`examples/proxy/` 是它的thrift实现版本。

这里先不讨论代理服务，只介绍一下pbrpc的开发方式。

首先，同样定义一个protobuf接口文件 :file:`Proxy.proto` ：

.. code-block:: proto

    // 查询Query结构
    message Query {
      required string traceid = 1;      // 请求跟踪ID
      optional string query = 2;        // 请求查询词
      optional string forward = 3;      // 请求转发地址
    }

    enum ResultCode {
      OK = 0;
    }

    // 结果结构
    message Result {
      required string traceid = 1;      // 请求跟踪ID
      optional ResultCode code = 2;     // 结果码
    }

    // 服务接口
    service ProxyService {
      rpc run(Query) returns(Result);
    }

运行 ``protoc --cpp_out=. Proxy.proto`` 生成protobuf的序列化和路由相关的代码。

接下来编写 :file:`Server.cpp` ，实现定义的接口。

.. code-block:: c++

    class ProxyServiceImpl : public ProxyService {
    public:
      ProxyServiceImpl() {
      }

      // 实现接口方法
      void run(::google::protobuf::RpcController* controller,
               const Query* request,
               Result* response,
               ::google::protobuf::Closure* done) {
        SCOPE_EXIT {
          done->Run();
        };

        if (!failReason_.empty()) {
          controller->SetFailed(failReason_);
          return;
        }

        response->set_traceid(generateUuid(request->traceid(), "rdde"));
        response->set_code(ResultCode::OK);

        // ...
      }

    private:
      std::string failReason_;
    };

在 :file:`Server.cpp` 中实现 ``main`` 函数。

.. code-block:: c++

    int main(int argc, char* argv[]) {
      // 使用gflags处理命令行参数
      google::ParseCommandLineFlags(&argc, &argv, true);
      // 处理信号
      setupIgnoreSignal(SIGPIPE);
      setupShutdownSignal(SIGINT);
      setupShutdownSignal(SIGTERM);
      // 定义Proxy的pbrpc异步服务，添加到全局的Actor调度器
      auto service = new PBAsyncServer();
      service->addService(std::make_shared<pbrpc::ProxyServiceImpl>());
      Singleton<Actor>::get()->addService(
          "Proxy", std::shared_ptr<Service>(service));
      // 根据配置文件配置服务
      config(FLAGS_conf.c_str(), {
             {configLogging, "logging"},
             {configActor, "actor"},
             {configService, "service"},
             {configThreadPool, "thread"},
             {configMonitor, "monitor"},
             });
      // 运行调度器以启动服务
      RDDLOG(INFO) << "rdd start ... ^_^";
      Singleton<Actor>::get()->start();
      // 程序结束
      google::ShutDownCommandLineFlags();

      return 0;
    }

可以沿用上面thrift示例的配置文件，把其中的 ``logging.logfile`` 和 ``service.8000.service`` 修改一下。

运行 ``pbrpc`` 示例可以通过下面的命令::

    $ ./examples/pbrpc/pbrpc -conf ../examples/pbrpc/server.json
    $ ./examples/pbrpc/pbrpc-bench -count 1000

上面的命令分别会启动 ``pbrpc`` 和使用 ``pbrpc-bench`` 压测。

:file:`Bench.cpp` 中使用同步客户端 ``PBSyncClient`` 建立短连接请求，可以作为创建同步客户端请求的示例来参考。

