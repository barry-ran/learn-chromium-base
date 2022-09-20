#include <iostream>

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

#include "base/bind.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"

// Callback
// https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/docs/callback.md
void TestCallback() {
  // clang-format off
  // Callback可以理解为是chromium中使用的function
  // 有很强的功能，尤其是配合base::Thread&base::TaskRunner使用时
  // 1. Callback作为参数时，如果需要转移所有权，用值传递，否则const引用传递
  // 2. OnceCallback只能通过右值（move）的方式运行，所以只能运行一次（用完就销毁了）
  // 3. RepeatingCallback可以运行多次，最后一次运行可以用move运行来达到运行并销毁的目的
  // 4. 尽量使用OnceCallback，满足不了再用RepeatingCallback
  // 5. 链式回调，类似once_callback_b(once_callback_a())
  // 6. 可以拆分一个Callback为两个
  // 7. 多次Run，触发一次回调的BarrierCallback
  // 8. Callback可以绑定到普通函数，成员函数，Lambda表达式
  // 9. 给Callback赋值base::DoNothing()表示什么都不做
  // 10. 没有参数的Callback又叫Closure（OnceClosure/RepeatingClosure）
  // 11. 绑定成员函数注意对象生命周期管理 https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/docs/callback.md#quick-reference-for-advanced-binding
  // 12. 绑定参数相关 https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/docs/callback.md#quick-reference-for-binding-parameters-to-bindonce_and-bindrepeating
  // clang-format on

  base::OnceCallback<void(int)> once_callback_1 = base::BindOnce(
      [](int a) { std::cout << "run once callback " << a << std::endl; });
  std::move(once_callback_1).Run(1);
  base::RepeatingCallback<void(int)> repeating_callback_1 = base::BindRepeating(
      [](int a) { std::cout << "run repeating callback" << a << std::endl; });
  repeating_callback_1.Run(1);
  repeating_callback_1.Run(2);
  std::move(repeating_callback_1).Run(3);

  // clang-format off
  // 链式回调，类似once_callback_b(once_callback_a())
  // 1. 如果第二个Callback不需要第一个Callback的参数，使用base::IgnoreResult实现
  // 2. 链式回调的两个回调可以运行在不同的task runners，使用base::BindPostTask实现
  // clang-format on
  base::OnceCallback<int(float)> once_callback_a =
      base::BindOnce([](float f) { return static_cast<int>(f); });
  base::OnceCallback<std::string(int)> once_callback_b =
      base::BindOnce([](int i) { return base::NumberToString(i); });
  std::string r =
      std::move(once_callback_a).Then(std::move(once_callback_b)).Run(3.5f);
  std::cout << "run chaining callback " << r << std::endl;
}

// Chromium 消息循环和线程池 (MessageLoop 和 TaskScheduler)
// https://keyou.github.io/blog/2019/06/11/Chromium-MessageLoop-and-TaskScheduler/
// https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/docs/threading_and_tasks.md
// clang-format off
// chromium线程设计：
// 1. 一个main thread，不阻塞，处理主要业务逻辑，如果是ui进程则还负责更新ui
// 2. 一个io thread，负责异步io的处理(ipc，网络，文件)，io数据相关的逻辑处理放在别的线程
// 3. 少量线程做一些特定的cpu密集任务
// 4. 一个thread pool做一些通用的cpu密集任务
// 5. 线程之间通过task沟通

