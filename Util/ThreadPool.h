///\file ThreadPool.h
/// a simple multithreading class

#include <memory>
#include <mutex>
#include <queue>
#include <thread>

// thread pool task
class TPTask {
 public:
  ///\return status code
  virtual void run() = 0;
  /// optional return code used by application.
  int returnCode;
  TPTask() : returnCode(0) {}
  virtual ~TPTask() {}
};

class ThreadPool {
 public:
  using TaskPtr = std::shared_ptr<TPTask>;
  ThreadPool() : numThreads(1) { threads.resize(1); }

  void addTask(TaskPtr task);

  ///\param task if queue is non empty,
  /// a TPTask pointer is assigned to task.
  int popTask(TaskPtr& task);

  /// runs all the tasks using mutiple threads and blocks until all threads
  /// finish. if run is called from multiple threads, they are executed one at a
  /// time.
  int run();
  // if 0, use total num CPU threads-2.
  // max is 1000;
  void setUserNumThreads(unsigned n) {
    if (n > 1000) {
      n = 1000;
    }
    userNumThreads = n;
  }

 private:
  int numThreads;
  std::queue<TaskPtr> taskQueue;
  std::mutex queueLock;
  std::vector<std::thread> threads;
  std::mutex busyLock;

  /// function run by each worker
  void workerLoop();
  // use user config num threads instead auto
  unsigned userNumThreads = 0;
};
