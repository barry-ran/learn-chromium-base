#include <iostream>

#include <mojo/core/embedder/embedder.h>
#include <mojo/core/embedder/scoped_ipc_support.h>
#include "akama-sdk/sample/ipc/mojom/logger.mojom.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

// clang-format off
// Chromium Mojo & IPC：https://keyou.github.io/blog/2020/01/03/Chromium-Mojo&IPC/
// mojo README：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/README.md
// mojo cpp bindings api，使用较多，基于cpp system api实现Mojom IDL和绑定生成器等：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/public/cpp/bindings/README.md
// mojo cpp system api,使用较多：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/public/cpp/system/README.md
// mojo c system api,使用较少：https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/mojo/public/c/system/README.md
// clang-format on
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
        /*
        mojo::MessagePipe pipe;
        mojo::Remote<sample::mojom::Logger> logger(
            mojo::PendingRemote<sample::mojom::Logger>(std::move(pipe.handle0),
        0)); mojo::PendingReceiver<sample::mojom::Logger> receiver(
            std::move(pipe.handle1));
            */
        mojo::Remote<sample::mojom::Logger> logger;
        auto receiver = logger.BindNewPipeAndPassReceiver();
      }));
  ipc_thread.Stop();

  MojoShutdown(nullptr);
  std::cout << "stop ipc:" << base::Time::Now() << std::endl;
  return 0;
}