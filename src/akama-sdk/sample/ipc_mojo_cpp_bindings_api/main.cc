#include <iostream>

#include "akama-sdk/sample/ipc_mojo_cpp_bindings_api/mojom/keep_alive.mojom.h"

#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/system/invitation.h"

constexpr int KKeepliveInterval = 5;
constexpr int KClientNum = 5;

class KeepAliveImpl : public ipc::mojom::KeepAlive {
 public:
  explicit KeepAliveImpl() {}
  KeepAliveImpl(const KeepAliveImpl&) = delete;
  KeepAliveImpl& operator=(const KeepAliveImpl&) = delete;
  ~KeepAliveImpl() override {}

  void AddPendingReceiver(
      mojo::PendingReceiver<ipc::mojom::KeepAlive> pending_receiver) {
    receivers_.emplace_back(new mojo::Receiver<ipc::mojom::KeepAlive>(
        this, std::move(pending_receiver)));
  }

  void Heartbeat(int32_t in_process_id) override {
    std::cout << base::Process::Current().Pid()
              << ":[keep alive] recv heart beat: " << in_process_id
              << std::endl;
  }

 private:
  std::vector<mojo::Receiver<ipc::mojom::KeepAlive>*> receivers_;
};

mojo::ScopedMessagePipeHandle RunClientProcessAndConnect() {
  // 创建一条系统级的IPC通信通道，用于支持MessagePipe
  // 在linux上是 socket pair, Windows 是 named pipe
  mojo::PlatformChannel channel;
  base::LaunchOptions options;
  base::CommandLine command_line(
      base::CommandLine::ForCurrentProcess()->GetProgram());
  command_line.AppendArg("--client");
  // 将channel信息填充到启动参数传递给子进程
  channel.PrepareToPassRemoteEndpoint(&options, &command_line);
  base::Process child_process = base::LaunchProcess(command_line, options);
  // 和PrepareToPassRemoteEndpoint成对，启动完进程调用
  channel.RemoteProcessLaunchAttempted();

  // invitation借助channel将MessagePipe发送到子进程
  mojo::OutgoingInvitation invitation;
  mojo::ScopedMessagePipeHandle pipe =
      invitation.AttachMessagePipe("keep_alive_pipe");
  mojo::OutgoingInvitation::Send(std::move(invitation), child_process.Handle(),
                                 channel.TakeLocalEndpoint());

  return pipe;
}

mojo::ScopedMessagePipeHandle RecvMessagePipe() {
  mojo::IncomingInvitation invitation = mojo::IncomingInvitation::Accept(
      mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
          *base::CommandLine::ForCurrentProcess()));

  return invitation.ExtractMessagePipe("keep_alive_pipe");
}

void Master() {
  std::cout << base::Process::Current().Pid() << ":master process running"
            << std::endl;
  // todo
  // 父进程心跳检测子进程终止则重新创建
  // 子进程心跳检测父进程退出则子进程退出
  KeepAliveImpl keep_alive_receiver;
  for (int i = 0; i < KClientNum; ++i) {
    auto pipe = RunClientProcessAndConnect();
    mojo::PendingReceiver<ipc::mojom::KeepAlive> pending_receiver(
        std::move(pipe));
    keep_alive_receiver.AddPendingReceiver(std::move(pending_receiver));
  }

  base::RunLoop().Run();
}

void Client() {
  std::cout << base::Process::Current().Pid() << ":client process running"
            << std::endl;

  mojo::ScopedMessagePipeHandle pipe = RecvMessagePipe();
  auto* remote = new mojo::Remote<ipc::mojom::KeepAlive>(
      mojo::PendingRemote<ipc::mojom::KeepAlive>(std::move(pipe), 0));

  base::RepeatingTimer timer;
  timer.Start(FROM_HERE, base::Seconds(KKeepliveInterval),
              base::BindRepeating(
                  [](mojo::Remote<ipc::mojom::KeepAlive>* remote) {
                    (*remote)->Heartbeat(base::Process::Current().Pid());
                  },
                  remote));

  base::RunLoop().Run();
}

int main(int argc, char* argv[]) {
  std::cout << base::Process::Current().Pid() << ":start ipc" << std::endl;
  base::CommandLine::Init(argc, argv);

  MojoResult ret = MojoInitialize(nullptr);
  if (MOJO_RESULT_OK != ret) {
    std::cout << base::Process::Current().Pid() << ":mojo init failed" << ret
              << std::endl;
    return 1;
  }

  base::SingleThreadTaskExecutor main_task_executer;

  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("client")) {
    Client();
  } else {
    Master();
  }

  MojoShutdown(nullptr);
  std::cout << base::Process::Current().Pid() << ":stop ipc" << std::endl;
  return 0;
}