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

#ifndef FAKE_PMUTEX_H_
#define FAKE_PMUTEX_H_

#include <tuple>
#include <cstdint>
#include "pmutex.h"

class FakePMutex {
 public:
  FakePMutex(bool create): state_(LockState::LOCKED) {}
  FakePMutex(const FakePMutex& other) = delete;
  FakePMutex(const FakePMutex&& other) = delete;

  LockResult lock() { return lock(UINT64_MAX); }
  LockResult lock(uint64_t timeout_nanos) {
    return LockResult{state_, PGuard()};
  }
  void unlock() {}

  void reset() {
    if (state_ == LockState::OWNERDEAD) {
      state_ = LockState::LOCKED;
    }
  }

  void SetState(LockState state) {
    state_ = state;
  }

 private:
  LockState state_;
};

#endif  // FAKE_PMUTEX_H_
