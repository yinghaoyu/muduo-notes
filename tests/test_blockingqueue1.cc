#include "../convey/base/BlockingQueue.h"
#include "../convey/base/CountDownLatch.h"
#include "../convey/base/Logging.h"
#include "../convey/base/Thread.h"
#include "../convey/base/Timestamp.h"

#include <stdio.h>
#include <unistd.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

bool g_verbose = false;

// Many threads, one queue.
class Bench
{
 public:
  // 创建n个线程
  Bench(int numThreads) : latch_(numThreads)
  {
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.emplace_back(new convey::Thread(std::bind(&Bench::threadFunc, this), convey::string(name)));
    }
    for (auto &thr : threads_)
    {
      thr->start();
    }
  }

  void run(int times)
  {
    printf("waiting for count down latch\n");
    latch_.wait();
    LOG_INFO << threads_.size() << " threads started";
    int64_t total_delay = 0;
    for (int i = 0; i < times; ++i)
    {
      convey::Timestamp now(convey::Timestamp::now());
      queue_.put(now);
      total_delay += delay_queue_.take();
    }
    printf("Average delay: %.3fus\n", static_cast<double>(total_delay) / times);
  }

  void joinAll()
  {
    // 非法的时间戳数量等于子线程数量，这样能让子线程全部正常结束
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put(convey::Timestamp::invalid());
    }

    for (auto &thr : threads_)
    {
      thr->join();
    }
    LOG_INFO << threads_.size() << " threads stopped";
  }

 private:
  // 每个线程运行的函数
  void threadFunc()
  {
    if (g_verbose)
    {
      printf("tid=%d, %s started\n", convey::CurrentThread::tid(), convey::CurrentThread::name());
    }

    std::map<int, int> delays;
    latch_.countDown();
    bool running = true;
    while (running)
    {
      convey::Timestamp t(queue_.take());  // 从队列取出时间戳
      convey::Timestamp now(convey::Timestamp::now());
      if (t.valid())
      {
        int delay = static_cast<int>(timeDifference(now, t) * 1000000);
        // printf("tid=%d, latency = %d us\n",
        //        convey::CurrentThread::tid(), delay);
        ++delays[delay];
        delay_queue_.put(delay);
      }
      running = t.valid();  // 有不合法的时间戳就结束该线程
    }

    if (g_verbose)
    {
      printf("tid=%d, %s stopped\n", convey::CurrentThread::tid(), convey::CurrentThread::name());
      for (const auto &delay : delays)
      {
        printf("tid = %d, delay = %d, count = %d\n", convey::CurrentThread::tid(), delay.first, delay.second);
      }
    }
  }

  convey::BlockingQueue<convey::Timestamp> queue_;
  convey::BlockingQueue<int> delay_queue_;
  convey::CountDownLatch latch_;
  std::vector<std::unique_ptr<convey::Thread>> threads_;
};

int main(int argc, char *argv[])
{
  int threads = argc > 1 ? atoi(argv[1]) : 1;

  Bench t(threads);
  t.run(100000);  // 主线程往队列添加时间戳，子线程消耗队列的时间戳
  t.joinAll();
}
