#include <iostream>

#include <mojo/core/embedder/embedder.h>
#include <mojo/core/embedder/scoped_ipc_support.h>
#include "akama-sdk/sample/ipc/mojom/logger.mojom.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

int main(int argc, char* argv[]) {
  std::cout << "start ipc:" << base::Time::Now() << std::endl;
  /*
  MojoInitialize(nullptr);

  mojo::MessagePipe pipe;
  mojo::Remote<sample::mojom::Logger> logger(
      mojo::PendingRemote<sample::mojom::Logger>(std::move(pipe.handle0), 0));
  mojo::PendingReceiver<sample::mojom::Logger> receiver(
      std::move(pipe.handle1));
  */
  std::cout << "stop ipc:" << base::Time::Now() << std::endl;
  return 0;
}