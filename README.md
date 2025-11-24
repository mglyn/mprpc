# mprpc — A minimal C++ RPC framework (example)

这是一个基于 Protobuf + muduo 的简易 RPC 框架示例，演示了如何定义 proto 服务、生成 C++ 代码，并使用服务端（provider）与客户端（consumer）进行 RPC 调用。

## 特性

- 使用 Protocol Buffers (proto3) 定义消息与服务接口。
- 基于 muduo 网络库实现的异步 TCP 服务。
- 将服务注册到 ZooKeeper（临时节点），便于服务发现（示例）。
- 包含简单的示例：`UserService`（登录接口）。

## 目录结构（摘录）

- `CMakeLists.txt` - 顶层构建文件，输出可执行到 `bin/`，静态库到 `lib/`。
- `src/` - 框架核心实现（mprpc 库）：包含 `mprpcApplication`、`RpcProvider`、`mprpcChannel`、配置与 ZK 辅助等。
- `example/` - 示例：`caller`（客户端）、`callee`（服务端）、`user.proto` 与生成的 `user.pb.*`。
- `bin/` - 构建后可执行文件输出目录。

完整的文件列表请参见仓库。

## 依赖

在构建前请确保安装并可用以下依赖：

- CMake >= 3.10
- g++ (支持 C++11 或更高)
- Protocol Buffers（libprotobuf、protoc）
- muduo（网络库，含 `muduo_net` 和 `muduo_base`）
- ZooKeeper C client（`zookeeper_mt`）
- pthread

在 Debian/Ubuntu 上，可参考如下安装命令（根据实际系统与包管理器调整）：

```bash
# 示例（请根据系统调整）
sudo apt-get update
sudo apt-get install -y build-essential cmake libprotobuf-dev protobuf-compiler libpthread-stubs0-dev
# muduo, zookeeper 请按项目要求单独安装或编译
```

## 构建

在仓库根目录执行：

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

成功构建后，示例可执行文件会被放置到仓库根目录下的 `bin/`（由顶层 CMakeLists.txt 指定）。

## 配置文件示例

框架通过命令行参数 `-i <configfile>` 加载一个简单的键值配置文件（每行 `key=value`）。支持的配置项示例：

```
rpcserverip=127.0.0.1
rpcserverport=9000
zookeeperip=127.0.0.1
zookeeperport=2181
```

保存为例如 `rpcserver.conf`，然后以 `-i rpcserver.conf` 作为程序参数运行服务或客户端。

## 运行示例

先运行服务端（示例 `UserService`）:

```bash
# 从仓库根目录
./bin/provider -i ./rpcserver.conf
```

在另一个终端运行客户端：

```bash
./bin/consumer -i ./rpcserver.conf
```

示例客户端会调用 `UserService::Login` 并打印返回结果。

## Protobuf 文件

示例 proto 文件：`example/user.proto`，定义了 `LoginRequest`、`LoginResponse`、`ResultCode` 以及服务 `UserServiceRpc`。生成的 C++ 文件位于 `example/user.pb.cc` / `example/user.pb.h`。

如果修改了 `.proto` 文件，请重新生成代码：

```bash
protoc --cpp_out=. example/user.proto
```

## 已知事项

- 框架示例中将服务信息注册到 ZooKeeper 节点，依赖本地可用的 ZooKeeper 服务。
- `src/CMakeLists.txt` 将生成 `libmprpc` 并链接 `muduo_net`、`muduo_base`、`zookeeper_mt`、`pthread` 等库。请确保这些库在系统中可用或通过指定 CMake 前缀路径找到。
- `mprpcApplication` 通过 `-i` 参数加载配置文件，若未提供会退出。
- RPC 消息的打包格式：4 字节 header_size + header + args。header 使用 `mprpc::RpcHeader` 定义（见 `src/rpcHeader.proto`）。
