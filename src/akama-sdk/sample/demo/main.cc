#include <iostream>

//#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

#include "base/bind.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"

// Chromium 消息循环和线程池 (MessageLoop 和 TaskScheduler)
// https://keyou.github.io/blog/2019/06/11/Chromium-MessageLoop-and-TaskScheduler/
// https://chromium.googlesource.com/chromium/src/+/refs/tags/62.0.3175.0/docs/threading_and_tasks.md

// callback
// https://chromium.googlesource.com/chromium/src/+/refs/tags/62.0.3175.0/docs/callback.md
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

  TestThread();

  getchar();

  std::cout << "stop demo:" << base::Time::Now() << std::endl;
  return 0;
}