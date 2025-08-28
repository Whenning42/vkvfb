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

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

#include "shm_pixbuf_data.h"
#include "tinylog.h"

namespace {

size_t round_to_page(size_t size) {
  size_t ps = getpagesize();
  return ps * ((size + ps - 1) / ps);
}

}

Shm::Shm(const std::string& path, char mode) : mode_(mode) {
  int flags = O_RDWR;
  if (mode == 'w') {
    flags |= O_CREAT;
  }
  
  shm_fd_ = shm_open(path.c_str(), flags, 0644);
  if (shm_fd_ == -1) {
    perror("Failed to open shared memory");
    exit(1);
  }
  
  size_ = ShmPixbufData::pixbuf_struct_size(0, 0);
  
  if (mode == 'w') {
    if (ftruncate(shm_fd_, size_) == -1) {
      perror("Failed to truncate shared memory");
      close(shm_fd_);
      exit(1);
    }
  }
  
  map_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
  if (map_ == MAP_FAILED) {
    perror("Failed to map shared memory");
    close(shm_fd_);
    exit(1);
  }
}

void Shm::resize(size_t new_size) {
  new_size = round_to_page(new_size);
  if (new_size == size_) {
    return;
  }
  LOGF("Resizing shm with mode '%c' old size: %zu, new size: %zu\n", mode_, size_, new_size);
  
  if (mode_ == 'w') {
    if (ftruncate(shm_fd_, new_size) == -1) {
      perror("Failed to resize shared memory");
      exit(1);
    }
  }
  
  void* new_map = mremap(map_, size_, new_size, MREMAP_MAYMOVE);
  if (new_map == MAP_FAILED) {
    perror("Failed to remap shared memory");
    exit(1);
  }
  
  map_ = new_map;
  size_ = new_size;
}
