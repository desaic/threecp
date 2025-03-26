#include "ThreadPool.h"

void ThreadPool::workerLoop() {
  while (1) {
    std::shared_ptr<TPTask> task;
    int ret;
    ret = popTask(task);
    if (ret < 0) {
      // no more tasks. done.
      break;
    }
    if (task != nullptr) {
      task->run();
    }
  }
}

void ThreadPool::addTask(TaskPtr task) {
  std::lock_guard lock(queueLock);
  taskQueue.push(task);
}

int ThreadPool::run() {
  std::lock_guard lock(busyLock);
  if (userNumThreads > 0) {
    numThreads = userNumThreads;
  } else {
    unsigned int processorCount = std::thread::hardware_concurrency();
    if (processorCount < 3) {
      numThreads = 1;
    } else {
      numThreads = processorCount - 2;
    }
  }
  // check any unfinished tasks
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  threads.resize(numThreads);
  for (size_t i = 0; i < threads.size(); i++) {
    threads[i] = std::thread(&ThreadPool::workerLoop, this);
  }

  // check any unfinished tasks
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  return 0;
}

int ThreadPool::popTask(TaskPtr& task) {
  std::lock_guard<std::mutex> lock(queueLock);
  if (taskQueue.empty()) {
    return -1;
  }
  task = taskQueue.front();
  taskQueue.pop();
  return 0;
}
