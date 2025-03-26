#pragma once

#include <stdint.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

template <typename T>
class threadsafe_queue {
 private:
  mutable std::mutex mut;
  std::deque<T> data_queue;
  std::condition_variable data_cond;
  bool cancel = false;

 public:
  threadsafe_queue() = default;

  ~threadsafe_queue() {
    cancel = true;
    data_cond.notify_all();
  }

  void push(const T& new_value) {
    {
      std::lock_guard<std::mutex> g(mut);
      data_queue.push_back(new_value);
    }
    data_cond.notify_all();
  }

  void push_front(const T& new_value) {
    {
      std::lock_guard<std::mutex> g(mut);
      data_queue.push_front(std::move(new_value));
    }
    data_cond.notify_all();
  }
  template <class _Iter>
  void insert(_Iter first, _Iter last) {
    {
      std::lock_guard<std::mutex> g(mut);
      data_queue.insert(data_queue.end(), first, last);
    }
    data_cond.notify_all();
  }

  bool wait_and_pop(T& value, uint32_t wait_milliseconds = 0) {
    std::unique_lock<std::mutex> lk(mut);
    if (wait_milliseconds == 0) {
      data_cond.wait(lk, [this] { return cancel || !data_queue.empty(); });
    } else {
      data_cond.wait_for(lk, std::chrono::milliseconds(wait_milliseconds),
                         [this] { return cancel || !data_queue.empty(); });
    }

    if (cancel) {
      return false;
    }

    if (!data_queue.empty()) {
      value = std::move(data_queue.front());
      data_queue.pop_front();
      return true;
    }
    return false;
  }

  bool pop_all(std::vector<T>& out, uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait_for(lk, std::chrono::milliseconds(timeoutMs),
                       [this] { return cancel || !data_queue.empty(); });
    if (cancel) {
      return false;
    }
    if (data_queue.empty()) {
      return false;
    }
    out = std::vector(data_queue.begin(), data_queue.end());
    data_queue.clear();
    return true;
  }

  bool try_pop(T& value) {
    std::lock_guard<std::mutex> g(mut);
    if (data_queue.empty()) {
      return false;
    }
    value = std::move(data_queue.front());
    data_queue.pop_front();
    return true;
  }

  bool empty() const {
    std::lock_guard<std::mutex> g(mut);
    return data_queue.empty();
  }

  void clear() {
    std::lock_guard g(mut);
    data_queue.clear();
  }

  std::size_t size() const {
    std::lock_guard<std::mutex> g(mut);
    return data_queue.size();
  }

  std::deque<T> copy() const {
    std::lock_guard<std::mutex> g(mut);
    return data_queue;
  }
};
