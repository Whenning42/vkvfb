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

#ifndef SHM_MUTEX_H_
#define SHM_MUTEX_H_

#include "pmutex.h"
#include "shm.h"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include "utility.h"


class ShmMutex {
 public:
  ShmMutex(const std::string& path, bool create) {
    const std::string lock_path = "/tmp/" + path;
    int fd = open(lock_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
    CCHECK(fd != -1, "Failed to open ShmMutex lock file", errno);
    int r = flock(fd, LOCK_EX | LOCK_NB);
    if (r == -1 && errno == EWOULDBLOCK) {
      create = false;
    } else {
      CCHECK(r != -1, "Unexpected flock error", errno);
    }

    shm_ = Shm(path, create ? 'w' : 'r', sizeof(PMutex));
    mu_ = new (shm_.map()) PMutex(create);
  }

  PMutex& mu() { return *mu_; }

 private:
  Shm shm_;
  PMutex* mu_;
};

#endif // SHM_MUTEX_H_
