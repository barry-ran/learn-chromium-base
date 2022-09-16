#include <iostream>

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

#include "base/bind.h"
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
void TestThread() {
  std::cout << "start TestThread:" << base::Time::Now() << std::endl;

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