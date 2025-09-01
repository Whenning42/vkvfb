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

#ifndef TIMEOUT_SEM_H_
#define TIMEOUT_SEM_H_

// Note: It may be simpler to use a double-buffered robust pthread_mutex locked
// pixbuf instead of a 

#include <atomic>
#include <cstddef>
#include <semaphore.h>
#include <string>



class MPMu {
 public:
  MPMu(const std::string& name, int oflag, int mode);
  ~MPMu();
  void lock();
  void unlock();

 private:
  sem_t* sem_ = nullptr;
};

// A generational semaphore that supports timeouts for waits that unlocks
// the semahpore if the thread can't acquire the semaphore within the specified time.
class Semaphore {
 public:
  using gen = int64_t;
  Semaphore(const std::string& path, bool create, int32_t initial_value);
  ~Semaphore();

  void post(gen g);
  // Pass in UINT64_MAX to wait indefintely.
  gen wait(uint64_t nanos);
  int32_t get_value();
  const std::string name() { return name_; }

 private:
  void advance_gen();

  std::atomic<gen> generation_ = 0;

  // Protects sem in the post and advance gen case.
  MPMu sem_mu_;
  sem_t* sem_;
  int32_t initial_value_;
  std::string name_;
};

#endif  // TIMEOUT_SEM_H_

