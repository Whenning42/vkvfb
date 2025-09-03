/*
 * Copyright (C) 2025 William Henning
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IPC_PMUTEX_H_
#define IPC_PMUTEX_H_

#include <pthread.h>
#include <tuple>
#include <time.h>
#include <errno.h>
#include <cstdint>
#include "utility.h"
#include "logger.h"

namespace {
struct timespec get_abstime(uint64_t nanos_from_now) {
  struct timespec abs_timeout;
  CCHECK(clock_gettime(CLOCK_REALTIME, &abs_timeout) != -1, "clock_gettime failed", errno);
  abs_timeout.tv_sec += nanos_from_now / 1'000'000'000;
  abs_timeout.tv_nsec += nanos_from_now % 1'000'000'000;
  if (abs_timeout.tv_nsec >= 1'000'000'000) {
    abs_timeout.tv_sec += 1;
    abs_timeout.tv_nsec -= 1'000'000'000;
  }
  return abs_timeout;
}
}

enum class LockState {
  LOCKED,
  TIMEOUT,
  OWNERDEAD
};

class PGuard;

class PMutex;
class PGuard {
 public:
  PGuard(): mu_(nullptr) {}
  PGuard(PMutex& mu): mu_(&mu) {}
  ~PGuard();

 private:
  PMutex* mu_;
};

struct LockResult {
  LockState state;
  PGuard guard;
};

class PMutex {
 public:
  PMutex(bool create) {
    if (create) {
      pthread_mutexattr_t attr;
      CCHECK_EVAL(pthread_mutexattr_init(&attr), "mutexattr_init");
      CCHECK_EVAL(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED), "mutexattr_setpshared");
      CCHECK_EVAL(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST), "mutexattr_setrobust");
      CCHECK_EVAL(pthread_mutex_init(&pmu_, &attr), "mutex_init");
    }
  }
  ~PMutex() = default;
  PMutex(const PMutex& other) = delete;
  PMutex(const PMutex&& other) = delete;

  LockResult lock() { return lock(UINT64_MAX); }
  LockResult lock(uint64_t timeout_nanos) {
    struct timespec abs_timeout = get_abstime(timeout_nanos);
    int r = pthread_mutex_timedlock(&pmu_, &abs_timeout);
    if (r == 0) {
      return LockResult{LockState::LOCKED, PGuard(*this)};
    } else if (r == ETIMEDOUT) {
      return LockResult{LockState::TIMEOUT, PGuard()};
    } else if (r == EOWNERDEAD) {
      return LockResult{LockState::OWNERDEAD, PGuard()};
    } else  {
      CCHECK_EVAL(r, "Unexpected error value from mutex_lock");
      // Silence no return from function warning, even though we're guaranteed
      // that the check here has failed.
      exit(1);
    } 
  }
  void unlock() {
    CCHECK_EVAL(pthread_mutex_unlock(&pmu_), "mutex_unlock");
  }

  // Sets the pthread_mutex to a consistent state and unlocks it.
  void reset() {
    // pthread_mutex_consistent only returns EINVAL if the mutex is already
    // consistent, which is fine, or if the mutex isn't a robust one, which we shouldn't
    // hit.
    (void)pthread_mutex_consistent(&pmu_);
    unlock();
  }

 private:
  pthread_mutex_t pmu_;
};

inline PGuard::~PGuard() {
  if (mu_) {
    mu_->unlock();
  }
}

#endif // IPC_PMUTEX_H_
