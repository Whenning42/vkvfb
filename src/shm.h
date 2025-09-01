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

#ifndef SHM_H
#define SHM_H

#include <string>

class Shm {
 public:
  // Constructs an empty default shared memory object.
  Shm() = default;

  // Mode must be either 'r' or 'w'.
  // In 'w' mode, a new shm is opened at the give path.
  // In 'r' mode, it's required that a shm already exists at path and it's opened
  // for reading.
  // Note: Calls to Shm(...) don't shrink existing shm allocations.
  Shm(const std::string& path, char mode, size_t alloc_size);
  void resize(size_t new_size);
  void* map() { return map_; }
  size_t size() { return size_; }

 private:
  int shm_fd_ = 0;
  char mode_ = 'r';
  size_t size_ = 0;
  void* map_ = nullptr;
};

#endif
