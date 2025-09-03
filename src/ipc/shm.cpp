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

#include "shm.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include "logger.h"
#include "status_or.h"
#include "utility.h"

StatusOr<Shm> Shm::Create(const std::string& path, char mode,
                          size_t alloc_size) {
  int flags = O_RDWR;
  if (mode == 'w') {
    flags |= O_CREAT;
  }

  int shm_fd = shm_open(path.c_str(), flags, 0644);
  if (shm_fd == -1) {
    return StatusVal(ErrorCode::NOT_FOUND);
  }

  size_t size = round_to_page(alloc_size);
  if (mode == 'w') {
    int r = posix_fallocate(shm_fd, 0, size);
    if (r != 0) {
      ERROR("Failed to allocate shared memory: %s", errno_to_string(r).c_str());
      close(shm_fd);
      return StatusVal(ErrorCode::GENERAL);
    }
  }

  void* map =
      mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (map == MAP_FAILED) {
    ERROR("Failed to map shared memory: %s", errno_to_string(errno).c_str());
    close(shm_fd);
    return StatusVal(ErrorCode::GENERAL);
  }
  LOG(kLogSync, "Mapped shm to %p", map);

  return Shm(shm_fd, mode, size, map);
}

Shm::Shm(int shm_fd, char mode, size_t size, void* map)
    : shm_fd_(shm_fd), mode_(mode), size_(size), map_(map) {}

Shm::~Shm() {
  if (map_ != nullptr) {
    munmap(map_, size_);
  }
  if (shm_fd_ != 0) {
    close(shm_fd_);
  }
}

void Shm::resize(size_t new_size) {
  new_size = round_to_page(new_size);
  if (new_size == size_) {
    return;
  }
  
  if (mode_ == 'w') {
    int r = ftruncate(shm_fd_, new_size);
    CCHECK(r == 0, "Failed to allocate shared memory", errno);
  }
  
  void* map_val = mremap(map_, size_, new_size, MREMAP_MAYMOVE);
  CCHECK(map_val != MAP_FAILED, "Failed to remap shared memory", errno);
  LOG(kLogSync, "Remapped shm to %p", map_val);
  
  map_ = map_val;
  size_ = new_size;
}
