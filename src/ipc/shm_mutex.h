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

#ifndef IPC_SHM_MUTEX_H_
#define IPC_SHM_MUTEX_H_

#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <string>

#include "pmutex.h"
#include "shm.h"
#include "status_or.h"
#include "utility.h"

class ShmMutex {
 public:
  // Factory function to create a ShmMutex.
  // Returns StatusOr<ShmMutex> with appropriate error status if creation fails.
  static StatusOr<ShmMutex> Create(const std::string& path, bool create) {
    const std::string lock_path = "/tmp/" + path;
    int fd = open(lock_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    if (fd == -1) {
      return StatusVal(ErrorCode::GENERAL);
    }
    int r = flock(fd, LOCK_EX | LOCK_NB);
    if (r == -1 && errno == EWOULDBLOCK) {
      create = false;
    } else if (r == -1) {
      close(fd);
      return StatusVal(ErrorCode::GENERAL);
    }

    StatusOr<Shm> shm_result =
        Shm::Create(path, create ? 'w' : 'r', sizeof(PMutex));
    RETURN_IF_ERROR(shm_result);

    PMutex* mu = new (shm_result->map()) PMutex(create);
    close(fd);
    return ShmMutex(std::move(*shm_result), mu);
  }

  // We manually call mu_'s destructor, since it was placement-new'd into shared
  // memory.
  ~ShmMutex() { mu_->~PMutex(); }
  PMutex& mu() { return *mu_; }

 private:
  // Private constructor, use Create() instead.
  ShmMutex(Shm&& shm, PMutex* mu) : shm_(std::move(shm)), mu_(mu) {}

  Shm shm_;
  PMutex* mu_;
};

#endif // IPC_SHM_MUTEX_H_
