#include <iostream>

#include <mojo/core/embedder/embedder.h>
#include <mojo/core/embedder/scoped_ipc_support.h>
#include "akama-sdk/sample/ipc/mojom/logger.mojom.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

// clang-format off
// Chromium Mojo & IPC：https://github.com/keyou/chromium_demo/blob/c/91.0.4472/docs/mojo.md
// mojo README：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/README.md
// mojo cpp bindings api，使用较多，基于cpp system api实现Mojom IDL和绑定生成器等：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/public/cpp/bindings/README.md
// mojo cpp system api,使用较多：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/public/cpp/system/README.md
// mojo c system api,使用较少：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/public/c/system/README.md
// clang-format on

class LoggerImpl : public sample::mojom::Logger {
 public:
  // NOTE: A common pattern for interface implementations which have one
  // instance per client is to take a PendingReceiver in the constructor.
  // Receiver通过PendingReceiver从pipe接收消息，并反序列化消息以后调用T的方法。
  explicit LoggerImpl(
      mojo::PendingReceiver<sample::mojom::Logger> pending_receiver)
      : receiver_(this, std::move(pending_receiver)) {
    receiver_.set_disconnect_handler(
        base::BindOnce(&LoggerImpl::OnError, base::Unretained(this)));
  }
  LoggerImpl(const LoggerImpl&) = delete;
  LoggerImpl& operator=(const LoggerImpl&) = delete;
  ~LoggerImpl() override {}

  // sample::mojom::Logger:
  void Log(const std::string& message) override {
    std::cout << "[Logger] " << message << std::endl;
    lines_.push_back(message);
  }

  void GetTail(GetTailCallback callback) override {
    std::move(callback).Run(lines_.back());
  }

  void OnError() {
    std::cout << "Client disconnected! Purging log lines." << std::endl;
    lines_.clear();
  }

 private:
  mojo::Receiver<sample::mojom::Logger> receiver_;
  std::vector<std::string> lines_;
};

int main(int argc, char* argv[]) {
  std::cout << "start ipc:" << base::Time::Now() << std::endl;

  MojoResult ret = MojoInitialize(nullptr);
  if (MOJO_RESULT_OK != ret) {
    std::cout << "mojo init failed" << ret << std::endl;
    return 1;
  }

  base::Thread ipc_thread("ipc");
  base::Thread::Options options;
  options.message_pump_type = base::MessagePumpType::IO;
  ipc_thread.StartWithOptions(std::move(options));

  ipc_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce([]() {
  // 两种方式等效：本质就是构造一个MessagePipe，sender和receiver分别绑定到两端来发送和接收
  // PendingRemote是发送端对pipe的封装，PendingReceiver是接收端对pipe的封装
  // Remote负责序列化T的接口调用，并通过PendingRemote把消息发送到pipe
  // Receiver通过PendingReceiver从pipe接收消息，反序列化消息以后调用T的接口
  //
  // mojo cpp binding api
  // 使进程间通信像创建一个接口对象（Logger接口指针指向LoggerImpl实现）并调用其方法一样简单。
  // Remote持有对象的接口，Receiver持有对象实现
#if 0
            mojo::MessagePipe pipe;
            auto* sender = new mojo::Remote<sample::mojom::Logger>(
                mojo::PendingRemote<sample::mojom::Logger>(
                    std::move(pipe.handle0), 0));
            mojo::PendingReceiver<sample::mojom::Logger> pending_receiver(
                std::move(pipe.handle1));
#else
        mojo::Remote<sample::mojom::Logger>* sender =
            new mojo::Remote<sample::mojom::Logger>;
        auto pending_receiver = sender->BindNewPipeAndPassReceiver();
#endif
        LoggerImpl* recver = new LoggerImpl(std::move(pending_receiver));
        (void)recver;

        (*sender)->Log("Hello!");
        (*sender)->GetTail(base::BindOnce([](const std::string& tail) {
          std::cout << "tail: " << tail << std::endl;
        }));

        // 模拟断开管道，触发LoggerImpl::OnError
        // sender->reset();
      }));
  // base::PlatformThread::Sleep(base::Milliseconds(1000));
  ipc_thread.Stop();

  MojoShutdown(nullptr);
  std::cout << "stop ipc:" << base::Time::Now() << std::endl;
  return 0;
}