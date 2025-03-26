/// @file Stopwatch.h
/// @author Mike Hebert (mike@inkbit3d.com)
///
/// @date 2021-01-12
///
/// Yet another implementation of a Timer... This one lets you pause and resume.

#pragma once

#include <cassert>
#include <chrono>

namespace Utils {

/// A Timer that can be paused and tracks total time.
class Stopwatch {
  // @todo(mike) paramaterize this
  using Clock = std::chrono::system_clock;

 public:
  Stopwatch() = default;

  /// (Re)starts stopwatch. @returns the start time.
  std::chrono::system_clock::time_point Start() {
    started_ = true;
    running_ = true;
    start_ = std::chrono::system_clock::now();
    last_start_ = start_;
    elapsed_ = Clock::duration::zero();
    return start_;
  }

  /// Stops increasing elapsed. @returns current elapsed duration.
  Clock::duration Pause() {
    if (running_) {
      elapsed_ += std::chrono::system_clock::now() - last_start_;
    }
    running_ = false;

    return elapsed_;
  }

  /// Resumes a paused stopwatch. @returns current elapsed time
  Clock::duration Resume() {
    if (!started_) {
      Start();
    }
    running_ = true;
    last_start_ = std::chrono::system_clock::now();
    return elapsed_;
  }

  bool IsStarted() const { return started_; }

  std::chrono::system_clock::time_point StartTime() const {
    assert(started_);
    return start_;
  }

  Clock::duration Elapsed() const {
    if (running_) {
      return elapsed_ + std::chrono::system_clock::now() - last_start_;
    }

    return elapsed_;
  }

  float ElapsedSeconds() const { 
    Clock::duration elapsed = Elapsed();
    auto secs = std::chrono::duration<float>(elapsed);
    return secs.count();
  }

  float ElapsedMS() const {
    Clock::duration elapsed = Elapsed();
    auto ms = std::chrono::duration<float, std::milli>(elapsed);
    return ms.count();
  }

 private:
  bool started_ = false;
  bool running_ = false;
  std::chrono::system_clock::time_point start_;
  std::chrono::system_clock::time_point last_start_;
  Clock::duration elapsed_ = Clock::duration::zero();
};

}  // namespace Utils