// task运行方式：
// 1. 并行：在任意线程上并行执行
// 2. 序列执行：顺序执行，但是可能在任意线程上执行
// 3. 单线程序列执行：在同一个线程上顺序执行
// 相比3更推荐2，因为base::SequencedTaskRunner能比您自己管理物理线程更好地实现线程安全
// clang-format on
void TestThread() {
  std::cout << "start TestThread:" << base::Time::Now() << std::endl;

  // ThreadPool
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("my_thread_pool");
  DCHECK(base::ThreadPoolInstance::Get());

  // 并行1：ThreadPool::PostTask
  for (int i = 0; i < 10; ++i) {
    base::ThreadPool::PostTask(
        FROM_HERE, base::BindOnce(
                       [](int i) {
                         std::cout
                             << "run ThreadPool task " << i << " on thread id:"
                             << base::PlatformThread::CurrentId() << std::endl;
                       },
                       i));
  }
  // 指定任务特点
  base::ThreadPool::PostTask(
      FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
      base::BindOnce([]() {
        std::cout << "run ThreadPool BEST_EFFORT task" << std::endl;
      }));
  // 并行2：TaskRunner(除非测试需要精确控制任务的执行方式，否则直接用ThreadPool::PostTask)
  // CreateTaskRunner创建的是并行的TaskRunner
  scoped_refptr<base::TaskRunner> task_runner =
      base::ThreadPool::CreateTaskRunner({base::TaskPriority::USER_VISIBLE});
  task_runner->PostTask(FROM_HERE, base::BindOnce([]() {
                          std::cout << "run ThreadPool on TaskRunner"
                                    << std::endl;
                        }));

  base::ThreadPoolInstance::Get()->JoinForTesting();
  base::ThreadPoolInstance::Get()->Shutdown();
  // Shutdown中故意不释放ThreadPoolInstance而内存泄漏(退出时由系统回收没啥影响)，我们可以通过Set来释放，两者区别是：
  // 1. 不调用Set，Shutdown后继续ThreadPool::PostTask没啥反应
  // 2. 调用Set，Shutdown后继续ThreadPool::PostTask会decheck崩溃
  // base::ThreadPoolInstance::Set(nullptr);
  // base::ThreadPool::PostTask(FROM_HERE, base::BindOnce([]() {}));

  base::Thread work_thread("ThreadName");
  base::Thread::Options options;
  options.message_pump_type = base::MessagePumpType::IO;
  work_thread.StartWithOptions(std::move(options));
  std::cout << "thread id:" << work_thread.GetThreadId() << std::endl;

  std::cout << "thread IsRunning:" << work_thread.IsRunning() << std::endl;
  base::PlatformThread::Sleep(base::Milliseconds(200));
  // Thread Start以后，Thread对象析构或者stop之前都是IsRunning状态
  std::cout << "thread IsRunning:" << work_thread.IsRunning() << std::endl;

  work_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce([]() {
        std::cout << "run task1 on thread id:"
                  << base::PlatformThread::CurrentId() << std::endl;

        std::cout << "sleep 200ms:" << base::PlatformThread::CurrentId()
                  << std::endl;
        base::PlatformThread::Sleep(base::Milliseconds(200));
      }));

  // 上一个任务执行了200ms，但是work_thread线程停止的时候会等待它执行完，
  // 并继续执行剩下的需要执行的task
  work_thread.task_runner()->PostTask(FROM_HERE, base::BindOnce([]() {
                                        std::cout
                                            << "run task2 on thread id:"
                                            << base::PlatformThread::CurrentId()
                                            << std::endl;
                                      }));
  // Delayed task能否被执行，取决于work_thread线程停止的时候delayed是否到期
  work_thread.task_runner()->PostDelayedTask(
      FROM_HERE, base::BindOnce([]() {
        std::cout << "run delay task on thread id:"
                  << base::PlatformThread::CurrentId() << std::endl;
      }),
      base::Milliseconds(250));

  // 50ms work_thread析构，线程自动停止
  base::PlatformThread::Sleep(base::Milliseconds(50));
  std::cout << "stop TestThread:" << base::Time::Now() << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "start demo:" << base::Time::Now() << std::endl;

  std::cout << std::endl << "***********************" << std::endl << std::endl;
  TestCallback();
  std::cout << std::endl << "***********************" << std::endl << std::endl;
  TestThread();
  std::cout << std::endl << "***********************" << std::endl << std::endl;

  // getchar();

  std::cout << "stop demo:" << base::Time::Now() << std::endl;
  return 0;
}